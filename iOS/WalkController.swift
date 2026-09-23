import Observation
import RealityKit
import SwiftUI
import simd

/// First-person walking on the phone: joystick to walk (with collision), drag to look,
/// tap a painting for its placard. Eye height 1.6 m.
@MainActor
@Observable
final class WalkController {
    let scene: MuseumScene
    @ObservationIgnored var camera: Entity?
    @ObservationIgnored var updateSubscription: EventSubscription?

    // Visitor state.
    private(set) var position: SIMD2<Float>
    /// Height of the visitor's feet (0 on the ground floor, −5.8 in the Reserve, up to 99.4 in the Sphere).
    private(set) var feet: Float = 0
    private(set) var yaw: Float
    private(set) var pitch: Float = 0
    var joystick: SIMD2<Float> = .zero       // x right, y forward, length ≤ 1
    var viewSize: CGSize = CGSize(width: 390, height: 844) { didSet { applyFieldOfView() } }

    // What the UI shows.
    var placard: Placard?
    var prompts: [ActionPrompt] = []
    var message: String?

    private let radius: Float = 0.3
    private let maxSpeed: Float = 2.0          // m/s at full deflection
    private let lookSensitivity: Float = 0.0045 // radians per point
    private let pitchLimit: Float = 80 * .pi / 180
    @ObservationIgnored private var uiTimer: Float = 0

    struct Placard: Equatable {
        let title: String
        let detail: String?
        let artist: String
        let year: String
        let collection: String
    }

    init(scene: MuseumScene) {
        self.scene = scene
        position = scene.spawn
        yaw = scene.spawnYaw
    }

    // MARK: Camera

    /// Horizontal field of view 75° in portrait; vertical 55° in landscape.
    private var fov: (horizontal: Bool, degrees: Float) {
        viewSize.height > viewSize.width ? (true, 75) : (false, 55)
    }

    private func applyFieldOfView() {
        guard let camera else { return }
        var c = camera.components[PerspectiveCameraComponent.self] ?? PerspectiveCameraComponent()
        c.fieldOfViewInDegrees = fov.degrees
        c.fieldOfViewOrientation = fov.horizontal ? .horizontal : .vertical
        c.near = 0.05
        c.far = 2000
        camera.components.set(c)
    }

    func attach(camera: Entity) {
        self.camera = camera
        applyFieldOfView()
        applyTransform()
    }

    private var orientation: simd_quatf {
        simd_quatf(angle: yaw, axis: [0, 1, 0]) * simd_quatf(angle: pitch, axis: [1, 0, 0])
    }

    private var eye: SIMD3<Float> { [position.x, feet + Plan.eyeHeight, position.y] }

    private func applyTransform() {
        camera?.position = eye
        camera?.orientation = orientation
    }

    // MARK: Per frame

    func update(dt: Float) {
        let dt = min(dt, 0.1)
        let j = joystick
        let mag = simd_length(j)
        if mag > 0.02 {
            // Gentle response near the centre, full speed at the rim.
            let speed = maxSpeed * mag * mag
            let dir = j / mag
            let forward = SIMD2<Float>(-sin(yaw), -cos(yaw))
            let right = SIMD2<Float>(cos(yaw), -sin(yaw))
            let delta = (forward * dir.y + right * dir.x) * speed * dt
            let moved = scene.collision.move(from: position, by: delta, radius: radius, feet: feet)
            // Only step where there is floor within a step of your feet (stairs, not stairwells).
            if let floor = scene.floors.floor(at: moved, feet: feet) {
                position = moved
                feet = floor
            } else {
                // Try sliding along each axis before giving up.
                for axis in [SIMD2<Float>(moved.x, position.y), SIMD2<Float>(position.x, moved.y)] {
                    if let floor = scene.floors.floor(at: axis, feet: feet) {
                        position = axis
                        feet = floor
                        break
                    }
                }
            }
        }
        // Moving floors (the elevator) carry you.
        if let floor = scene.floors.floor(at: position, feet: feet) { feet = floor }
        scene.visitorFeet = feet
        scene.visitorPlan = position
        scene.visitorForward = orientation.act([0, 0, -1])
        scene.update(dt: dt, eye: eye)
        applyTransform()
        uiTimer += dt
        if uiTimer > 0.2 {
            uiTimer = 0
            let p = scene.prompts(at: eye)
            if p != prompts { prompts = p }
            if scene.message != message { message = scene.message }
        }
    }

    func perform(_ prompt: ActionPrompt) { scene.perform(prompt.id) }

    func look(dx: CGFloat, dy: CGFloat) {
        yaw -= Float(dx) * lookSensitivity
        pitch = simd_clamp(pitch - Float(dy) * lookSensitivity, -pitchLimit, pitchLimit)
        applyTransform()
    }

    #if DEBUG
    /// Debug-only: jump to a pose (used for headless checks in the Simulator).
    func debugPose(x: Float, z: Float, yawDegrees: Float, pitchDegrees: Float, feet f: Float = 0) {
        position = [x, z]
        feet = f
        yaw = yawDegrees * .pi / 180
        pitch = pitchDegrees * .pi / 180
        applyTransform()
    }
    #endif

    // MARK: Tapping a painting

    func tap(at point: CGPoint) {
        let w = Float(viewSize.width), h = Float(viewSize.height)
        guard w > 0, h > 0 else { return }
        let nx = 2 * Float(point.x) / w - 1
        let ny = 1 - 2 * Float(point.y) / h
        let half = fov.degrees * .pi / 360
        let (tx, ty): (Float, Float) = fov.horizontal
            ? (tan(half), tan(half) * h / w)
            : (tan(half) * w / h, tan(half))
        let local = normalize(SIMD3<Float>(nx * tx, ny * ty, -1))
        let dir = orientation.act(local)
        guard let target = scene.pick(origin: eye, direction: dir) else {
            placard = nil
            return
        }
        if let id = target.artworkID {
            let art = Catalogue.artwork(id)
            let newPlacard = Placard(title: art?.title ?? id, detail: target.detail,
                                     artist: art?.artist ?? "", year: art?.year ?? "",
                                     collection: [art?.collection, art?.city].compactMap { $0 }.joined(separator: ", "))
            placard = (placard == newPlacard && target.action == nil) ? nil : newPlacard
        } else {
            placard = nil
        }
        target.action?()
        message = scene.message
    }
}
