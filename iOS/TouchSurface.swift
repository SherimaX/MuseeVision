import SwiftUI
import UIKit

/// A full-screen multitouch layer: a touch that starts in the joystick's circle drives the
/// joystick; any other touch drags the view; a short touch that barely moves is a tap.
/// UIKit is used so walking and looking work at the same time with two thumbs.
struct TouchSurface: UIViewRepresentable {
    var joystickCentre: CGPoint
    var joystickRadius: CGFloat
    var onJoystick: (CGVector) -> Void
    var onLook: (CGFloat, CGFloat) -> Void
    var onTap: (CGPoint) -> Void

    func makeUIView(context: Context) -> TouchView {
        let v = TouchView()
        v.isMultipleTouchEnabled = true
        v.backgroundColor = .clear
        return v
    }

    func updateUIView(_ v: TouchView, context: Context) {
        v.joystickCentre = joystickCentre
        v.joystickRadius = joystickRadius
        v.onJoystick = onJoystick
        v.onLook = onLook
        v.onTap = onTap
    }

    final class TouchView: UIView {
        var joystickCentre: CGPoint = .zero
        var joystickRadius: CGFloat = 60
        var onJoystick: (CGVector) -> Void = { _ in }
        var onLook: (CGFloat, CGFloat) -> Void = { _, _ in }
        var onTap: (CGPoint) -> Void = { _ in }

        private var joystickTouch: UITouch?
        private struct Look { var start: CGPoint; var last: CGPoint; var time: TimeInterval; var moved: Bool }
        private var looks: [ObjectIdentifier: Look] = [:]

        override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
            for t in touches {
                let p = t.location(in: self)
                let d = hypot(p.x - joystickCentre.x, p.y - joystickCentre.y)
                if joystickTouch == nil && d < joystickRadius * 1.6 {
                    joystickTouch = t
                    updateJoystick(p)
                } else {
                    looks[ObjectIdentifier(t)] = Look(start: p, last: p, time: t.timestamp, moved: false)
                }
            }
        }

        override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
            for t in touches {
                let p = t.location(in: self)
                if t === joystickTouch {
                    updateJoystick(p)
                } else if var l = looks[ObjectIdentifier(t)] {
                    if hypot(p.x - l.start.x, p.y - l.start.y) > 8 { l.moved = true }
                    if l.moved { onLook(p.x - l.last.x, p.y - l.last.y) }
                    l.last = p
                    looks[ObjectIdentifier(t)] = l
                }
            }
        }

        override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
            finish(touches, cancelled: false)
        }

        override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
            finish(touches, cancelled: true)
        }

        private func finish(_ touches: Set<UITouch>, cancelled: Bool) {
            for t in touches {
                if t === joystickTouch {
                    joystickTouch = nil
                    onJoystick(.zero)
                } else if let l = looks.removeValue(forKey: ObjectIdentifier(t)) {
                    if !cancelled && !l.moved && t.timestamp - l.time < 0.35 { onTap(l.start) }
                }
            }
        }

        private func updateJoystick(_ p: CGPoint) {
            var v = CGVector(dx: (p.x - joystickCentre.x) / joystickRadius, dy: -(p.y - joystickCentre.y) / joystickRadius)
            let len = hypot(v.dx, v.dy)
            if len > 1 { v = CGVector(dx: v.dx / len, dy: v.dy / len) }
            onJoystick(v)
        }
    }
}
