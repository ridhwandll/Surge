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
    void ShadowPass::Setup(GraphicsRHI* rhi, FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::Setup");
        mRHI = rhi;

        /*
        *  (Rid) Shadow map sizs for D16_UNORM (2 bytes per pixel)
        *  1024 * 0.5 = 0512 = 0.5 MB
        *  1024 * 1   = 1024 = 2   MB
        *  1024 * 2   = 2048 = 8   MB <- Our Default for mobile
        *  1024 * 3   = 3072 = 18  MB
        *  1024 * 4   = 4096 = 32  MB <- Our Default for PC
        * 
        *  Multiply these MB values by 2 to get size for D32_SFLOAT
        */

        mShadowMapResolution = 1024.0f * 2; // Default 2048
        glm::vec2 size = { mShadowMapResolution, mShadowMapResolution };

        ImageDesc depthDesc = {};
        depthDesc.Width = size.x;
        depthDesc.Height = size.y;
        depthDesc.Format = ImageFormat::D16_UNORM;
        depthDesc.Usage = ImageUsage::DEPTH_ATTACHMENT | ImageUsage::SAMPLED;
        depthDesc.GenerateImGuiID = true;
        for(Uint i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
        {
            depthDesc.DebugName = std::format("ShadowMap (Cascade: {})", i);
            blackBoard.ShadowMap[i] = mRHI->CreateImage(depthDesc);
        }

        for(Uint i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
        {
            FramebufferAttachment depthAttachment = {};
            depthAttachment.Handle = blackBoard.ShadowMap[i];
            depthAttachment.Load = LoadOp::CLEAR;
            depthAttachment.Store = StoreOp::STORE;

            FramebufferDesc fbDesc = {};
            fbDesc.ColorAttachmentCount = 0;
            fbDesc.DepthAttachment = depthAttachment;
            fbDesc.HasDepth = true;
            fbDesc.Width = size.x;
            fbDesc.Height = size.y;

            fbDesc.DebugName = std::format("ShadowPass Framebuffer (Cascade: {})", i);
            mShadowPassFramebuffer[i] = mRHI->CreateFramebuffer(fbDesc);
        }

        PipelineDesc desc = {};
        desc.Shader_ = Core::GetRenderer()->GetShaderManager().Get("Shadow.glsl");
        desc.Raster.Topo = Topology::TRIANGLE_LIST;
        desc.Raster.Polygon = PolygonMode::FILL;
        desc.Raster.Cull = CullMode::FRONT; //Cull front to reduce Peter panning

        desc.Raster.DepthBiasEnable = true;
        desc.Raster.DepthBiasConstantFactor = 1.25f;
        desc.Raster.DepthBiasSlopeFactor = 1.75f;
        desc.Raster.DepthBiasClamp = 0.0f;

        desc.Stencil.Enable = false;
        desc.Depth.TestEnable = true;
        desc.Depth.WriteEnable = true;
        desc.Depth.Op = CompareOp::LESS_OR_EQUAL;
        desc.Blend.Enable = false;
        
        desc.TargetSwapchain = false;
        for(Uint i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
        {
            desc.DebugName = std::format("ShadowPass (Cascade: {})", i);
            desc.TargetFramebuffer = mShadowPassFramebuffer[i];
            mShadowPipelines[i] = mRHI->CreatePipeline(desc);
        }

        for(Uint i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
            mImageWrites.push_back(blackBoard.ShadowMap[i]);

        // All cascade pipelines share one layout, so one instance set binds to any of them
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            EnsureInstanceCapacity(i, 1024);

        mInstanceSet = mRHI->CreateDescriptorSet(mShadowPipelines[0], DescriptorSetSlot::ZERO, DescriptorUpdateFrequency::DYNAMIC, "ShadowInstances [Set0]");
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
        {
            DescriptorWrite write = {};
            write.Binding = 0;
            write.Type = DescriptorType::STORAGE_BUFFER;
            write.Buffer = mInstanceBuffers[i];
            write.BufferRange = sizeof(glm::mat4) * mInstanceCapacity[i];
            mRHI->UpdateDescriptorSet(mInstanceSet, &write, 1, i);
        }

        mCasterTransforms.reserve(1024);
        mDrawItems.reserve(1024);
        mInstanceData.reserve(1024);
    }

    void ShadowPass::EnsureInstanceCapacity(Uint frameIndex, Uint instanceCount)
    {
        if(instanceCount <= mInstanceCapacity[frameIndex] && !mInstanceBuffers[frameIndex].IsNull())
            return;

        Uint capacity = std::max(mInstanceCapacity[frameIndex], 1024u);
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

    void ShadowPass::BuildCascadeDraws(const FrameContext& ctx, const FrameBlackboard& bb, Uint cascadeCount)
    {
        SURGE_PROFILE_FUNC("ShadowPass::BuildCascadeDraws");

        // World transforms of every shadow casting submesh, computed once and shared by all cascades
        mCasterTransforms.clear();
        for(const MeshSubmitCmd& cmd : bb.MeshList)
        {
            if(!cmd.DropShadow)
                continue;

            for(const Submesh& submesh : cmd.Mesh_->GetSubmeshes())
                mCasterTransforms.push_back(cmd.Transform * submesh.Transform);
        }
        mCasterCount = static_cast<Uint>(mCasterTransforms.size());

        mInstanceData.clear();
        for(Uint cascade = 0; cascade < cascadeCount; cascade++)
        {
            // Only what the light's ortho box can see, anything outside would be clipped by the rasterizer anyway
            const Frustum lightFrustum(mLightViewProjections[cascade]);

            mDrawItems.clear();
            Uint transformIndex = 0;
            for(const MeshSubmitCmd& cmd : bb.MeshList)
            {
                if(!cmd.DropShadow)
                    continue;

                const Vector<Submesh>& submeshes = cmd.Mesh_->GetSubmeshes();
                for(Uint i = 0; i < static_cast<Uint>(submeshes.size()); i++, transformIndex++)
                {
                    if(lightFrustum.IntersectsAABB(submeshes[i].BoundingBox, mCasterTransforms[transformIndex]))
                        mDrawItems.push_back({ cmd.Mesh_, i, transformIndex });
                }
            }

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
                mInstanceData.push_back(mCasterTransforms[item.TransformIndex]);

                if(!draws.empty() && draws.back().Mesh_ == item.Mesh_ && draws.back().SubmeshIndex == item.SubmeshIndex)
                    draws.back().InstanceCount++;
                else
                    draws.push_back({ item.Mesh_, item.SubmeshIndex, instanceIndex, 1 });
            }
            mCascadeVisible[cascade] = static_cast<Uint>(mDrawItems.size());
        }

        if(mInstanceData.empty())
            return;

        EnsureInstanceCapacity(ctx.FrameIndex, static_cast<Uint>(mInstanceData.size()));
        mRHI->UploadBuffer(mInstanceBuffers[ctx.FrameIndex], mInstanceData.data(), static_cast<Uint>(sizeof(glm::mat4) * mInstanceData.size()), 0);
    }

    void ShadowPass::Execute(const FrameContext& ctx, const FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::Execute");
        if(!blackBoard.HasDirectionalLight)
            return;

        CalculateCascades(blackBoard);
        const Uint& cascadeCount = blackBoard.ShadowSettings_.CascadeCount;

        ShadowUBO shadowData = {};
        shadowData.CascadeCount = cascadeCount;
        shadowData.ShowCascades = blackBoard.ShadowSettings_.ShowCascades;
        for(Uint i = 0; i < shadowData.CascadeCount; i++)
        {
            shadowData.CascadeEnds[i] = mCascadeEnds[i];
            shadowData.LightSpaceMatrix[i] = mLightViewProjections[i];
        }
        mRHI->UploadBuffer(blackBoard.ShadowUBOs[ctx.FrameIndex], &shadowData, sizeof(ShadowUBO));

        BuildCascadeDraws(ctx, blackBoard, cascadeCount);

        for(Uint cascade = 0; cascade < cascadeCount; cascade++)
        {
            // ShadowPass manages its own renderpass unlike other passes, an empty cascade is still cleared so stale depth never shadows anything
            mRHI->CmdBeginRenderPass(ctx, mShadowPassFramebuffer[cascade]);

            const Vector<ShadowDraw>& draws = mCascadeDraws[cascade];
            if(!draws.empty())
            {
                mRHI->CmdBindPipeline(ctx, mShadowPipelines[cascade]);
                mRHI->CmdBindDescriptorSet(ctx, mShadowPipelines[cascade], mInstanceSet, DescriptorSetSlot::ZERO);

                const float cascadeScale = 1.0f / (static_cast<float>(cascade) + 1.0f);
                const float currentConstant = mDepthBiasConstant * cascadeScale;
                const float currentSlope = mDepthBiasSlope * cascadeScale;
                mRHI->CmdSetDepthBias(ctx, currentConstant, mDepthBiasClamp, currentSlope);
                mRHI->CmdPushConstants(ctx, mShadowPipelines[cascade], ShaderType::VERTEX | ShaderType::FRAGMENT, 0, sizeof(glm::mat4), &mLightViewProjections[cascade]);

                const Mesh* boundMesh = nullptr;
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
            mRHI->CmdEndRenderPass(ctx, mShadowPassFramebuffer[cascade]);
        }
    }

    void ShadowPass::Resize(Uint /*width*/, Uint /*height*/, FrameBlackboard& /*blackBoard*/) {}
    void ShadowPass::OnImGuiRender(FrameBlackboard& blackBoard)
    {
        ImFont* boldFont = ImGui::GetIO().Fonts->Fonts[1];
        ImGui::PushFont(boldFont, 25.0f);
        ImGui::TextUnformatted("Shadow Pass");
        ImGui::Separator();
        ImGui::PopFont();

        ImGui::PushFont(boldFont, 20.0f);
        ImGui::TextUnformatted("Depth Bias");
        ImGui::Separator();
        ImGui::PopFont();
        ImGui::DragFloat("Constant", &mDepthBiasConstant, 0.1f, 0.0f, 100.0f, "%.2f");
        ImGui::DragFloat("Clamp", &mDepthBiasClamp, 0.1f, 0.0f, 100.0f, "%.2f");
        ImGui::DragFloat("Slope", &mDepthBiasSlope, 0.1f, 0.0f, 100.0f, "%.2f");
        ImGui::Separator();
        ImGui::Checkbox("Visualize Cascades", &blackBoard.ShadowSettings_.ShowCascades);
        ImGui::SliderInt("Cascade Count", &blackBoard.ShadowSettings_.CascadeCount, 2, MAX_SHADOW_CASCADE_COUNT);
        ImGui::SliderFloat("Cascade Split Lambda", &blackBoard.ShadowSettings_.CascadeSplitLambda, 0.0f, 1.0f, "%.3f");
        ImGui::Text("CascadeSplits: %f | %f | %f", mCascadeEnds[0], mCascadeEnds[1], mCascadeEnds[2]);
        ImGui::Text("Casters: %u | Visible per cascade: %u | %u | %u", mCasterCount, mCascadeVisible[0], mCascadeVisible[1], mCascadeVisible[2]);
    }

    void ShadowPass::Shutdown(FrameBlackboard& blackBoard)
    {
        SURGE_PROFILE_FUNC("ShadowPass::Shutdown");

        for(Uint i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
        {
            mRHI->DestroyFramebuffer(mShadowPassFramebuffer[i]);
            mRHI->DestroyImage(blackBoard.ShadowMap[i]);
            mRHI->DestroyPipeline(mShadowPipelines[i]);
        }

        mRHI->DestroyDescriptorSet(mInstanceSet);
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            mRHI->DestroyBuffer(mInstanceBuffers[i]);
    }

    // (Rid)Visualization: https://gemini.google.com/share/13d9f8dfbf32
    void ShadowPass::CalculateCascades(const FrameBlackboard& bb)
    {
        SURGE_PROFILE_FUNC("ShadowPass::CalculateCascades");

        constexpr Uint NUM_FRUSTUM_CORNERS = 8;
        const Uint totalCascades = bb.ShadowSettings_.CascadeCount;
        const float cascadeSplitLambda = bb.ShadowSettings_.CascadeSplitLambda;

        const float minZ = bb.CameraNearFarPlane.x;
        const float maxZ = bb.CameraNearFarPlane.y;
        const float clipRange = maxZ - minZ;
        const float ratio = maxZ / minZ;

        // Calculate split intervals
        for(Uint i = 0; i < totalCascades; i++)
        {
            const float p = (i + 1.0f) / static_cast<float>(totalCascades);
            const float log = minZ * glm::pow(ratio, p);
            const float uniform = maxZ * p;
            const float d = cascadeSplitLambda * (log - uniform) + uniform;
            mCascadeSplits[i] = (d - minZ) / clipRange;
        }

        // Reconstruct master view frustum in world space
        glm::mat4 inverseViewProjection = glm::inverse(bb.ViewProjection);
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
            glm::vec4 inv = inverseViewProjection * c;
            c = inv / inv.w;
        }

        // Setup light orientation properties
        glm::vec3 lightDir = glm::normalize(bb.DirectionalLightDir);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        if(glm::abs(lightDir.y) > 0.99f)
            upVector = glm::vec3(0.0f, 0.0f, 1.0f);

        // Compute orthographic projections per cascade layer
        float lastSplitDist = 0.0f;
        for(Uint cascade = 0; cascade < totalCascades; cascade++)
        {
            float splitDist = mCascadeSplits[cascade];
            glm::vec4 sliceCorners[NUM_FRUSTUM_CORNERS]; //Temporary buffer to protect master frustum

            // Slice the frustum to this cascade's near/far range
            for(Uint i = 0; i < 4; i++)
            {
                glm::vec4 dist = frustumCorners[i + 4] - frustumCorners[i];
                sliceCorners[i + 4] = frustumCorners[i] + dist * splitDist;
                sliceCorners[i] = frustumCorners[i] + dist * lastSplitDist;
            }

            // Compute frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for(const glm::vec4& c : sliceCorners)
                frustumCenter += glm::vec3(c);
            frustumCenter /= static_cast<float>(NUM_FRUSTUM_CORNERS);

            // Compute bounding sphere radius
            float radius = 0.0f;
            for(const glm::vec4& c : sliceCorners)
                radius = glm::max(radius, glm::length(glm::vec3(c) - frustumCenter));
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            constexpr float zPullback = 50.0f;
            float nearPlane = -zPullback;
            float farPlane = (maxExtents.z - minExtents.z) + zPullback;

            // View & Ortho Matrices
            glm::mat4 lightViewMatrix = glm::lookAt(
                frustumCenter - lightDir * maxExtents.z,
                frustumCenter,
                upVector
            );
            glm::mat4 lightProjectionMatrix = glm::ortho(
                minExtents.x, maxExtents.x,
                minExtents.y, maxExtents.y,
                nearPlane, farPlane
            );
            lightProjectionMatrix[1][1] *= -1; // Vulkan NDC Y flip

            // Texel snapping to eliminate shadow edge shimmer
            glm::mat4 shadowMatrix = lightProjectionMatrix * lightViewMatrix;
            glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            shadowOrigin *= mShadowMapResolution / 2.0f;
            glm::vec4 roundOffset = glm::round(shadowOrigin) - shadowOrigin;
            roundOffset *= 2.0f / mShadowMapResolution;
            roundOffset.z = 0.0f;
            roundOffset.w = 0.0f;
            shadowMatrix[3] += roundOffset;

            // Store final parameters
            mCascadeEnds[cascade] = minZ + splitDist * clipRange;
            mLightViewProjections[cascade] = shadowMatrix;

            lastSplitDist = splitDist;
        }
    }
}
