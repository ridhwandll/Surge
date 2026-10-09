// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderGraph/RenderPass.hpp"
#include "Surge/Graphics/RHI/RHIHandle.hpp"

namespace Surge
{
    /*
    * GeometryPass:
    * Reads : BlackBoard.ShadowPassImage
    * Writes: Blackboard.MainPassColorImage, BlackBoard.MainPassDepthImage
    *
    * Per frame (Prepare): frustum culls every submitted submesh, sorts the survivors by material/mesh and merges identical
    * (material, mesh, submesh) runs into one instanced draw. Per-instance transforms live in a per-frame storage buffer
    * indexed with gl_InstanceIndex (Renderer3D.glsl)
    */
    class GraphicsRHI;
    class Material;
    class GeometryPass : public RenderPass
    {
    public:
        GeometryPass() { mName = "GeometryPass"; mGroup = PassGroup::MAIN_SCENE; }
        virtual ~GeometryPass() = default;

        void Setup(GraphicsRHI* rhi, FrameBlackboard& blackBoard) override;
        void Prepare(const FrameContext& ctx, FrameBlackboard& blackBoard) override;
        void Execute(const FrameContext& ctx, const FrameBlackboard& blackBoard) override;
        void Resize(Uint width, Uint height, FrameBlackboard& blackBoard) override;
        void OnImGuiRender(FrameBlackboard& blackBoard) override;
        void Shutdown(FrameBlackboard& blackBoard) override;

    public:
        static constexpr Uint MAX_LIGHTS = 256; // Size of the Lights[] array in Renderer3D.glsl, the per-platform budget is FrameBlackboard::Lighting

    private:
        struct PushConstantData
        {
            Uint LightCount;
        };

        struct DrawItem
        {
            const Material* Material_;
            const Mesh* Mesh_;
            Uint SubmeshIndex;
            Uint TransformIndex; // Into mVisibleTransforms
        };

        struct InstancedDraw
        {
            const Material* Material_;
            const Mesh* Mesh_;
            Uint SubmeshIndex;
            Uint FirstInstance;
            Uint InstanceCount;
        };

        void BuildDrawList(const FrameContext& ctx, const FrameBlackboard& blackBoard);
        void UploadLights(const FrameContext& ctx, const FrameBlackboard& blackBoard);
        void EnsureInstanceCapacity(Uint frameIndex, Uint instanceCount);
        void UpdateDepthStoreOp(const FrameBlackboard& blackBoard);

    private:
        GraphicsRHI* mRHI;
        DescriptorSetHandle mFrameDescriptorSet;
        DescriptorSetHandle mShadowMapDescriptorSet;
        SamplerHandle mShadowSampler;
        BufferHandle mLightUBOs[RHISettings::FRAMES_IN_FLIGHT];
        PipelineHandle m3DPipeline;

        // Instancing
        BufferHandle mInstanceBuffers[RHISettings::FRAMES_IN_FLIGHT];
        Uint mInstanceCapacity[RHISettings::FRAMES_IN_FLIGHT] = {};
        Vector<glm::mat4> mVisibleTransforms;
        Vector<glm::mat4> mSortedTransforms;
        Vector<DrawItem> mDrawItems;
        Vector<InstancedDraw> mDraws;

        // Lights (scratch, reused every frame)
        LightUBOData mLightData = {};
        Vector<Pair<float, Uint>> mPointLightOrder;
        Uint mUploadedLightCount = 0;

        bool mDepthStored = true; // Depth is only written back to memory when something samples it later (Example: AO)

        // Stats
        Uint mSubmittedSubmeshes = 0;
        Uint mVisibleSubmeshes = 0;
    };
}
