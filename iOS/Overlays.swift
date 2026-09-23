import SwiftUI

/// The on-screen joystick (drawn only; touches are handled by TouchSurface).
struct JoystickView: View {
    var vector: SIMD2<Float>
    var radius: CGFloat

    var body: some View {
        ZStack {
            Circle()
                .fill(.ultraThinMaterial)
                .overlay(Circle().stroke(.white.opacity(0.35), lineWidth: 1))
                .frame(width: radius * 2, height: radius * 2)
            Circle()
                .fill(.white.opacity(0.85))
                .shadow(color: .black.opacity(0.25), radius: 4, y: 1)
                .frame(width: radius * 0.8, height: radius * 0.8)
                .offset(x: CGFloat(vector.x) * radius * 0.6, y: -CGFloat(vector.y) * radius * 0.6)
        }
        .allowsHitTesting(false)
    }
}

/// A small glass placard with the work's title, artist, year and collection.
struct PlacardView: View {
    let placard: WalkController.Placard
    var onClose: () -> Void

    var body: some View {
        let content = VStack(alignment: .leading, spacing: 4) {
            Text(placard.title)
                .font(.custom("Didot", size: 20))
                .fixedSize(horizontal: false, vertical: true)
            if let detail = placard.detail {
                Text(detail).font(.system(size: 13, weight: .medium)).foregroundStyle(.secondary)
            }
            Text([placard.artist, placard.year].filter { !$0.isEmpty }.joined(separator: " · "))
                .font(.system(size: 14))
            if !placard.collection.isEmpty {
                Text(placard.collection).font(.system(size: 12)).foregroundStyle(.secondary)
            }
        }
        .padding(.horizontal, 18)
        .padding(.vertical, 14)
        .frame(maxWidth: 340, alignment: .leading)
        .contentShape(Rectangle())
        .onTapGesture(perform: onClose)

        if #available(iOS 26.0, *) {
            content.glassEffect(.regular, in: .rect(cornerRadius: 18))
        } else {
            content
                .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 18))
                .overlay(RoundedRectangle(cornerRadius: 18).stroke(.white.opacity(0.3), lineWidth: 1))
        }
    }
}

struct HintView: View {
    var body: some View {
        Text("Joystick to walk · drag to look · tap a painting")
            .font(.system(size: 13, weight: .medium))
            .padding(.horizontal, 14)
            .padding(.vertical, 8)
            .background(.ultraThinMaterial, in: Capsule())
            .allowsHitTesting(false)
    }
}

/// A short line from the museum (the pond rising, the car coming).
struct MessageView: View {
    let text: String
    var body: some View {
        Text(text)
            .font(.system(size: 14, weight: .medium))
            .multilineTextAlignment(.center)
            .padding(.horizontal, 16)
            .padding(.vertical, 10)
            .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 14))
            .allowsHitTesting(false)
    }
}

/// A glass button for what can be done here (the elevator's floors).
struct PromptButton: View {
    let prompt: ActionPrompt
    let action: () -> Void
    var body: some View {
        let label = Label(prompt.title, systemImage: prompt.symbol)
            .font(.system(size: 16, weight: .semibold))
            .padding(.horizontal, 18)
            .padding(.vertical, 12)
        if #available(iOS 26.0, *) {
            Button(action: action) { label }
                .buttonStyle(.plain)
                .glassEffect(.regular.interactive(), in: Capsule())
        } else {
            Button(action: action) { label }
                .buttonStyle(.plain)
                .background(.ultraThinMaterial, in: Capsule())
                .overlay(Capsule().stroke(.white.opacity(0.3), lineWidth: 1))
        }
    }
}

/// The logo on the museum's dark bronze while the building appears.
struct LaunchView: View {
    var body: some View {
        ZStack {
            LinearGradient(colors: [Color(red: 0.165, green: 0.141, blue: 0.102), Color(red: 0.047, green: 0.043, blue: 0.031)],
                           startPoint: .top, endPoint: .bottom)
            VStack(spacing: 18) {
                Image("LogoMark").resizable().scaledToFit().frame(width: 220, height: 220)
                Text("Musée Vision").font(.custom("Didot", size: 40)).foregroundStyle(Color(red: 0.945, green: 0.922, blue: 0.875))
                Text("CLASSICAL BONES · MODERN LIGHT · FUTURE GLASS")
                    .font(.system(size: 11, weight: .medium)).kerning(2)
                    .foregroundStyle(Color(red: 0.788, green: 0.635, blue: 0.4))
            }
        }
        .ignoresSafeArea()
        .allowsHitTesting(false)
    }
}
