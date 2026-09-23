import Foundation
import ImageIO
import RealityKit
import simd

/// Something you can tap: a work (for its placard) and/or a thing that does something
/// (the golden lily, a rack handle, the elevator's call ring).
struct PickTarget {
    let artworkID: String?
    /// Extra line under the title (e.g. which Water Lilies panel).
    var detail: String? = nil
    /// Returns the distance along the ray to the hit, if the ray hits.
    let hit: (_ origin: SIMD3<Float>, _ direction: SIMD3<Float>) -> Float?
    /// Runs when tapped (after the placard, if any, is shown).
    var action: (@MainActor () -> Void)? = nil
    /// Only pickable while this returns true.
    var active: (@MainActor () -> Bool)? = nil
}

/// A button the phone shows when something can be done where you stand (the elevator panel).
struct ActionPrompt: Identifiable, Equatable {
    let id: String
    let title: String
    let symbol: String
}

/// Builds the whole museum procedurally from the plan data and runs its moving parts. It is
/// platform-neutral so a visionOS target can reuse it; the iOS app adds the camera, controls,
/// placards and buttons.
@MainActor
final class MuseumScene {
    let root = Entity()
    /// Everything built (walls, works, gardens). Hidden while you are inside the Sphere.
    let building = Entity()
    var skySystem: SkySystem?
    var collision = CollisionWorld()
    var floors = FloorWorld()
    var targets: [PickTarget] = []
    var observer = Observer()

    /// Where every visit begins: on the gilt sun at the centre of the Rotunda, facing the open
    /// west door to the Salon.
    let spawn = SIMD2<Float>(0, 0)
    let spawnYaw: Float = .pi / 2

    /// Things that need a tick every frame (pond, elevator, scroll, racks, sky…).
    var updaters: [(Float, SIMD3<Float>) -> Void] = []
    /// Buttons offered at the visitor's position.
    var promptProviders: [(SIMD3<Float>) -> [ActionPrompt]] = []
    var actionHandlers: [String: () -> Void] = [:]
    /// A short line of text for the visitor (e.g. "The car is on its way down").
    var message: String?

    // Painting textures stream in and out by distance to keep memory in check.
    struct PaintingSlot {
        let entity: ModelEntity
        let image: String
        let maxPixels: Int
        let position: () -> SIMD3<Float>
        var tint: UInt32 = 0xF4F1EA
        var loaded = false
        var loading = false
    }
    var paintingSlots: [PaintingSlot] = []
    let placeholder = Mat.glow(0xB8AE9C)

    // Lights: only the nearest few are switched on (RealityKit handles about eight).
    var lights: [(entity: Entity, position: SIMD3<Float>, always: Bool)] = []
    private var lightTimer: Float = 1

    // Moving parts of the wings.
    var pondLift: PondLift?
    var reserveRacks: [ReserveRack] = []
    var reserveWorks: [(entity: Entity, rack: ReserveRack, local: SIMD3<Float>, id: String)] = []
    var easelWork: Int?
    var pendingEasel: Int?
    var carry: (index: Int, toEasel: Bool, t: Float, from: float4x4)?
    let easelLight = Entity()
    var elevator: Elevator?
    var chickenCup: (entity: Entity, home: SIMD3<Float>)?
    var cupHeld = false
    var visitorForward: SIMD3<Float> = [0, 0, -1]
    var irisProxy: Entity?
    /// Where the visitor stands (set by the controller every frame).
    var visitorFeet: Float = 0
    var visitorPlan: SIMD2<Float> = .zero

    // The Rotunda's sun clock.
    let sunLight = Entity()
    let shadowPoint = ModelEntity()
    let sunPatch = ModelEntity()

    lazy var slabTexture = Textures.resource(Textures.stoneSlab())
    lazy var cofferTexture = Textures.resource(Textures.coffer())
    lazy var lightGridTexture = Textures.resource(Textures.lightGrid())

    init() {
        root.name = "Musée Vision"
        building.name = "Building"
        root.addChild(building)
        floors.add("Level 0", .everywhere, height: 0, holes: groundHoles())
        buildRotunda()
        buildPassage()
        buildSalon()
        buildCabinet()
        buildOval()
        buildHang()
        buildSalonLights()
        buildReserve()
        buildSculptureHall()
        buildChineseWing()
        buildHallOfLight()
        buildElan()
        buildSky()
    }

    // MARK: Building helpers

    @discardableResult
    func add(_ b: MeshBuilder, _ material: RealityKit.Material, name: String, to parent: Entity? = nil) -> ModelEntity? {
        guard !b.isEmpty else { return nil }
        let e = ModelEntity(mesh: b.mesh(name: name), materials: [material])
        e.name = name
        (parent ?? building).addChild(e)
        return e
    }

    func addLight(_ e: Entity, always: Bool = false) {
        root.addChild(e)
        lights.append((e, e.position(relativeTo: nil), always))
    }

    func spot(at p: SIMD3<Float>, looking target: SIMD3<Float>, colour: UInt32 = 0xFFF4E2, intensity: Float,
              inner: Float = 50, outer: Float = 88, radius: Float = 24) -> Entity {
        let e = Entity()
        e.components.set(SpotLightComponent(color: PlatformColor(hex: colour), intensity: intensity,
                                            innerAngleInDegrees: inner, outerAngleInDegrees: outer, attenuationRadius: radius))
        e.position = p
        e.look(at: target, from: p, relativeTo: nil)
        return e
    }

    /// A framed picture: gilt (or other) frame on the wall, the image as a self-lit canvas.
    /// `wall` is the centre of the picture on the wall surface, `facing` the direction into the room.
    @discardableResult
    func hangFramed(id: String, artworkID: String?, image: String, wall: SIMD3<Float>, facing f: SIMD3<Float>,
                    width: Float, height: Float, frame: (width: Float, depth: Float, colour: UInt32, metallic: Bool)? = nil,
                    detail: String? = nil, parent: Entity? = nil, tint: UInt32 = 0xF4F1EA) -> Entity {
        let entity = Entity()
        entity.name = id
        entity.position = wall
        entity.orientation = simd_quatf(angle: atan2(f.x, f.z), axis: [0, 1, 0])
        let fw = frame?.width ?? (width < 1 ? 0.07 : (width < 2 ? 0.09 : 0.12))
        let depth = frame?.depth ?? 0.06
        let w = width / 2, h = height / 2
        if fw > 0 {
            var fr = MeshBuilder()
            fr.box(min: [-w - fw, -h - fw, 0], max: [w + fw, -h, depth])
            fr.box(min: [-w - fw, h, 0], max: [w + fw, h + fw, depth])
            fr.box(min: [-w - fw, -h, 0], max: [-w, h, depth])
            fr.box(min: [w, -h, 0], max: [w + fw, h, depth])
            let colour = frame?.colour ?? Mat.gilt
            let mat = (frame?.metallic ?? true) ? Mat.metal(colour, roughness: 0.35) : Mat.matte(colour)
            entity.addChild(ModelEntity(mesh: fr.mesh(name: "frame"), materials: [mat]))
        }
        let picture = ModelEntity(mesh: .generatePlane(width: width, height: height), materials: [placeholder])
        picture.position = [0, 0, max(depth - 0.015, 0.004)]
        entity.addChild(picture)
        (parent ?? building).addChild(entity)
        paintingSlots.append(PaintingSlot(entity: picture, image: image, maxPixels: pixels(for: max(width, height)),
                                          position: { [weak entity] in entity?.position(relativeTo: nil) ?? wall }, tint: tint))
        if let artworkID {
            let right = SIMD3<Float>(f.z, 0, -f.x)
            let front = wall + f * depth
            targets.append(PickTarget(artworkID: artworkID, detail: detail, hit: { [weak entity] o, d in
                guard let entity else { return nil }
                let c = entity.position(relativeTo: nil) + (front - wall)
                return Self.rayRect(o, d, centre: c, normal: f, right: right, halfW: w + fw, halfH: h + fw)
            }))
        }
        return entity
    }

    /// Registers any mesh that shows an image, for streaming.
    func addImageSlot(_ e: ModelEntity, image: String, maxPixels: Int, at p: SIMD3<Float>, tint: UInt32 = 0xF4F1EA) {
        paintingSlots.append(PaintingSlot(entity: e, image: image, maxPixels: maxPixels, position: { p }, tint: tint))
    }

    func pixels(for longSide: Float) -> Int {
        Int(min(2048, max(768, longSide * 700)))
    }

    // MARK: Per frame

    func update(dt: Float, eye: SIMD3<Float>) {
        for u in updaters { u(dt, eye) }
        lightTimer += dt
        if lightTimer > 0.25 {
            lightTimer = 0
            updateLights(eye)
            streamPaintings(eye)
        }
    }

    func prompts(at eye: SIMD3<Float>) -> [ActionPrompt] {
        promptProviders.flatMap { $0(eye) }
    }

    func perform(_ id: String) { actionHandlers[id]?() }

    private func updateLights(_ eye: SIMD3<Float>) {
        // Weight height differences heavily so other floors' lights don't count as near.
        func d(_ p: SIMD3<Float>) -> Float {
            let v = p - eye
            return v.x * v.x + v.z * v.z + 9 * v.y * v.y
        }
        let order = lights.indices.filter { !lights[$0].always }.sorted { d(lights[$0].position) < d(lights[$1].position) }
        let alwaysCount = lights.filter { $0.always && $0.entity.isEnabled }.count
        let budget = max(0, 7 - alwaysCount)
        for (rank, i) in order.enumerated() { lights[i].entity.isEnabled = rank < budget }
    }

    // MARK: Painting streaming

    private func streamPaintings(_ eye: SIMD3<Float>) {
        let loadRadius: Float = 34, unloadRadius: Float = 52
        var inFlight = paintingSlots.filter(\.loading).count
        let order = paintingSlots.indices.sorted {
            simd_distance(paintingSlots[$0].position(), eye) < simd_distance(paintingSlots[$1].position(), eye)
        }
        for i in order {
            let dist = simd_distance(paintingSlots[i].position(), eye)
            if dist < loadRadius, !paintingSlots[i].loaded, !paintingSlots[i].loading, inFlight < 3 {
                paintingSlots[i].loading = true
                inFlight += 1
                load(slot: i)
            } else if dist > unloadRadius, paintingSlots[i].loaded {
                paintingSlots[i].entity.model?.materials = [placeholder]
                paintingSlots[i].loaded = false
            }
        }
    }

    private func load(slot i: Int) {
        let slot = paintingSlots[i]
        guard let url = Bundle.main.url(forResource: slot.image, withExtension: "jpg", subdirectory: "paintings")
                ?? Bundle.main.url(forResource: slot.image, withExtension: "jpg") else {
            paintingSlots[i].loading = false
            paintingSlots[i].loaded = true   // nothing to load; keep the placeholder
            return
        }
        let maxPixels = slot.maxPixels
        Task { @MainActor in
            let image: CGImage? = await Task.detached(priority: .userInitiated) {
                guard let src = CGImageSourceCreateWithURL(url as CFURL, nil) else { return nil }
                let opts: [CFString: Any] = [
                    kCGImageSourceCreateThumbnailFromImageAlways: true,
                    kCGImageSourceThumbnailMaxPixelSize: maxPixels,
                    kCGImageSourceCreateThumbnailWithTransform: true,
                ]
                return CGImageSourceCreateThumbnailAtIndex(src, 0, opts as CFDictionary)
            }.value
            if let image, let tex = try? await TextureResource(image: image, withName: nil,
                                                               options: .init(semantic: .color, mipmapsMode: .allocateAndGenerateAll)) {
                paintingSlots[i].entity.model?.materials = [Mat.glow(tex, tint: paintingSlots[i].tint)]
            }
            paintingSlots[i].loading = false
            paintingSlots[i].loaded = true
        }
    }

    // MARK: Picking

    /// Returns the nearest active target hit by a ray, unless a wall is in the way.
    func pick(origin o: SIMD3<Float>, direction d: SIMD3<Float>, maxDistance: Float = 25) -> PickTarget? {
        var best: (PickTarget, Float)?
        for t in targets where t.active?() ?? true {
            guard let dist = t.hit(o, d), dist < maxDistance else { continue }
            if best == nil || dist < best!.1 { best = (t, dist) }
        }
        guard let (target, dist) = best else { return nil }
        let flat = SIMD2<Float>(d.x, d.z)
        let flatLen = simd_length(flat)
        if flatLen > 1e-3,
           collision.firstHit(origin: [o.x, o.z], direction: flat / flatLen, maxDistance: dist * flatLen - 0.12,
                              eyeY: o.y) != nil {
            return nil
        }
        return target
    }

    nonisolated static func rayRect(_ o: SIMD3<Float>, _ d: SIMD3<Float>, centre c: SIMD3<Float>, normal n: SIMD3<Float>,
                                    right r: SIMD3<Float>, halfW: Float, halfH: Float) -> Float? {
        let denom = dot(d, n)
        guard denom < -1e-4 else { return nil }
        let t = dot(c - o, n) / denom
        guard t > 0 else { return nil }
        let local = o + d * t - c
        guard abs(dot(local, r)) <= halfW, abs(local.y) <= halfH else { return nil }
        return t
    }

    nonisolated static func rayBox(_ o: SIMD3<Float>, _ d: SIMD3<Float>, min lo: SIMD3<Float>, max hi: SIMD3<Float>) -> Float? {
        var tmin: Float = 0, tmax: Float = .greatestFiniteMagnitude
        for i in 0..<3 {
            if abs(d[i]) < 1e-6 {
                if o[i] < lo[i] || o[i] > hi[i] { return nil }
            } else {
                var t0 = (lo[i] - o[i]) / d[i], t1 = (hi[i] - o[i]) / d[i]
                if t0 > t1 { swap(&t0, &t1) }
                tmin = max(tmin, t0); tmax = min(tmax, t1)
                if tmin > tmax { return nil }
            }
        }
        return tmin
    }

    nonisolated static func raySphere(_ o: SIMD3<Float>, _ d: SIMD3<Float>, centre c: SIMD3<Float>, radius r: Float) -> Float? {
        let oc = o - c
        let b = dot(oc, d), cc = dot(oc, oc) - r * r
        let disc = b * b - cc
        guard disc >= 0 else { return nil }
        let t = -b - sqrt(disc)
        return t > 0 ? t : nil
    }

    nonisolated static func rayOvalPanel(_ o: SIMD3<Float>, _ d: SIMD3<Float>, centre c: SIMD2<Float>, a: Float, b: Float,
                                         t0: Float, t1: Float, y0: Float, y1: Float) -> Float? {
        let px = (o.x - c.x) / a, pz = (o.z - c.y) / b, dx = d.x / a, dz = d.z / b
        let A = dx * dx + dz * dz, B = 2 * (px * dx + pz * dz), C = px * px + pz * pz - 1
        let disc = B * B - 4 * A * C
        guard A > 1e-8, disc >= 0 else { return nil }
        let t = (-B + sqrt(disc)) / (2 * A)
        guard t > 0 else { return nil }
        let p = o + d * t
        guard p.y >= y0, p.y <= y1 else { return nil }
        let ang = atan2((p.z - c.y) / b, (p.x - c.x) / a)
        func within(_ x: Float) -> Bool { x >= t0 && x <= t1 }
        guard within(ang) || within(ang + 2 * .pi) || within(ang - 2 * .pi) else { return nil }
        return t
    }
}
