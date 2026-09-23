import Metal
import MetalPerformanceShaders
import RealityKit

/// A photographic finish on iOS 26 and later: a soft bloom on bright light (skylights, the eye,
/// daylight on stone), a gentle filmic curve with a warm grade, and a light vignette.
@available(iOS 26.0, *)
struct MuseumPostFX: PostProcessEffect {
    private var bright: MTLComputePipelineState?
    private var composite: MTLComputePipelineState?
    private var half: MTLTexture?
    private var blurred: MTLTexture?

    static let source = """
    #include <metal_stdlib>
    using namespace metal;

    kernel void brightPass(texture2d<half, access::sample> src [[texture(0)]],
                           texture2d<half, access::write> dst [[texture(1)]],
                           uint2 gid [[thread_position_in_grid]]) {
        if (gid.x >= dst.get_width() || gid.y >= dst.get_height()) return;
        constexpr sampler s(filter::linear, address::clamp_to_edge);
        float2 uv = (float2(gid) + 0.5) / float2(dst.get_width(), dst.get_height());
        half3 c = src.sample(s, uv).rgb;
        half l = dot(c, half3(0.2126h, 0.7152h, 0.0722h));
        half k = smoothstep(0.72h, 1.0h, l);
        dst.write(half4(c * k, 1.0h), gid);
    }

    kernel void compositeFX(texture2d<half, access::read> src [[texture(0)]],
                            texture2d<half, access::sample> bloom [[texture(1)]],
                            texture2d<half, access::write> dst [[texture(2)]],
                            uint2 gid [[thread_position_in_grid]]) {
        if (gid.x >= dst.get_width() || gid.y >= dst.get_height()) return;
        constexpr sampler s(filter::linear, address::clamp_to_edge);
        float2 uv = (float2(gid) + 0.5) / float2(dst.get_width(), dst.get_height());
        float3 c = float3(src.read(gid).rgb);
        float3 b = float3(bloom.sample(s, uv).rgb);
        c = c * 1.1 + b * 0.35;
        // Gentle filmic shoulder and a touch of contrast.
        c = c * (1.0 + c * 0.06) / (1.0 + c * 0.16);
        c = mix(c, c * c * (3.0 - 2.0 * c), 0.18);
        // Warm grade: lift the reds, cool the shadows slightly.
        c *= float3(1.015, 1.0, 0.985);
        // Vignette.
        float2 d = uv - 0.5;
        float v = smoothstep(0.95, 0.35, length(d * float2(1.0, 0.85)));
        c *= mix(0.88, 1.0, v);
        dst.write(half4(half3(saturate(c)), 1.0h), gid);
    }
    """

    mutating func prepare(for device: MTLDevice) {
        guard let lib = try? device.makeLibrary(source: Self.source, options: nil),
              let f1 = lib.makeFunction(name: "brightPass"), let f2 = lib.makeFunction(name: "compositeFX") else { return }
        bright = try? device.makeComputePipelineState(function: f1)
        composite = try? device.makeComputePipelineState(function: f2)
    }

    mutating func postProcess(context: borrowing PostProcessEffectContext<MTLCommandBuffer>) {
        let src = context.sourceColorTexture, dst = context.targetColorTexture
        guard let bright, let composite else {
            // Fall back to a straight copy.
            if let blit = context.commandBuffer.makeBlitCommandEncoder() {
                blit.copy(from: src, to: dst)
                blit.endEncoding()
            }
            return
        }
        let w = max(1, src.width / 2), h = max(1, src.height / 2)
        if half == nil || half!.width != w || half!.height != h {
            let d = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba16Float, width: w, height: h, mipmapped: false)
            d.usage = [.shaderRead, .shaderWrite]
            d.storageMode = .private
            half = context.device.makeTexture(descriptor: d)
            blurred = context.device.makeTexture(descriptor: d)
        }
        guard let half, let blurred, let enc = context.commandBuffer.makeComputeCommandEncoder() else { return }
        func dispatch(_ p: MTLComputePipelineState, _ width: Int, _ height: Int) {
            let tg = MTLSize(width: 16, height: 16, depth: 1)
            enc.dispatchThreadgroups(MTLSize(width: (width + 15) / 16, height: (height + 15) / 16, depth: 1), threadsPerThreadgroup: tg)
        }
        enc.setComputePipelineState(bright)
        enc.setTexture(src, index: 0)
        enc.setTexture(half, index: 1)
        dispatch(bright, w, h)
        enc.endEncoding()
        let blur = MPSImageGaussianBlur(device: context.device, sigma: 9)
        blur.encode(commandBuffer: context.commandBuffer, sourceTexture: half, destinationTexture: blurred)
        guard let enc2 = context.commandBuffer.makeComputeCommandEncoder() else { return }
        enc2.setComputePipelineState(composite)
        enc2.setTexture(src, index: 0)
        enc2.setTexture(blurred, index: 1)
        enc2.setTexture(dst, index: 2)
        let tg = MTLSize(width: 16, height: 16, depth: 1)
        enc2.dispatchThreadgroups(MTLSize(width: (dst.width + 15) / 16, height: (dst.height + 15) / 16, depth: 1), threadsPerThreadgroup: tg)
        enc2.endEncoding()
    }
}
