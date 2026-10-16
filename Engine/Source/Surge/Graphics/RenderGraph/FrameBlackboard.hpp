// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RHI/RHIHandle.hpp"
#include "Surge/Graphics/RHI/RHISettings.hpp"
#include "Surge/Graphics/Renderer/Lights.hpp"
#include "Surge/Graphics/HighLevel/Mesh.hpp"
#include "Surge/Graphics/HighLevel/Font.hpp"
#include "Surge/Graphics/Renderer/Text.hpp"
#include "Surge/Graphics/Particles/ParticleRenderData.hpp"
#include "SurgeMath/Frustum.hpp"

#include <glm/glm.hpp>

#define MAX_SHADOW_CASCADE_COUNT 3

namespace Surge
{
    struct FrameUBO
    {
        glm::mat4 View;
        glm::mat4 ViewProjection;        // 64 bytes
        glm::mat4 InverseViewProjection; // 64 bytes
        glm::vec3 CameraPos;             // 12 bytes
        float _pad0;                     // 4 bytes pads to 16-byte boundary
    };
    static_assert(sizeof(FrameUBO) % 16 == 0, "Size of 'FrameUBO' struct must be 16 bytes aligned!");

    // Mirrored in Renderer3D.glsl (set 2, binding 1)
    struct ShadowUBO
    {
        glm::mat4 LightSpaceMatrix;                         // World -> cascade 0 (xy: [-1, 1], z: [0, 1])
        glm::vec4 CascadeScale[MAX_SHADOW_CASCADE_COUNT];   // xyz: cascade 0 -> cascade i, w: world size of one shadow texel
        glm::vec4 CascadeOffset[MAX_SHADOW_CASCADE_COUNT];  // xyz: cascade 0 -> cascade i
        glm::vec4 CascadeEnds;                              // Positive view depth where each cascade ends
        glm::vec2 AtlasTexelSize;                           // 1 / atlas size
        float NormalBias;                                   // In shadow texels
        float CascadeBlend;
        float FadeStart;                                    // View depth where shadows start fading out
        float Distance;                                     // View depth where they are gone
        Uint CascadeCount;
        int FilterSize;                                     // Tent PCF width in texels: 3, 5 or 7
        int DitheredBlend;
        int ShowCascades;
        int Enabled;
        float _pad0;
    };
    static_assert(sizeof(ShadowUBO) % 16 == 0, "Size of 'ShadowUBO' struct must be 16 bytes aligned!");

    enum class ShadowQuality : uint8_t
    {
        LOW,    // 2 cascades of 2048, 3x3 tent PCF (4 taps), dithered cascade blend (Mobile default)
        MEDIUM, // 3 cascades of 2048, 5x5 tent PCF (9 taps)
        HIGH,   // 3 cascades of 2048, 7x7 tent PCF (16 taps)
        ULTRA   // 3 cascades of 4096, 7x7 tent PCF (16 taps) (Desktop default)
    };

    // Directional light cascaded shadow maps, every cascade is a region of one D16 atlas rendered in a single render pass
    struct ShadowSettings
    {
        static constexpr Uint MIN_RESOLUTION = 512;
        static constexpr Uint MAX_RESOLUTION = 4096;

        bool Enable = true;
        ShadowQuality Quality = ShadowQuality::LOW;

        // Driven by Quality via ApplyQualityPreset(), can be tweaked individually afterwards
        int CascadeCount = 2;        // Every cascade draws its casters again
        Uint Resolution = 2048;      // Per cascade, the atlas is CascadeCount * Resolution wide
        int FilterSize = 3;          // Tent PCF width in texels: 3 (4 taps), 5 (9 taps) or 7 (16 taps)
        bool DitheredBlend = true;   // Cascade blend picks one cascade per pixel (noise) instead of sampling both

        float Distance = 50.0f;      // View depth covered by the cascades, shadows fade out over the last FadeRange of it
        float FadeRange = 0.1f;
        float CascadeSplitLambda = 0.9f;
        float CascadeBlend = 0.1f;   // Fraction of each cascade blended into the next one, hides the resolution jump
        bool ShowCascades = false;

        float NormalBias = 1.0f;     // Receiver offset along its normal in shadow texels, so it scales with each cascade
        float DepthBiasConstant = 1.25f;
        float DepthBiasClamp = 0.0f;
        float DepthBiasSlope = 1.75f;

        void ApplyQualityPreset(ShadowQuality quality)
        {
            Quality = quality;
            switch(quality)
            {
                case ShadowQuality::LOW:    CascadeCount = 2; Resolution = 2048; FilterSize = 3; DitheredBlend = true;  break;
                case ShadowQuality::MEDIUM: CascadeCount = 3; Resolution = 2048; FilterSize = 5; DitheredBlend = false; break;
                case ShadowQuality::HIGH:   CascadeCount = 3; Resolution = 2048; FilterSize = 7; DitheredBlend = false; break;
                case ShadowQuality::ULTRA:  CascadeCount = 3; Resolution = 4096; FilterSize = 7; DitheredBlend = false; break;
            }
        }

        static ShadowSettings PlatformDefault()
        {
            ShadowSettings settings;
#ifdef SURGE_PLATFORM_ANDROID
            settings.ApplyQualityPreset(ShadowQuality::LOW);
            settings.Distance = 50.0f;
#else
            settings.ApplyQualityPreset(ShadowQuality::ULTRA);
            settings.Distance = 150.0f;
#endif
            return settings;
        }
    };

    struct Environnment
    {
        bool HasEnvironment = false;
        bool ShowSky = true; // false: only the ambient below lights the scene, the background stays clear

        // Sky
        float Elevation = 30.0f; // In degrees
        float Azimuth = 0.0f;   // In degrees
        float Turbidity = 2.0f;
        float Exposure = 0.02f;
        float SunIntensity = 5.0f;
        bool EnableSunDisk = true;

        // GI
        glm::vec3 SkyAmbient { 0.35f, 0.55f, 0.90f };
        glm::vec3 HorizonAmbient { 0.45f, 0.52f, 0.60f };
        glm::vec3 GroundAmbient { 0.12f, 0.11f, 0.10f };
    };

    enum class AOQuality : uint8_t
    {
        LOW,    // Half res, 1 slice x 3 steps, derivative normals (Mobile default)
        MEDIUM, // Half res, 2 slices x 3 steps
        HIGH,   // Full res, 3 slices x 4 steps
        ULTRA   // Full res, 4 slices x 8 steps, wide blur (Desktop default)
    };

    // GTAO (Ground Truth Ambient Occlusion), computed from depth only
    // Applied as a multiply on the lit scene color in PostProcess
    struct AmbientOcclusionSettings
    {
        bool Enable = true;
        AOQuality Quality = AOQuality::LOW;

        // Driven by Quality via ApplyQualityPreset(), can be tweaked individually afterwards
        float ResolutionScale = 0.5f; // AO render target size relative to the screen (0.25 - 1.0)
        int SliceCount = 1;
        int StepsPerSlice = 3;
        int BlurRadius = 2;           // In AO texels, 0 disables the blur
        bool AccurateNormals = false; // 4 tap normal reconstruction instead of screen space derivatives

        float Radius = 0.75f;      // World space units
        float Intensity = 1.0f;    // Final visibility power
        float FalloffRange = 0.6f; // Fraction of Radius over which occluders fade out
        float MaxDistance = 80.0f; // AO fades out completely at this view distance
        float BlurSharpness = 8.0f;

        void ApplyQualityPreset(AOQuality quality)
        {
            Quality = quality;
            switch(quality)
            {
                case AOQuality::LOW:    ResolutionScale = 0.5f; SliceCount = 1; StepsPerSlice = 3; BlurRadius = 2; AccurateNormals = false; break;
                case AOQuality::MEDIUM: ResolutionScale = 0.5f; SliceCount = 2; StepsPerSlice = 3; BlurRadius = 3; AccurateNormals = true;  break;
                case AOQuality::HIGH:   ResolutionScale = 1.0f; SliceCount = 3; StepsPerSlice = 4; BlurRadius = 3; AccurateNormals = true;  break;
                case AOQuality::ULTRA:  ResolutionScale = 1.0f; SliceCount = 4; StepsPerSlice = 8; BlurRadius = 4; AccurateNormals = true;  break;
            }
        }

        static AmbientOcclusionSettings PlatformDefault()
        {
            AmbientOcclusionSettings settings;
#ifdef SURGE_PLATFORM_ANDROID
            // 4 extra render passes + a stored full-res depth buffer, opt-in on mobile
            settings.ApplyQualityPreset(AOQuality::LOW);
            settings.Enable = false;
#else
            settings.ApplyQualityPreset(AOQuality::ULTRA);
#endif
            return settings;
        }
    };

    enum class BloomQuality : uint8_t
    {
        LOW,    // Fragment, 3 mips, dual filter (5/8 taps), bilinear composite (Mobile default)
        MEDIUM, // Fragment, 5 mips, 13 tap down + 9 tap tent up, anti flicker
        HIGH,   // Compute, 6 mips, 13 tap down + 9 tap tent up, anti flicker
        ULTRA   // Compute, 8 mips, 13 tap down + 9 tap tent up, anti flicker (Desktop default)
    };

    // Physically based style bloom (CoD: Advanced Warfare / Unity URP / Hazel), mip chain starts at half resolution
    // Fragment path renders each mip with a renderpass (tile friendly, keeps framebuffer compression on mobile, R11G11B10)
    // Compute path writes storage images (RGBA16F), used on desktop
    struct BloomSettings
    {
        static constexpr int MIN_MIP_COUNT = 2;
        static constexpr int MAX_MIP_COUNT = 8;

        bool Enable = true;
        BloomQuality Quality = BloomQuality::LOW;

        // Driven by Quality via ApplyQualityPreset(), can be tweaked individually afterwards
        bool UseCompute = false;           // Falls back to the fragment path if the device queue has no compute support
        int MipCount = 3;                  // 2 - 8, clamped further so the smallest mip stays >= 2px
        bool HighQualityFilter = false;    // 13 tap downsample + 9 tap tent upsample, otherwise ARM dual filter
        bool AntiFlicker = false;          // Karis average on the first downsample, kills fireflies from tiny HDR highlights
        bool HighQualityComposite = false; // 9 tap tent when upscaling the half res bloom in PostProcess

        float Threshold = 1.0f;  // Linear HDR brightness where bloom starts
        float Knee = 0.5f;       // Soft threshold, fraction of Threshold
        float Intensity = 0.8f;
        float Scatter = 0.7f;    // How far the glow spreads (mix factor between mips)
        float ClampValue = 65000.0f; // Max HDR value fed into bloom
        glm::vec3 Tint = glm::vec3(1.0f);

        void ApplyQualityPreset(BloomQuality quality)
        {
            Quality = quality;
            switch(quality)
            {
                case BloomQuality::LOW:    UseCompute = false; MipCount = 3; HighQualityFilter = false; AntiFlicker = false; HighQualityComposite = false; break;
                case BloomQuality::MEDIUM: UseCompute = false; MipCount = 5; HighQualityFilter = true;  AntiFlicker = true;  HighQualityComposite = true;  break;
                case BloomQuality::HIGH:   UseCompute = true;  MipCount = 6; HighQualityFilter = true;  AntiFlicker = true;  HighQualityComposite = true;  break;
                case BloomQuality::ULTRA:  UseCompute = true;  MipCount = 8; HighQualityFilter = true;  AntiFlicker = true;  HighQualityComposite = true;  break;
            }
        }

        static BloomSettings PlatformDefault()
        {
            BloomSettings settings;
#ifdef SURGE_PLATFORM_ANDROID
            settings.ApplyQualityPreset(BloomQuality::LOW);
#else
            settings.ApplyQualityPreset(BloomQuality::ULTRA);
#endif
            return settings;
        }
    };

    // Resolution of the 3D scene (main pass, AO, bloom) relative to the output, UI and post-processing stay at full resolution
    struct RenderScaleSettings
    {
        static constexpr float MIN_SCALE = 0.5f;
        static constexpr float MAX_SCALE = 1.0f;

        static float PlatformDefault()
        {
#ifdef SURGE_PLATFORM_ANDROID
            return 0.8f;
#else
            return 1.0f;
#endif
        }
    };

    struct PostProcessSettings
    {
        bool EnableFXAA = true;
        glm::vec3 OutlineColor = glm::vec3(1.0f, 0.6f, 0.1f);
        float OutlineThickness = 1.1f;
        float VignetteIntensity = 0.0f;
        float VignetteSoftness = 0.25f;
        float Grain = 0.0f;
        bool ShowAOOnly = false; // Debug view, outputs the upsampled AO buffer
    };

    // CPU-side submit commands
    // Pushed by Renderer::Submit*(), consumed by nodes in Execute()

    struct MeshSubmitCmd  // Pushed by Renderer::SubmitMesh()
    {
        glm::mat4 Transform;
        const Mesh* Mesh_;
        bool DropShadow;
    };

    struct OutlineSubmitCmd // Pushed by Renderer::SubmitOutlinedMesh()
    {
        glm::mat4 Transform;
        const Mesh* Mesh_;
    };

    struct LightSubmitCmd // Pushed by Renderer::SubmitLight()
    {
        Light GPULight; // Pre-converted from LightComponent at submit time
    };

    // Forward lighting budget: every lit pixel loops over these, so only the most relevant lights reach the shader
    struct LightingSettings
    {
#ifdef SURGE_PLATFORM_ANDROID
        Uint MaxLights = 8;
#else
        Uint MaxLights = 64;
#endif
    };

    struct QuadSubmitCmd // Pushed by Renderer::SubmitQuad()
    {
        glm::mat4 Transform;
        glm::vec4 Color;
        ImageHandle Texture;
        bool Billboard;
    };

    struct LineSubmitCmd // Pushed by Renderer::SubmitLine()
    {
        glm::vec3 P0;
        glm::vec3 P1;
        glm::vec4 Color;
    };

    struct TextSubmitCmd // Pushed by Renderer::SubmitText()
    {
        glm::mat4 Transform;
        String Text;
        glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };

        float MaxWidth = 0.0f; // 0.0 means no wrapping

        float LetterSpacing = 0.0f;
        float LineSpacing = 0.0f;
        TextAlignment Alignment = TextAlignment::LEFT;
        TextVerticalAlignment VerticalAlignment = TextVerticalAlignment::CENTER;

        bool Italic = false;
        bool Underline = false;
        bool Billboard = false;

        // Shadows
        bool EnableShadow = false;
        glm::vec2 ShadowOffset = { 0.0f, 0.0f };
        glm::vec4 ShadowColor = { 0.0f, 0.0f, 0.0f, 0.0f };

        const Font* FontAsset;
    };

    // Screen space UI sprite, pushed by UI::Manager (pixels, (0,0) = top left of the UI target)
    struct UISpriteCmd
    {
        glm::vec2 Min;
        glm::vec2 Max;
        glm::vec2 UVMin = { 0.0f, 0.0f }; // Sub-rect of Texture, lets 9-slice and filled images reuse the same quad path
        glm::vec2 UVMax = { 1.0f, 1.0f };
        glm::vec4 Color;
        ImageHandle Texture;
        float CornerRadius = 0.0f; // Pixels, rounded in UI.glsl (signed distance), 0 = sharp
    };

    // Screen space UI draw order: sprites and text are interleaved in hierarchy order
    struct UIDrawItem
    {
        bool IsText;
        uint16_t ClipIndex; // UI_NO_CLIP or index + 1 into UIClipRects, applied on the CPU (no scissor, batches stay intact)
        Uint Index;         // Into UITextList or UISpriteList
    };
    static constexpr uint16_t UI_NO_CLIP = 0;

    struct FrameBlackboard
    {
        // Written by Renderer::BeginFrame()
        float ScreenWidth, ScreenHeight;
        float RenderScale = RenderScaleSettings::PlatformDefault();
        glm::vec3 CameraPosition;
        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewProjection;
        glm::mat4 InverseViewProjection;
        glm::vec2 CameraNearFarPlane;
        glm::vec3 CameraRight;
        glm::vec3 CameraUp;
        Frustum CameraFrustum; // Of ViewProjection, culls the particle systems before they submit

        bool HasDirectionalLight = false;
        glm::vec3 DirectionalLightDir; //Set by Renderer.cpp

        Uint FrameIndex;
        BufferHandle FrameUBOs[RHISettings::FRAMES_IN_FLIGHT];
        BufferHandle ShadowUBOs[RHISettings::FRAMES_IN_FLIGHT];
        ImageHandle WhiteImage;
        ImageHandle ParticleDefaultImage; // Soft round particle, used by emitters without a texture

        ImageHandle MainPassColorImage;
        ImageHandle MainPassDepthImage;
        ImageHandle OutlineMask;
        ImageHandle AOImage;       // R8, AO resolution, 1.0 = unoccluded
        ImageHandle AOLinearDepth; // R32F, AO resolution, positive view space depth
        ImageHandle BloomImage;    // Half res, final (top) mip of the bloom upsample chain
        ImageHandle UIOverlayImage;
        ImageHandle FinalImage;
        ImageHandle ShadowAtlas;     // D16, cascades side by side (ShadowPass)
        Uint ShadowAtlasVersion = 0; // Bumped when the atlas is recreated (resolution/cascade count change), samplers rewrite their descriptors

        FramebufferHandle OutlineFramebuffer;
        FramebufferHandle PostProcessFramebuffer;
        FramebufferHandle UIOverlayFramebuffer;
        FramebufferHandle MainPassFramebuffer;

        SamplerHandle DefaultSampler;
        SamplerHandle TextSampler;
        SamplerHandle PointClampSampler;
        SamplerHandle LinearClampSampler;

        PipelineHandle MaterialPipeline; // TODO: Remove this (It is currently GeometryPassPipeline(set by GeometryPass::Setup))

        //Shadow Settings
        ShadowSettings ShadowSettings_ = ShadowSettings::PlatformDefault();

        // GI & skybox
        Environnment Env;

        LightingSettings Lighting;
        ParticleSettings Particles;
        ParticleStats ParticleStatistics; // Written by the scene each frame

        // Screen Space options
        PostProcessSettings PostProcessSettings_;
        AmbientOcclusionSettings AOSettings = AmbientOcclusionSettings::PlatformDefault();
        BloomSettings Bloom = BloomSettings::PlatformDefault();

        // CMD lists
        Vector<MeshSubmitCmd> MeshList;
        Vector<LightSubmitCmd> LightList;
        Vector<QuadSubmitCmd> QuadList;
        Vector<LineSubmitCmd> LineList;
        Vector<TextSubmitCmd> TextList;
        Vector<OutlineSubmitCmd> OutlineList;
        Vector<ParticleRecord> ParticleList;            // Renderer::SubmitParticles
        Vector<ParticleEmitterCmd> ParticleEmitterList; // One per submitted emitter, records are contiguous per emitter

        // UI
        bool ShowUI = true;
        glm::vec2 UIResolution = { 0.0f, 0.0f }; // Size the UI was laid out at, 0 = ScreenWidth/Height (Editor views share one layout)
        Vector<UISpriteCmd> UISpriteList;
        Vector<TextSubmitCmd> UITextList;
        Vector<UIDrawItem> UIDrawOrder;
        Vector<glm::vec4> UIClipRects; // (minX, minY, maxX, maxY) pixels

        // Size of the scene targets (main pass, AO, bloom) for a given output size
        glm::uvec2 GetSceneSize(Uint outputWidth, Uint outputHeight) const
        {
            const float scale = glm::clamp(RenderScale, RenderScaleSettings::MIN_SCALE, RenderScaleSettings::MAX_SCALE);
            return glm::max(glm::uvec2(glm::vec2(static_cast<float>(outputWidth), static_cast<float>(outputHeight)) * scale + 0.5f), glm::uvec2(1));
        }

        void ClearLists()
        {
            MeshList.clear();
            LightList.clear();
            TextList.clear();
            QuadList.clear();
            LineList.clear();
            OutlineList.clear();
            ParticleList.clear();
            ParticleEmitterList.clear();

            UISpriteList.clear();
            UITextList.clear();
            UIDrawOrder.clear();
            UIClipRects.clear();
        }
    };
}
