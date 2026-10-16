// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ShadowPass.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Profiler.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"

#include "SurgeMath/Frustum.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace Surge
{
    namespace
    {
        constexpr Uint INITIAL_INSTANCE_CAPACITY = 1024;

        // (Rid) Every cascade's ortho box is grown by this many texels per side, so the PCF footprint (up to 4 texels for 7x7) and the
        // normal offset never read outside the cascade's region of the atlas (Renderer3D.glsl clamps as well)
        constexpr float CASCADE_BORDER_TEXELS = 8.0f;

        // How far toward the light casters are looked for, anything further is pancaked onto the near plane (Shadow.glsl)
        constexpr float MAX_CASTER_DISTANCE = 500.0f;

        // The fitted near plane moves in steps, so the depth range (and with it the depth bias) doesn't change every frame
        constexpr float NEAR_PLANE_STEP = 8.0f;

        // Same test as Frustum::IntersectsAABB, on a world space box computed once per caster for every cascade
        bool IntersectsBox(const Frustum& frustum, const glm::vec3& center, const glm::vec3& extents)
        {
            for(const glm::vec4& plane : frustum.Planes)
            {
                const glm::vec3 n = glm::vec3(plane);
                if(glm::dot(n, center) + plane.w < -glm::dot(extents, glm::abs(n)))
                    return false;
            }
            return true;
        }
    }

    void ShadowPass::Setup(GraphicsRHI* rhi, FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::Setup");
        mRHI = rhi;

        /*
        *  (Rid) Shadow map sizes for D16_UNORM (2 bytes per pixel), per cascade
        *  1024 * 0.5 = 0512 = 0.5 MB
        *  1024 * 1   = 1024 = 2   MB
        *  1024 * 2   = 2048 = 8   MB <- Our Default for mobile (2 cascades = 16 MB atlas)
        *  1024 * 3   = 3072 = 18  MB
        *  1024 * 4   = 4096 = 32  MB <- Our Default for PC (3 cascades = 96 MB atlas)
        *
        *  Multiply these MB values by 2 to get size for D32_SFLOAT
        */
        const ShadowSettings& settings = blackBoard.ShadowSettings_;
        mAtlasCascades = static_cast<Uint>(glm::clamp(settings.CascadeCount, 1, MAX_SHADOW_CASCADE_COUNT));
        mResolution = ComputeResolution(settings.Resolution, mAtlasCascades);

        ImageDesc depthDesc = {};
        depthDesc.Width = mResolution * mAtlasCascades;
        depthDesc.Height = mResolution;
        depthDesc.Format = ImageFormat::D16_UNORM;
        depthDesc.Usage = ImageUsage::DEPTH_ATTACHMENT | ImageUsage::SAMPLED;
        depthDesc.GenerateImGuiID = true;
        depthDesc.DebugName = "ShadowAtlas";
        blackBoard.ShadowAtlas = mRHI->CreateImage(depthDesc);

        FramebufferAttachment depthAttachment = {};
        depthAttachment.Handle = blackBoard.ShadowAtlas;
        depthAttachment.Load = LoadOp::CLEAR;
        depthAttachment.Store = StoreOp::STORE;

        FramebufferDesc fbDesc = {};
        fbDesc.ColorAttachmentCount = 0;
        fbDesc.DepthAttachment = depthAttachment;
        fbDesc.HasDepth = true;
        fbDesc.Width = depthDesc.Width;
        fbDesc.Height = depthDesc.Height;
        fbDesc.DebugName = "ShadowPass Framebuffer";
        mFramebuffer = mRHI->CreateFramebuffer(fbDesc);

        PipelineDesc desc = {};
        desc.Shader_ = Core::GetRenderer()->GetShaderManager().Get("Shadow.glsl");
        desc.Raster.Topo = Topology::TRIANGLE_LIST;
        desc.Raster.Polygon = PolygonMode::FILL;
        desc.Raster.Cull = CullMode::BACK; // Receivers carry the normal offset, so contact points don't float and single sided casters still cast

        desc.Raster.DepthBiasEnable = true;
        desc.Stencil.Enable = false;
        desc.Depth.TestEnable = true;
        desc.Depth.WriteEnable = true;
        desc.Depth.Op = CompareOp::LESS_OR_EQUAL;
        desc.Blend.Enable = false;

        desc.TargetSwapchain = false;
        desc.TargetFramebuffer = mFramebuffer;
        desc.DebugName = "ShadowPass";
        mPipeline = mRHI->CreatePipeline(desc);

        mImageWrites.push_back(blackBoard.ShadowAtlas);

        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            EnsureInstanceCapacity(i, INITIAL_INSTANCE_CAPACITY);

        mInstanceSet = mRHI->CreateDescriptorSet(mPipeline, DescriptorSetSlot::ZERO, DescriptorUpdateFrequency::DYNAMIC, "ShadowInstances [Set0]");
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
        {
            DescriptorWrite write = {};
            write.Binding = 0;
            write.Type = DescriptorType::STORAGE_BUFFER;
            write.Buffer = mInstanceBuffers[i];
            write.BufferRange = sizeof(glm::mat4) * mInstanceCapacity[i];
            mRHI->UpdateDescriptorSet(mInstanceSet, &write, 1, i);
        }

        mCasters.reserve(INITIAL_INSTANCE_CAPACITY);
        mDrawItems.reserve(INITIAL_INSTANCE_CAPACITY);
        mInstanceData.reserve(INITIAL_INSTANCE_CAPACITY);
    }

    Uint ShadowPass::ComputeResolution(Uint requested, Uint cascadeCount) const
    {
        // The atlas is cascadeCount * resolution wide, halved until it fits the device (Vulkan guarantees 4096, most GPUs have 8192+)
        const Uint maxDimension = mRHI->GetMaxImageDimension2D();
        Uint resolution = glm::clamp(requested, ShadowSettings::MIN_RESOLUTION, ShadowSettings::MAX_RESOLUTION);

        while(resolution * cascadeCount > maxDimension && resolution > ShadowSettings::MIN_RESOLUTION)
            resolution /= 2; // (Rid) Half until it fits; should not reach

        return resolution;
    }

    void ShadowPass::UpdateAtlas(FrameBlackboard& blackBoard, Uint cascadeCount)
    {
        const Uint resolution = ComputeResolution(blackBoard.ShadowSettings_.Resolution, cascadeCount);
        if(resolution == mResolution && cascadeCount == mAtlasCascades)
            return;

        mResolution = resolution;
        mAtlasCascades = cascadeCount;

        // Same handle (RenderGraph barriers stay valid), the old image and framebuffer go through the deletion queue
        mRHI->ResizeImage(blackBoard.ShadowAtlas, resolution * cascadeCount, resolution);
        mRHI->ResizeFramebuffer(mFramebuffer, resolution * cascadeCount, resolution, false);
        blackBoard.ShadowAtlasVersion++; // GeometryPass rewrites its shadow descriptors (new image view)
    }

    void ShadowPass::EnsureInstanceCapacity(Uint frameIndex, Uint instanceCount)
    {
        if(instanceCount <= mInstanceCapacity[frameIndex] && !mInstanceBuffers[frameIndex].IsNull())
            return;

        Uint capacity = std::max(mInstanceCapacity[frameIndex], INITIAL_INSTANCE_CAPACITY);
        while(capacity < instanceCount)
            capacity *= 2;

        if(!mInstanceBuffers[frameIndex].IsNull())
            mRHI->DestroyBuffer(mInstanceBuffers[frameIndex]);

        BufferDesc desc = {};
        desc.Usage = BufferUsage::STORAGE;
        desc.HostVisible = true;
        desc.Size = sizeof(glm::mat4) * capacity;
        desc.DebugName = std::format("ShadowInstances Frame: {}", frameIndex);
        mInstanceBuffers[frameIndex] = mRHI->CreateBuffer(desc);

        const bool resized = mInstanceCapacity[frameIndex] != 0;
        mInstanceCapacity[frameIndex] = capacity;
        if(resized)
        {
            DescriptorWrite write = {};
            write.Binding = 0;
            write.Type = DescriptorType::STORAGE_BUFFER;
            write.Buffer = mInstanceBuffers[frameIndex];
            write.BufferRange = sizeof(glm::mat4) * capacity;
            mRHI->UpdateDescriptorSet(mInstanceSet, &write, 1, frameIndex);
        }
    }

    void ShadowPass::GatherCasters(const FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::GatherCasters");

        // World transform + world AABB of every shadow casting submesh, computed once and shared by all cascades
        mCasters.clear();
        for(const MeshSubmitCmd& cmd : blackBoard.MeshList)
        {
            if(!cmd.DropShadow)
                continue;

            const Vector<Submesh>& submeshes = cmd.Mesh_->GetSubmeshes();
            for(Uint i = 0; i < static_cast<Uint>(submeshes.size()); i++)
            {
                const Submesh& submesh = submeshes[i];
                const glm::mat4 transform = cmd.Transform * submesh.Transform;

                // Arvo's method, like Frustum::IntersectsAABB
                const glm::vec3 localCenter = (submesh.BoundingBox.Min + submesh.BoundingBox.Max) * 0.5f;
                const glm::vec3 localExtents = (submesh.BoundingBox.Max - submesh.BoundingBox.Min) * 0.5f;
                const glm::mat3 basis = glm::mat3(transform);
                const glm::vec3 center = glm::vec3(transform * glm::vec4(localCenter, 1.0f));
                const glm::vec3 extents = glm::abs(basis[0]) * localExtents.x + glm::abs(basis[1]) * localExtents.y + glm::abs(basis[2]) * localExtents.z;

                mCasters.push_back({ transform, center, extents, cmd.Mesh_, i });
            }
        }
    }

    // (Rid)Visualization: https://gemini.google.com/share/13d9f8dfbf32
    void ShadowPass::BuildCascades(const FrameBlackboard& bb, Uint cascadeCount)
    {
        SURGE_PROFILE_FUNC("ShadowPass::BuildCascades");

        constexpr Uint NUM_FRUSTUM_CORNERS = 8;
        const ShadowSettings& settings = bb.ShadowSettings_;
        const float resolution = static_cast<float>(mResolution);

        // Cascades only cover [near, Distance] instead of the whole view frustum, that's where the texels are worth spending
        const float minZ = bb.CameraNearFarPlane.x;
        const float maxZ = bb.CameraNearFarPlane.y;
        const float clipRange = maxZ - minZ;
        mShadowDistance = std::clamp(settings.Distance, minZ, maxZ);
        const float shadowRange = mShadowDistance - minZ;
        const float ratio = mShadowDistance / minZ;

        // Practical split scheme: blend of logarithmic and uniform splits, as fractions of the view frustum's depth
        std::array<float, MAX_SHADOW_CASCADE_COUNT> splits = {};
        for(Uint i = 0; i < cascadeCount; i++)
        {
            const float p = (i + 1.0f) / static_cast<float>(cascadeCount);
            const float log = minZ * glm::pow(ratio, p);
            const float uniform = minZ + shadowRange * p;
            const float d = settings.CascadeSplitLambda * (log - uniform) + uniform;
            splits[i] = (d - minZ) / clipRange;
            mCascadeEnds[i] = d;
        }

        // Reconstruct master view frustum in world space
        glm::vec4 frustumCorners[NUM_FRUSTUM_CORNERS] =
        {
            // Near Plane
            { 1.0f,  1.0f, 0.0f, 1.0f },
            {-1.0f,  1.0f, 0.0f, 1.0f },
            { 1.0f, -1.0f, 0.0f, 1.0f },
            {-1.0f, -1.0f, 0.0f, 1.0f },

            // Far Plane
            { 1.0f,  1.0f, 1.0f, 1.0f },
            {-1.0f,  1.0f, 1.0f, 1.0f },
            { 1.0f, -1.0f, 1.0f, 1.0f },
            {-1.0f, -1.0f, 1.0f, 1.0f },
        };

        for(glm::vec4& c : frustumCorners)
        {
            const glm::vec4 inv = bb.InverseViewProjection * c;
            c = inv / inv.w;
        }

        // Every cascade shares the light's rotation, so a cascade is a scale + offset of cascade 0 (Renderer3D.glsl relies on it)
        const glm::vec3 lightDir = glm::normalize(bb.DirectionalLightDir);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        if(glm::abs(lightDir.y) > 0.99f)
            upVector = glm::vec3(0.0f, 0.0f, 1.0f);

        mInstanceData.clear();
        float lastSplitDist = 0.0f;
        for(Uint cascade = 0; cascade < cascadeCount; cascade++)
        {
            const float splitDist = splits[cascade];
            glm::vec4 sliceCorners[NUM_FRUSTUM_CORNERS]; // Temporary buffer to protect master frustum

            // Slice the frustum to this cascade's near/far range
            for(Uint i = 0; i < 4; i++)
            {
                const glm::vec4 dist = frustumCorners[i + 4] - frustumCorners[i];
                sliceCorners[i + 4] = frustumCorners[i] + dist * splitDist;
                sliceCorners[i] = frustumCorners[i] + dist * lastSplitDist;
            }

            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for(const glm::vec4& c : sliceCorners)
                frustumCenter += glm::vec3(c);
            frustumCenter /= static_cast<float>(NUM_FRUSTUM_CORNERS);

            // Bounding sphere: the box keeps its size while the camera rotates, so shadow edges don't swim
            float radius = 0.0f;
            for(const glm::vec4& c : sliceCorners)
                radius = glm::max(radius, glm::length(glm::vec3(c) - frustumCenter));
            radius = std::ceil(radius * 16.0f) / 16.0f;
            const float extent = radius * resolution / (resolution - 2.0f * CASCADE_BORDER_TEXELS);
            const float farPlane = 2.0f * extent;

            const glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * radius, frustumCenter, upVector);

            // Casters: anything inside the box from far toward the light to the far side of the sphere
            const Frustum casterFrustum(glm::ortho(-extent, extent, -extent, extent, -MAX_CASTER_DISTANCE, farPlane) * lightViewMatrix);
            const glm::vec3 depthAxis = -glm::vec3(lightViewMatrix[0][2], lightViewMatrix[1][2], lightViewMatrix[2][2]); // Light view depth = -z
            const float depthOffset = -lightViewMatrix[3][2];

            mDrawItems.clear();
            float nearestCaster = 0.0f;
            for(Uint i = 0; i < static_cast<Uint>(mCasters.size()); i++)
            {
                const Caster& caster = mCasters[i];
                if(!IntersectsBox(casterFrustum, caster.Center, caster.Extents))
                    continue;

                mDrawItems.push_back({ caster.Mesh_, caster.SubmeshIndex, i });
                const float casterNear = glm::dot(depthAxis, caster.Center) + depthOffset - glm::dot(glm::abs(depthAxis), caster.Extents);
                nearestCaster = std::min(nearestCaster, casterNear);
            }

            // Near plane fitted to the casters instead of a fixed pullback: tall casters are never clipped and the depth range
            // stays as small as the scene allows (D16 precision)
            const float nearPlane = std::max(std::floor(nearestCaster / NEAR_PLANE_STEP) * NEAR_PLANE_STEP, -MAX_CASTER_DISTANCE);
            glm::mat4 lightProjectionMatrix = glm::ortho(-extent, extent, -extent, extent, nearPlane, farPlane);
            lightProjectionMatrix[1][1] *= -1; // Vulkan NDC Y flip

            // Texel snapping to eliminate shadow edge shimmer
            glm::mat4 shadowMatrix = lightProjectionMatrix * lightViewMatrix;
            glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            shadowOrigin *= resolution / 2.0f;
            glm::vec4 roundOffset = glm::round(shadowOrigin) - shadowOrigin;
            roundOffset *= 2.0f / resolution;
            roundOffset.z = 0.0f;
            roundOffset.w = 0.0f;
            shadowMatrix[3] += roundOffset;

            mLightViewProjections[cascade] = shadowMatrix;
            mTexelWorldSizes[cascade] = 2.0f * extent / resolution;
            lastSplitDist = splitDist;

            std::sort(mDrawItems.begin(), mDrawItems.end(), [](const ShadowDrawItem& a, const ShadowDrawItem& b)
            {
                if(a.Mesh_ != b.Mesh_) return a.Mesh_ < b.Mesh_;
                return a.SubmeshIndex < b.SubmeshIndex;
            });

            Vector<ShadowDraw>& draws = mCascadeDraws[cascade];
            draws.clear();
            for(const ShadowDrawItem& item : mDrawItems)
            {
                const Uint instanceIndex = static_cast<Uint>(mInstanceData.size());
                mInstanceData.push_back(mCasters[item.CasterIndex].Transform);

                if(!draws.empty() && draws.back().Mesh_ == item.Mesh_ && draws.back().SubmeshIndex == item.SubmeshIndex)
                    draws.back().InstanceCount++;
                else
                    draws.push_back({ item.Mesh_, item.SubmeshIndex, instanceIndex, 1 });
            }
            mCascadeVisible[cascade] = static_cast<Uint>(mDrawItems.size());
        }
    }

    void ShadowPass::UploadShadowData(const FrameContext& ctx, const FrameBlackboard& blackBoard, Uint cascadeCount)
    {
        const ShadowSettings& settings = blackBoard.ShadowSettings_;

        ShadowUBO data = {};
        data.Enabled = cascadeCount > 0 ? 1 : 0;
        if(cascadeCount > 0)
        {
            // Cascade i = cascade 0 * scale + offset: one light space position per vertex instead of one per cascade
            const glm::mat4 inverseCascade0 = glm::inverse(mLightViewProjections[0]);
            data.LightSpaceMatrix = mLightViewProjections[0];
            for(Uint i = 0; i < cascadeCount; i++)
            {
                const glm::mat4 toCascade = mLightViewProjections[i] * inverseCascade0; // Diagonal scale + translation
                data.CascadeScale[i] = glm::vec4(toCascade[0][0], toCascade[1][1], toCascade[2][2], mTexelWorldSizes[i]);
                data.CascadeOffset[i] = glm::vec4(glm::vec3(toCascade[3]), 0.0f);
                data.CascadeEnds[i] = mCascadeEnds[i];
            }

            data.AtlasTexelSize = { 1.0f / static_cast<float>(mResolution * cascadeCount), 1.0f / static_cast<float>(mResolution) };
            data.NormalBias = settings.NormalBias;
            data.CascadeBlend = glm::clamp(settings.CascadeBlend, 0.0f, 0.5f);
            data.Distance = mShadowDistance;
            data.FadeStart = mShadowDistance * (1.0f - glm::clamp(settings.FadeRange, 0.0f, 1.0f));
            data.CascadeCount = cascadeCount;
            data.FilterSize = settings.FilterSize <= 3 ? 3 : (settings.FilterSize <= 5 ? 5 : 7);
            data.DitheredBlend = settings.DitheredBlend ? 1 : 0;
            data.ShowCascades = settings.ShowCascades ? 1 : 0;
        }
        mRHI->UploadBuffer(blackBoard.ShadowUBOs[ctx.FrameIndex], &data, sizeof(ShadowUBO));
    }

    void ShadowPass::Prepare(const FrameContext& ctx, FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::Prepare");
        mActiveCascades = 0;

        // Nothing samples the shadow data without a sun
        if(!blackBoard.HasDirectionalLight)
            return;

        if(blackBoard.ShadowSettings_.Enable)
        {
            const Uint cascadeCount = static_cast<Uint>(std::clamp(blackBoard.ShadowSettings_.CascadeCount, 1, MAX_SHADOW_CASCADE_COUNT));
            UpdateAtlas(blackBoard, cascadeCount);
            GatherCasters(blackBoard);
            BuildCascades(blackBoard, cascadeCount);

            // No visible caster in any cascade: skip the atlas clear + store, the shader leaves the sun unshadowed
            if(!mInstanceData.empty())
            {
                EnsureInstanceCapacity(ctx.FrameIndex, static_cast<Uint>(mInstanceData.size()));
                mRHI->UploadBuffer(mInstanceBuffers[ctx.FrameIndex], mInstanceData.data(), static_cast<Uint>(sizeof(glm::mat4) * mInstanceData.size()), 0);
                mActiveCascades = cascadeCount;
            }
        }

        // Disabled shadows still upload Enabled = 0, so the sun is drawn unshadowed
        UploadShadowData(ctx, blackBoard, mActiveCascades);
    }

    void ShadowPass::Execute(const FrameContext& ctx, const FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::Execute");
        if(mActiveCascades == 0)
            return;

        // One render pass for the whole atlas: a single clear + store, an empty cascade stays cleared so it never shadows anything
        mRHI->CmdBeginRenderPass(ctx, mFramebuffer);
        mRHI->CmdBindPipeline(ctx, mPipeline);
        mRHI->CmdBindDescriptorSet(ctx, mPipeline, mInstanceSet, DescriptorSetSlot::ZERO);

        const ShadowSettings& settings = blackBoard.ShadowSettings_;
        mRHI->CmdSetDepthBias(ctx, settings.DepthBiasConstant, settings.DepthBiasClamp, settings.DepthBiasSlope);

        const Mesh* boundMesh = nullptr;
        for(Uint cascade = 0; cascade < mActiveCascades; cascade++)
        {
            const Vector<ShadowDraw>& draws = mCascadeDraws[cascade];
            if(draws.empty())
                continue;

            mRHI->CmdSetViewport(ctx, cascade * mResolution, 0, mResolution, mResolution);
            mRHI->CmdPushConstants(ctx, mPipeline, ShaderType::VERTEX | ShaderType::FRAGMENT, 0, sizeof(glm::mat4), &mLightViewProjections[cascade]);

            for(const ShadowDraw& draw : draws)
            {
                if(draw.Mesh_ != boundMesh)
                {
                    mRHI->CmdBindVertexBuffer(ctx, draw.Mesh_->GetVertexBuffer());
                    mRHI->CmdBindIndexBuffer(ctx, draw.Mesh_->GetIndexBuffer());
                    boundMesh = draw.Mesh_;
                }

                const Submesh& submesh = draw.Mesh_->GetSubmeshes()[draw.SubmeshIndex];
                mRHI->CmdDrawIndexed(ctx, submesh.IndexCount, draw.InstanceCount, submesh.BaseIndex, submesh.BaseVertex, draw.FirstInstance);
            }
        }
        mRHI->CmdEndRenderPass(ctx, mFramebuffer);
    }

    void ShadowPass::Resize(Uint /*width*/, Uint /*height*/, FrameBlackboard& /*blackBoard*/) {}
    void ShadowPass::RenderStatsViaImGui(FrameBlackboard& blackBoard)
    {
        if(mActiveCascades == 0)
            return;

        ImGui::SeparatorText("Shadows");
        const double atlasMB = static_cast<double>(mRHI->GetImageSize(blackBoard.ShadowAtlas)) / (1024.0 * 1024.0);
        ImGui::Text("Atlas: %ux%u D16 (%.1f MB)", mResolution * mAtlasCascades, mResolution, atlasMB);
        ImGui::Text("Casters: %u", static_cast<Uint>(mCasters.size()));
        for(Uint i = 0; i < mActiveCascades; i++)
            ImGui::Text("Cascade %u: %u visible, ends at %.1f, texel %.3f", i, mCascadeVisible[i], mCascadeEnds[i], mTexelWorldSizes[i]);
    }

    void ShadowPass::Shutdown(FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::Shutdown");

        mRHI->DestroyFramebuffer(mFramebuffer);
        mRHI->DestroyImage(blackBoard.ShadowAtlas);
        mRHI->DestroyPipeline(mPipeline);

        mRHI->DestroyDescriptorSet(mInstanceSet);
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            mRHI->DestroyBuffer(mInstanceBuffers[i]);
    }
}
