import RealityKit
import SwiftUI
import UIKit

@main
struct MuseeVisionApp: App {
    var body: some SwiftUI.Scene {
        WindowGroup {
            MuseumView()
                .statusBarHidden()
                .persistentSystemOverlays(.hidden)
        }
    }
}

struct MuseumView: View {
    @State private var controller = WalkController(scene: MuseumView.makeScene())
    @State private var showHint = true
    @State private var sceneReady = false
    @State private var location: LocationProvider?

    private let joystickRadius: CGFloat = 58

    /// The window's top safe inset (the Dynamic Island), since the view ignores safe areas.
    static var windowSafeTop: CGFloat {
        let scenes = UIApplication.shared.connectedScenes.compactMap { $0 as? UIWindowScene }
        return scenes.first?.windows.first?.safeAreaInsets.top ?? 47
    }

    static func makeScene() -> MuseumScene {
        let t0 = Date()
        let scene = MuseumScene()
        #if DEBUG
        print("[debug] built museum in \(Date().timeIntervalSince(t0)) s")
        #endif
        return scene
    }

    var body: some View {
        GeometryReader { geo in
            let topInset = max(geo.safeAreaInsets.top, 59)
            let joystickCentre = CGPoint(x: geo.safeAreaInsets.leading + joystickRadius + 28,
                                         y: geo.size.height - geo.safeAreaInsets.bottom - joystickRadius - 28)
            ZStack {
                RealityView { content in
                    let t0 = Date()
                    content.camera = .virtual
                    content.add(controller.scene.root)
                    let camera = Entity()
                    camera.name = "Visitor"
                    camera.components.set(PerspectiveCameraComponent())
                    content.add(camera)
                    controller.attach(camera: camera)
                    if let sky = try? await EnvironmentResource(equirectangular: Textures.skyEquirect()) {
                        content.environment = .skybox(sky)
                    }
                    #if DEBUG
                    if Lab.enabled { Lab.install(in: controller.scene.root) }
                    if Lab.stats { Lab.printStats(controller.scene.root); Lab.paintingMemory(controller.scene) }
                    if ProcessInfo.processInfo.arguments.contains("-export") { await Lab.export(controller.scene.root) }
                    #endif
                    #if DEBUG
                    print("[debug] scene ready after \(Date().timeIntervalSince(t0)) s")
                    #endif
                    controller.updateSubscription = content.subscribe(to: SceneEvents.Update.self) { event in
                        controller.update(dt: Float(event.deltaTime))
                    }
                    Task {
                        try? await Task.sleep(for: .seconds(1.2))
                        withAnimation(.easeOut(duration: 0.8)) { sceneReady = true }
                    }
                }
                .ignoresSafeArea()

                TouchSurface(joystickCentre: joystickCentre, joystickRadius: joystickRadius,
                             onJoystick: { v in controller.joystick = [Float(v.dx), Float(v.dy)] },
                             onLook: { dx, dy in
                                 controller.look(dx: dx, dy: dy)
                                 if showHint { withAnimation { showHint = false } }
                             },
                             onTap: { p in withAnimation(.easeOut(duration: 0.2)) { controller.tap(at: p) } })
                    .ignoresSafeArea()

                JoystickView(vector: controller.joystick, radius: joystickRadius)
                    .position(joystickCentre)

                VStack(spacing: 10) {
                    if showHint {
                        HintView().padding(.top, topInset + 8).transition(.opacity)
                    }
                    if let message = controller.message {
                        MessageView(text: message)
                            .padding(.top, showHint ? 0 : topInset + 8)
                            .padding(.horizontal, 24)
                            .transition(.opacity)
                    }
                    Spacer()
                    if !controller.prompts.isEmpty {
                        VStack(alignment: .trailing, spacing: 10) {
                            ForEach(controller.prompts) { prompt in
                                PromptButton(prompt: prompt) { controller.perform(prompt) }
                            }
                        }
                        .frame(maxWidth: .infinity, alignment: .trailing)
                        .padding(.trailing, geo.safeAreaInsets.trailing + 24)
                        .padding(.bottom, controller.placard == nil ? geo.safeAreaInsets.bottom + 36 : 8)
                        .transition(.opacity)
                    }
                    if let placard = controller.placard {
                        PlacardView(placard: placard) {
                            withAnimation(.easeOut(duration: 0.2)) { controller.placard = nil }
                        }
                        // Sits above the joystick so both stay usable.
                        .padding(.bottom, geo.safeAreaInsets.bottom + joystickRadius * 2 + 44)
                        .padding(.horizontal, 20)
                        .frame(maxWidth: .infinity, alignment: .center)
                        .transition(.opacity.combined(with: .move(edge: .bottom)))
                    }
                }
                .ignoresSafeArea()
                if !sceneReady, !ProcessInfo.processInfo.arguments.contains("-lab") {
                    LaunchView().transition(.opacity)
                }
            }
            .animation(.easeOut(duration: 0.3), value: controller.prompts)
            .animation(.easeOut(duration: 0.3), value: controller.message)
            .onAppear {
                controller.viewSize = geo.size
                if location == nil, !ProcessInfo.processInfo.arguments.contains("-lab") {
                    let scene = controller.scene
                    location = LocationProvider { scene.observer = $0 }
                    location?.start()
                }
            }
            .onChange(of: geo.size) { _, s in controller.viewSize = s }
            .task {
                #if DEBUG
                await runDebugScript(size: geo.size)
                #endif
                try? await Task.sleep(for: .seconds(8))
                withAnimation { showHint = false }
            }
        }
        .ignoresSafeArea()
        .background(Color(red: 0.09, green: 0.08, blue: 0.06))
    }
}

#if DEBUG
extension MuseumView {
    /// Debug-only launch arguments for checking the museum headlessly in the Simulator:
    ///   -pose "x z yaw pitch"   start somewhere else (degrees; yaw 90 = facing west)
    ///   -walk "seconds jx jy"   hold the joystick for a while (tests collision)
    ///   -tap "x y"              tap a screen point (points) after loading
    func runDebugScript(size: CGSize) async {
        let args = ProcessInfo.processInfo.arguments
        func value(_ key: String) -> [Float]? {
            guard let i = args.firstIndex(of: key), i + 1 < args.count else { return nil }
            return args[i + 1].split(separator: " ").compactMap { Float($0) }
        }
        if let p = value("-pose"), p.count >= 4 {
            controller.debugPose(x: p[0], z: p[1], yawDegrees: p[2], pitchDegrees: p[3], feet: p.count > 4 ? p[4] : 0)
        }
        if let a = value("-act"), !a.isEmpty {
            // Debug: perform scene actions by index after a delay: -act "seconds" with -actions list.
            _ = a
        }
        if let i = args.firstIndex(of: "-do"), i + 1 < args.count {
            for step in args[i + 1].split(separator: ";") {
                let parts = step.split(separator: "@")
                guard parts.count == 2, let at = Double(parts[1]) else { continue }
                let id = String(parts[0])
                Task { @MainActor in
                    try? await Task.sleep(for: .seconds(at))
                    let bits = id.split(separator: ":")
                    if bits.first == "walk", bits.count == 4, let d = Double(bits[1]), let jx = Float(bits[2]), let jy = Float(bits[3]) {
                        controller.joystick = [jx, jy]
                        try? await Task.sleep(for: .seconds(d))
                        controller.joystick = .zero
                    } else if bits.first == "turn", bits.count == 2, let deg = Float(bits[1]) {
                        controller.look(dx: CGFloat(-deg / (0.0045 * 180 / .pi)), dy: 0)
                    } else if bits.first == "tap", bits.count == 3, let x = Double(bits[1]), let y = Double(bits[2]) {
                        controller.tap(at: CGPoint(x: x, y: y))
                    } else if id == "lily" {
                        controller.scene.tapGoldenLily()
                    } else if bits.first == "log" {
                        print("[debug] at \(at)s pose x=\(controller.position.x) z=\(controller.position.y) feet=\(controller.feet) prompts=\(controller.prompts.map(\.id)) msg=\(controller.message ?? "-")")
                    } else {
                        controller.scene.perform(id)
                    }
                }
            }
        }
        if let w = value("-walk"), w.count == 3 {
            try? await Task.sleep(for: .seconds(2))
            controller.joystick = [w[1], w[2]]
            try? await Task.sleep(for: .seconds(Double(w[0])))
            controller.joystick = .zero
        }
        if let t = value("-tap"), t.count == 2 {
            try? await Task.sleep(for: .seconds(3))
            controller.tap(at: CGPoint(x: CGFloat(t[0]), y: CGFloat(t[1])))
        }
        print("[debug] pose x=\(controller.position.x) z=\(controller.position.y) feet=\(controller.feet)")
    }
}
#endif
