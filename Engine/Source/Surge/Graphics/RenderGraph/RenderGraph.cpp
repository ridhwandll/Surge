// Copyright (c) - SurgeTechnologies - All rights reserved
#include "RenderGraph.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/RHI/RHI.hpp"
#include "Surge/Core/Profiler.hpp"

namespace Surge
{
    void RenderGraph::Setup(GraphicsRHI* rhi)
    {
        mRHI = rhi;
        mImGuiEnabled = Core::GetClient()->GetClientOptions().EnableImGui;

        // Producers must be registered before consumers so blackboard handles are valid
        for(auto& pass : mPasses)
            pass->Setup(rhi, mBlackboard);
    }

    void RenderGraph::Compile()
    {
        SCOPED_TIMER("RenderGraph::Compile");
        mCompiledGraph = {};

        ExecutionGroup outlineGroup = { .Name = "OutlineMask", .Type = PassGroup::OUTLINE_MASK, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .ManagesOwnExecution = false, .IsSwapchain = false };
        ExecutionGroup shadowGroup = { .Name = "Shadow", .Type = PassGroup::SHADOW, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .ManagesOwnExecution = true, .IsSwapchain = false };
        ExecutionGroup mainGroup = { .Name = "MainScene", .Type = PassGroup::MAIN_SCENE, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .ManagesOwnExecution = false, .IsSwapchain = false };
        ExecutionGroup aoGroup = { .Name = "AmbientOcclusion", .Type = PassGroup::AMBIENT_OCCLUSION, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .ManagesOwnExecution = true, .IsSwapchain = false };
        ExecutionGroup bloomGroup = { .Name = "Bloom", .Type = PassGroup::BLOOM, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .ManagesOwnExecution = true, .IsSwapchain = false };
        ExecutionGroup postProcessGroup = { .Name = "PostProcess", .Type = PassGroup::POST_PROCESS, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .ManagesOwnExecution = false, .IsSwapchain = false };
        ExecutionGroup uiOverlayGroup = { .Name = "UIOverlay", .Type = PassGroup::UI_OVERLAY, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .ManagesOwnExecution = false, .IsSwapchain = false };
        ExecutionGroup swapchainGroup = { .Name = "Swapchain", .Type = PassGroup::SWAPCHAIN, .Passes = {}, .BarriersBeforeGroup = {}, .Framebuffer {}, .IsSwapchain = true };

        for(auto& pass : mPasses)
        {
            switch(pass->GetGroup())
            {
                case PassGroup::SHADOW:       shadowGroup.Passes.push_back(pass.get());      break;
                case PassGroup::MAIN_SCENE:   mainGroup.Passes.push_back(pass.get());        break;
                case PassGroup::AMBIENT_OCCLUSION: aoGroup.Passes.push_back(pass.get());     break;
                case PassGroup::BLOOM:        bloomGroup.Passes.push_back(pass.get());       break;
                case PassGroup::OUTLINE_MASK: outlineGroup.Passes.push_back(pass.get());     break;
                case PassGroup::POST_PROCESS: postProcessGroup.Passes.push_back(pass.get()); break;
                case PassGroup::UI_OVERLAY:   uiOverlayGroup.Passes.push_back(pass.get());   break;
                case PassGroup::SWAPCHAIN:    swapchainGroup.Passes.push_back(pass.get());   break;
            }
        }

        outlineGroup.Framebuffer = mBlackboard.OutlineFramebuffer;
        postProcessGroup.Framebuffer = mBlackboard.PostProcessFramebuffer;
        uiOverlayGroup.Framebuffer = mBlackboard.UIOverlayFramebuffer;
        mainGroup.Framebuffer = mBlackboard.MainPassFramebuffer;

        SortByDependencies(shadowGroup.Passes);
        SortByDependencies(mainGroup.Passes);
        SortByDependencies(aoGroup.Passes);
        SortByDependencies(bloomGroup.Passes);
        SortByDependencies(outlineGroup.Passes);
        SortByDependencies(postProcessGroup.Passes);
        SortByDependencies(uiOverlayGroup.Passes);
        SortByDependencies(swapchainGroup.Passes);

        // Execution order. Passes may move between groups (e.g. PostProcess/UI render straight into the swapchain in the Player),
        // so barriers are derived from the actual order instead of fixed group pairs
        Vector<ExecutionGroup*> ordered = { &outlineGroup, &shadowGroup, &mainGroup, &aoGroup, &bloomGroup, &postProcessGroup, &uiOverlayGroup, &swapchainGroup };
        DeriveBarriers(ordered);

        // Build final groups
        if(!outlineGroup.Passes.empty())
            mCompiledGraph.Groups.push_back(std::move(outlineGroup));

        if(!shadowGroup.Passes.empty())
            mCompiledGraph.Groups.push_back(std::move(shadowGroup));

        mCompiledGraph.Groups.push_back(std::move(mainGroup));

        if(!aoGroup.Passes.empty())
            mCompiledGraph.Groups.push_back(std::move(aoGroup));

        if(!bloomGroup.Passes.empty())
            mCompiledGraph.Groups.push_back(std::move(bloomGroup));

        if (!postProcessGroup.Passes.empty())
            mCompiledGraph.Groups.push_back(std::move(postProcessGroup));

        if(!uiOverlayGroup.Passes.empty())
            mCompiledGraph.Groups.push_back(std::move(uiOverlayGroup));

        if(!swapchainGroup.Passes.empty())
            mCompiledGraph.Groups.push_back(std::move(swapchainGroup));

        mCompiledGraph.IsValid = true;
    }

    void RenderGraph::Execute(FrameContext& ctx)
    {
        SURGE_PROFILE_FUNC("RenderGraph::Execute");

        for(auto& pass : mPasses)
        {
            if(pass->IsEnabled() && (mSceneEnabled || pass->GetGroup() == PassGroup::SWAPCHAIN))
                pass->Prepare(ctx, mBlackboard);
        }

        for (const ExecutionGroup& group : mCompiledGraph.Groups)
        {
            if(!mSceneEnabled && !group.IsSwapchain)
                continue;

            for(const ImageBarrier& barrier : group.BarriersBeforeGroup)
                mRHI->CmdTransitionImageLayout(ctx, barrier.Handle, barrier.NewUsage);

            // An empty group would still clear + store its attachments, which costs a full-screen write on tile based GPUs
            // The swapchain group always runs (it presents), self managed groups decide inside Execute
            if (!group.ManagesOwnExecution && !group.IsSwapchain)
            {
                bool hasWork = false;
                for(RenderPass* pass : group.Passes)
                    hasWork |= pass->IsEnabled() && pass->HasWork(mBlackboard);

                if(!hasWork)
                    continue;
            }

            if (!group.ManagesOwnExecution)
                group.IsSwapchain ? mRHI->CmdBeginSwapchainRenderpass(ctx) : mRHI->CmdBeginRenderPass(ctx, group.Framebuffer);

            for(RenderPass* pass : group.Passes)
            {
                if (pass->IsEnabled())
                    pass->Execute(ctx, mBlackboard);
            }

            if(!group.ManagesOwnExecution)
                group.IsSwapchain ? (mImGuiEnabled ? RenderImGuiCallbacks() : void(), mRHI->CmdEndSwapchainRenderpass(ctx)) : mRHI->CmdEndRenderPass(ctx, group.Framebuffer);
        }
    }

    void RenderGraph::OnWindowResize(Uint width, Uint height)
    {
        if(RHISettings::RENDER_TO_SWAPCHAIN && (width > 0 && height > 0))
        {
            for(auto& pass : mPasses)
                pass->Resize(width, height, mBlackboard);
        }
    }

    void RenderGraph::ForceResize(Uint width, Uint height)
    {
        for(auto& pass : mPasses)
            pass->Resize(width, height, mBlackboard);
    }

    void RenderGraph::Shutdown()
    {
        for(auto& pass : mPasses)
            pass->Shutdown(mBlackboard);
    }

    void RenderGraph::AddImGuiRenderCallback(std::function<void()> callback)
    {
        if(callback && mImGuiEnabled)
            mImGuiRenderCallbacks.push_back(std::move(callback));
    }

    void RenderGraph::SortByDependencies(Vector<RenderPass*>& passes)
    {
        //SCOPED_TIMER("RenderGraph::SortByDependencies");

        if(passes.size() <= 1)
            return;

        // Stable bubble-up sort based on direct read/write dependencies
        bool changed = true;
        size_t iterations = 0;
        const size_t maxIterations = passes.size() * passes.size(); // To detect cycles

        while(changed)
        {
            changed = false;
            iterations++;

            if(iterations > maxIterations)
            {
                SG_ASSERT_INTERNAL("Cyclic dependency between passes!");
                return;
            }

            for(size_t i = 0; i < passes.size() - 1; ++i)
            {
                RenderPass* a = passes[i];
                RenderPass* b = passes[i + 1];

                // (Rid)Does pass A depend on pass B?
                // (Meaning B writes to something A reads, so B must go BEFORE A)
                bool aDependsOnB = false;
                for(ImageHandle write : b->GetImageWrites())
                {
                    for(ImageHandle read : a->GetImageReads())
                    {
                        if(write == read)
                        {
                            aDependsOnB = true;
                            break;
                        }
                    }
                    if(aDependsOnB)
                        break;
                }

                if(aDependsOnB)
                {
                    // If A depends on B, swap them so B comes first
                    std::swap(passes[i], passes[i + 1]);
                    changed = true;
                }
            }
        }
    }

    void RenderGraph::DeriveBarriers(Vector<ExecutionGroup*>& orderedGroups)
    {
        // Images written by an executed group and not yet transitioned for sampling
        // Example: MainScene writes MainPassColorImage -> Bloom reads it -> barrier before Bloom, PostProcess later reads it without another one
        Vector<ImageHandle> pendingWrites;
        auto contains = [](const Vector<ImageHandle>& list, ImageHandle h) { return std::find(list.begin(), list.end(), h) != list.end(); };

        for(ExecutionGroup* group : orderedGroups)
        {
            for(RenderPass* pass : group->Passes)
            {
                for(ImageHandle read : pass->GetImageReads())
                {
                    auto itr = std::find(pendingWrites.begin(), pendingWrites.end(), read);
                    if(itr == pendingWrites.end())
                        continue;

                    group->BarriersBeforeGroup.push_back({ read, ImageUsage::SAMPLED });
                    pendingWrites.erase(itr);
                }
            }

            for(RenderPass* pass : group->Passes)
            {
                for(ImageHandle write : pass->GetImageWrites())
                {
                    if(!contains(pendingWrites, write))
                        pendingWrites.push_back(write);
                }
            }
        }
    }

    void RenderGraph::RenderImGuiCallbacks()
    {
        // The render debug UI is Editor code (Render Graph / Renderer panels), registered like any other callback
        for(auto& callback : mImGuiRenderCallbacks)
            callback();
    }
}
