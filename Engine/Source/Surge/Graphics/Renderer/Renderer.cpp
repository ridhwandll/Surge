// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Profiler.hpp"
#include "Surge/Graphics/RHI/RHI.hpp"
#include "Surge/Graphics/RenderGraph/Passes/Renderer2DPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/GeometryPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/PostProcessPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/SwapchainPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/OutlinePass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/SkyPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/ShadowPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/UIOverlayPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/AmbientOcclusionPass.hpp"
#include "Surge/Graphics/RenderGraph/Passes/BloomPass.hpp"

#include "Surge/Graphics/UISystem/UIManager.hpp"

#define ENGINE_SHADER_PATH "Engine/Assets/Shaders"

namespace Surge
{
    void Renderer::Initialize()
    {
        SURGE_PROFILE_FUNC("Renderer::Initialize()");

        const ClientOptions& clientOptions = Core::GetClient()->GetClientOptions();
        RHISettings::RENDER_TO_SWAPCHAIN = clientOptions.RenderFinalImageToSwapchian;

        mShaderManager.Initialize(ENGINE_SHADER_PATH);
        mShaderManager.Load("Renderer2D.glsl");
        mShaderManager.Load("Renderer2DLine.glsl");
        mShaderManager.Load("Renderer2DText.glsl");
        mShaderManager.Load("UI.glsl");
        mShaderManager.Load("Renderer3D.glsl");
        mShaderManager.Load("PostProcess.glsl");
        mShaderManager.Load("OutlineMask.glsl");
        mShaderManager.Load("PreethamSky.glsl");
        mShaderManager.Load("Shadow.glsl");
        mShaderManager.Load("AOLinearDepth.glsl");
        mShaderManager.Load("GTAO.glsl");
        mShaderManager.Load("AOBlur.glsl");
        mShaderManager.Load("Bloom.glsl");
        mShaderManager.Load("BloomCompute.glsl", ShaderType::COMPUTE);

        mRHI = CreateScope<GraphicsRHI>();
        mRHI->Initialize(Core::GetWindow());

        FrameBlackboard& blackBoard = mMainRenderGraph.GetBlackboard();

        //Sampler
        {
            SamplerDesc samplerDesc = {};
            samplerDesc.DebugName = "DefaultSampler";
            samplerDesc.Min = FilterMode::NEAREST; // We set to NEAREST as Rendergraph Passes uses this internally
            samplerDesc.Mag = FilterMode::NEAREST;
            samplerDesc.Mip = MipmapMode::LINEAR;
            samplerDesc.WrapU = WrapMode::REPEAT;
            samplerDesc.WrapV = WrapMode::REPEAT;
            samplerDesc.Anisotropy = true;
            samplerDesc.MaxAniso = 4;
            blackBoard.DefaultSampler = mRHI->CreateSampler(samplerDesc);
        }
        {
            SamplerDesc samplerDesc = {};
            samplerDesc.DebugName = "TextSampler";
            samplerDesc.Min = FilterMode::LINEAR;
            samplerDesc.Mag = FilterMode::LINEAR;
            samplerDesc.Mip = MipmapMode::NEAREST;
            samplerDesc.WrapU = WrapMode::CLAMP;
            samplerDesc.WrapV = WrapMode::CLAMP;
            blackBoard.TextSampler = mRHI->CreateSampler(samplerDesc);
        }
        {
            SamplerDesc samplerDesc = {};
            samplerDesc.DebugName = "PointClampSampler";
            samplerDesc.Min = FilterMode::NEAREST;
            samplerDesc.Mag = FilterMode::NEAREST;
            samplerDesc.Mip = MipmapMode::NEAREST;
            samplerDesc.WrapU = WrapMode::CLAMP;
            samplerDesc.WrapV = WrapMode::CLAMP;
            blackBoard.PointClampSampler = mRHI->CreateSampler(samplerDesc);
        }
        {
            SamplerDesc samplerDesc = {};
            samplerDesc.DebugName = "LinearClampSampler";
            samplerDesc.Min = FilterMode::LINEAR;
            samplerDesc.Mag = FilterMode::LINEAR;
            samplerDesc.Mip = MipmapMode::NEAREST;
            samplerDesc.WrapU = WrapMode::CLAMP;
            samplerDesc.WrapV = WrapMode::CLAMP;
            blackBoard.LinearClampSampler = mRHI->CreateSampler(samplerDesc);
        }

        Byte whitePixel[] = { 255, 255, 255, 255 };
        ImageDesc texDesc = {};
        texDesc.Width = 1;
        texDesc.Height = 1;
        texDesc.Format = ImageFormat::RGBA8_UNORM;
        texDesc.Usage = ImageUsage::SAMPLED | ImageUsage::TRANSFER_DST;
        texDesc.DebugName = "WhiteTexture";
        texDesc.InitialData = whitePixel;
        texDesc.GenerateImGuiID = true;
        texDesc.DataSize = sizeof(whitePixel);
        texDesc.Sampler = blackBoard.DefaultSampler;
        blackBoard.WhiteImage = mRHI->CreateImage(texDesc);

        BufferDesc frameUBODesc = {};
        frameUBODesc.Usage = BufferUsage::UNIFORM;
        frameUBODesc.HostVisible = true;
        frameUBODesc.DebugName = "FrameUBO";
        frameUBODesc.Size = sizeof(FrameUBO);
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            blackBoard.FrameUBOs[i] = mRHI->CreateBuffer(frameUBODesc);

        AddScenePasses(mMainRenderGraph, true);
        mMainRenderGraph.AddPass<SwapchainPass>();
        mMainRenderGraph.Setup(mRHI.get());
        mMainRenderGraph.Compile();

        mUIManager.Initialize();
    }

    void Renderer::AddScenePasses(RenderGraph& graph, bool withUI)
    {
        graph.AddPass<ShadowPass>();
        graph.AddPass<OutlinePass>();
        graph.AddPass<GeometryPass>();
        graph.AddPass<SkyPass>();
        graph.AddPass<Renderer2DPass>();
        graph.AddPass<AmbientOcclusionPass>(); // After GeometryPass (needs MainPassDepthImage), before PostProcessPass (consumes AOImage)
        graph.AddPass<BloomPass>();            // After GeometryPass (needs MainPassColorImage), before PostProcessPass (consumes BloomImage)
        graph.AddPass<PostProcessPass>();
        if(withUI)
            graph.AddPass<UIOverlayPass>();
    }

    void Renderer::BeginFrame(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& cameraNearFar)
    {
        SURGE_PROFILE_FUNC("Renderer::BeginFrame(Camera)");
        FrameBlackboard& blackBoard = mMainRenderGraph.GetBlackboard();
        blackBoard.Env.HasEnvironment = false; //SHOULD we do it here? Seems hacky

        // Camera setup
        if (!RHISettings::RENDER_TO_SWAPCHAIN)
        {
            blackBoard.ScreenWidth = mScreenSize.x;
            blackBoard.ScreenHeight = mScreenSize.y;

            // The main scene isn't drawn, its screen size only feeds the UI layout: the view that shows the UI decides it
            const RenderView* uiView = mMainRenderGraph.IsSceneEnabled() ? nullptr : FindView(mUIView);
            if(uiView && uiView->Size.x > 0 && uiView->Size.y > 0)
            {
                blackBoard.ScreenWidth = static_cast<float>(uiView->Size.x);
                blackBoard.ScreenHeight = static_cast<float>(uiView->Size.y);
            }
        }
        else
        {
            glm::vec2 windowSize = Core::GetWindow()->GetSize();
            blackBoard.ScreenWidth = windowSize.x; blackBoard.ScreenHeight = windowSize.y;
        }

        blackBoard.ViewMatrix = viewMatrix;
        blackBoard.ProjectionMatrix = projectionMatrix;
        blackBoard.ViewProjection = blackBoard.ProjectionMatrix * blackBoard.ViewMatrix;
        blackBoard.InverseViewProjection = glm::inverse(blackBoard.ViewProjection);
        blackBoard.CameraPosition = glm::inverse(viewMatrix)[3];
        blackBoard.CameraNearFarPlane = cameraNearFar;
        blackBoard.CameraRight = { viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0] };
        blackBoard.CameraUp = { viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1] };

        FrameUBO frameData = {};
        frameData.View = blackBoard.ViewMatrix;
        frameData.ViewProjection = blackBoard.ViewProjection;
        frameData.InverseViewProjection = blackBoard.InverseViewProjection;
        frameData.CameraPos = blackBoard.CameraPosition;

        mCurrentFrameCtx = mRHI->BeginFrame();
        mRHI->UploadBuffer(blackBoard.FrameUBOs[mCurrentFrameCtx.FrameIndex], &frameData, sizeof(FrameUBO));
    }

    void Renderer::EndFrame()
    {
        SURGE_PROFILE_FUNC("Renderer::EndFrame()");

        mUIManager.ExtractRenderData(mMainRenderGraph.GetBlackboard());

        if(!mViews.empty())
            ExecuteViews();

        mMainRenderGraph.Execute(mCurrentFrameCtx);

        mRHI->EndFrame(mCurrentFrameCtx); // Stops command buffer recording & presents image to swapchain
        mMainRenderGraph.ClearLists();
        mOverlayMark.Set = false;
    }

    void Renderer::OnWindowResize(Uint width, Uint height)
    {
        mScreenSize = { width, height };
        mMainRenderGraph.OnWindowResize(width, height);
    }

    void Renderer::ForceResize(Uint width, Uint height)
    {
        mScreenSize = { width, height };
        mMainRenderGraph.ForceResize(width, height);
    }

    void Renderer::SetRenderScale(float scale)
    {
        FrameBlackboard& blackBoard = mMainRenderGraph.GetBlackboard();
        const float clamped = glm::clamp(scale, RenderScaleSettings::MIN_SCALE, RenderScaleSettings::MAX_SCALE);
        if(clamped == blackBoard.RenderScale)
            return;

        blackBoard.RenderScale = clamped;

        glm::uvec2 size = mScreenSize;
        if(size.x == 0 || size.y == 0)
            size = glm::uvec2(Core::GetWindow()->GetSize());

        mMainRenderGraph.ForceResize(size.x, size.y);

        // Views rebuild their scene targets at the new scale
        for(Scope<RenderView>& view : mViews)
        {
            if(view->Destroyed)
                continue;

            view->Graph->GetBlackboard().RenderScale = clamped;
            if(view->Size.x > 0 && view->Size.y > 0)
            {
                view->HasOutput = false;
                view->Graph->ForceResize(view->Size.x, view->Size.y);
            }
        }
    }

    void Renderer::SetMainSceneEnabled(bool enabled)
    {
        if(mMainRenderGraph.IsSceneEnabled() == enabled)
            return;

        mMainRenderGraph.SetSceneEnabled(enabled);

        // Unused targets shrink to a minimum while the views draw the scene (they still exist: the passes and their ImGui stay valid)
        constexpr Uint minimalSize = 64;
        const glm::uvec2 size = enabled ? glm::uvec2(Core::GetWindow()->GetSize()) : glm::uvec2(minimalSize);
        ForceResize(std::max(size.x, 1u), std::max(size.y, 1u));
    }

    void Renderer::BeginEditorOverlay()
    {
        const FrameBlackboard& blackBoard = mMainRenderGraph.GetBlackboard();
        mOverlayMark.Meshes = blackBoard.MeshList.size();
        mOverlayMark.Lights = blackBoard.LightList.size();
        mOverlayMark.Quads = blackBoard.QuadList.size();
        mOverlayMark.Lines = blackBoard.LineList.size();
        mOverlayMark.Texts = blackBoard.TextList.size();
        mOverlayMark.Outlines = blackBoard.OutlineList.size();
        mOverlayMark.Set = true;
    }

    //////////////////////////////////////////////////////////////////////////
    // Views
    //////////////////////////////////////////////////////////////////////////

    RenderViewID Renderer::CreateView(const RenderViewDesc& desc)
    {
        SG_ASSERT(!RHISettings::RENDER_TO_SWAPCHAIN, "Render views need offscreen targets, set ClientOptions::RenderFinalImageToSwapchain to false");

        Scope<RenderView> view = CreateScope<RenderView>();
        view->ID = mNextViewID++;
        view->Desc = desc;
        view->Graph = CreateScope<RenderGraph>();

        // Shared resources and settings come from the main graph, the targets and per-frame buffers are the view's own
        const FrameBlackboard& mainBoard = mMainRenderGraph.GetBlackboard();
        FrameBlackboard& blackBoard = view->Graph->GetBlackboard();
        blackBoard.RenderScale = mainBoard.RenderScale;
        blackBoard.WhiteImage = mainBoard.WhiteImage;
        blackBoard.DefaultSampler = mainBoard.DefaultSampler;
        blackBoard.TextSampler = mainBoard.TextSampler;
        blackBoard.PointClampSampler = mainBoard.PointClampSampler;
        blackBoard.LinearClampSampler = mainBoard.LinearClampSampler;
        blackBoard.ShadowSettings_ = mainBoard.ShadowSettings_;
        blackBoard.Lighting = mainBoard.Lighting;
        blackBoard.PostProcessSettings_ = mainBoard.PostProcessSettings_;
        blackBoard.AOSettings = mainBoard.AOSettings;
        blackBoard.Bloom = mainBoard.Bloom;
        blackBoard.ShowUI = desc.ShowUI;

        BufferDesc frameUBODesc = {};
        frameUBODesc.Usage = BufferUsage::UNIFORM;
        frameUBODesc.HostVisible = true;
        frameUBODesc.DebugName = desc.DebugName + " FrameUBO";
        frameUBODesc.Size = sizeof(FrameUBO);
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            blackBoard.FrameUBOs[i] = mRHI->CreateBuffer(frameUBODesc);

        AddScenePasses(*view->Graph, desc.ShowUI);
        view->Graph->Setup(mRHI.get());
        view->Graph->Compile();

        const RenderViewID id = view->ID;
        mViews.push_back(std::move(view));
        return id;
    }

    void Renderer::DestroyView(RenderViewID id)
    {
        RenderView* view = FindView(id);
        if(!view || view->Destroyed)
            return;

        // Callbacks the view's passes already queued (resizes) run first, nothing new is queued for a destroyed view
        view->Destroyed = true;
        if(mUIView == id)
            mUIView = INVALID_RENDER_VIEW;

        Core::AddFrameEndCallback([this, id]() {
            for(size_t i = 0; i < mViews.size(); i++)
            {
                if(mViews[i]->ID != id)
                    continue;

                DestroyViewImmediate(*mViews[i]);
                mViews.erase(mViews.begin() + static_cast<std::ptrdiff_t>(i));
                return;
            }
        });
    }

    void Renderer::DestroyViewImmediate(RenderView& view)
    {
        FrameBlackboard& blackBoard = view.Graph->GetBlackboard();
        view.Graph->Shutdown(); // RHI destruction is deferred until the GPU is done with the in-flight frames
        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            mRHI->DestroyBuffer(blackBoard.FrameUBOs[i]);
    }

    void Renderer::ResizeView(RenderViewID id, Uint width, Uint height)
    {
        RenderView* view = FindView(id);
        if(!view || view->Destroyed || width == 0 || height == 0 || view->RequestedSize == glm::uvec2(width, height))
            return;

        view->RequestedSize = { width, height };
        QueueViewResize(id, view->RequestedSize);
    }

    void Renderer::QueueViewResize(RenderViewID id, glm::uvec2 size)
    {
        // At frame end: this frame still draws with the current targets, the passes queue their own rebuilds right behind this one
        Core::AddFrameEndCallback([this, id, size]() {
            RenderView* view = FindView(id);
            if(!view || view->Destroyed || view->RequestedSize != size)
                return; // Destroyed, or resized again in the meantime

            mRHI->WaitIdle();
            view->Size = size;
            view->HasOutput = false;
            view->Graph->ForceResize(size.x, size.y);
        });
    }

    void Renderer::SetViewCamera(RenderViewID id, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& cameraNearFar)
    {
        RenderView* view = FindView(id);
        if(!view)
            return;

        view->ViewMatrix = viewMatrix;
        view->ProjectionMatrix = projectionMatrix;
        view->NearFar = cameraNearFar;
        view->HasCamera = true;
    }

    void Renderer::RequestViewRender(RenderViewID id)
    {
        if(RenderView* view = FindView(id))
            view->RenderRequested = true;
    }

    uint64_t Renderer::GetViewImGuiID(RenderViewID id) const
    {
        const RenderView* view = FindView(id);
        return view ? mRHI->GetImGuiImage(view->Graph->GetBlackboard().FinalImage) : 0;
    }

    bool Renderer::HasViewOutput(RenderViewID id) const
    {
        const RenderView* view = FindView(id);
        return view && view->HasOutput && !view->Destroyed;
    }

    glm::uvec2 Renderer::GetViewSize(RenderViewID id) const
    {
        const RenderView* view = FindView(id);
        return view ? view->Size : glm::uvec2(0);
    }

    RenderGraph* Renderer::GetViewGraph(RenderViewID id) const
    {
        const RenderView* view = FindView(id);
        return view && !view->Destroyed ? view->Graph.get() : nullptr;
    }

    Renderer::RenderView* Renderer::FindView(RenderViewID id) const
    {
        for(const Scope<RenderView>& view : mViews)
        {
            if(view->ID == id)
                return view.get();
        }
        return nullptr;
    }

    void Renderer::ExecuteViews()
    {
        SURGE_PROFILE_FUNC("Renderer::ExecuteViews");
        const FrameBlackboard& mainBlackBoard = mMainRenderGraph.GetBlackboard();
        const OverlayMark& mark = mOverlayMark;

        for(Scope<RenderView>& viewPtr : mViews)
        {
            RenderView& view = *viewPtr;
            const bool requested = view.RenderRequested;
            view.RenderRequested = false; // Requested again by the next ImGui frame while the view stays on screen
            if(view.Destroyed || !requested || !view.HasCamera || view.Size.x == 0 || view.Size.y == 0)
                continue;

            FrameBlackboard& blackBoard = view.Graph->GetBlackboard();

            // Camera
            blackBoard.ScreenWidth = static_cast<float>(view.Size.x);
            blackBoard.ScreenHeight = static_cast<float>(view.Size.y);
            blackBoard.ViewMatrix = view.ViewMatrix;
            blackBoard.ProjectionMatrix = view.ProjectionMatrix;
            blackBoard.ViewProjection = view.ProjectionMatrix * view.ViewMatrix;
            blackBoard.InverseViewProjection = glm::inverse(blackBoard.ViewProjection);
            blackBoard.CameraPosition = glm::inverse(view.ViewMatrix)[3];
            blackBoard.CameraNearFarPlane = view.NearFar;
            blackBoard.CameraRight = { view.ViewMatrix[0][0], view.ViewMatrix[1][0], view.ViewMatrix[2][0] };
            blackBoard.CameraUp = { view.ViewMatrix[0][1], view.ViewMatrix[1][1], view.ViewMatrix[2][1] };

            FrameUBO frameData = {};
            frameData.View = blackBoard.ViewMatrix;
            frameData.ViewProjection = blackBoard.ViewProjection;
            frameData.InverseViewProjection = blackBoard.InverseViewProjection;
            frameData.CameraPos = blackBoard.CameraPosition;
            mRHI->UploadBuffer(blackBoard.FrameUBOs[mCurrentFrameCtx.FrameIndex], &frameData, sizeof(FrameUBO));

            // Settings edited on the main graph (Renderer panel), passes rebuild their targets themselves when these change
            blackBoard.ShadowSettings_ = mainBlackBoard.ShadowSettings_;
            blackBoard.Lighting = mainBlackBoard.Lighting;
            blackBoard.PostProcessSettings_ = mainBlackBoard.PostProcessSettings_;
            blackBoard.AOSettings = mainBlackBoard.AOSettings;
            blackBoard.Bloom = mainBlackBoard.Bloom;
            blackBoard.Env = mainBlackBoard.Env;
            blackBoard.HasDirectionalLight = mainBlackBoard.HasDirectionalLight;
            blackBoard.DirectionalLightDir = mainBlackBoard.DirectionalLightDir;

            // Submissions, assign() reuses the view's capacity so this does no allocations
            const bool overlay = view.Desc.EditorOverlay || (mark.Set == false);
            const auto copyList = [overlay](auto& destination, const auto& source, size_t sceneCount) {
                destination.assign(source.begin(), source.begin() + static_cast<std::ptrdiff_t>(overlay ? source.size() : sceneCount));
            };
            copyList(blackBoard.MeshList, mainBlackBoard.MeshList, mark.Meshes);
            copyList(blackBoard.LightList, mainBlackBoard.LightList, mark.Lights);
            copyList(blackBoard.QuadList, mainBlackBoard.QuadList, mark.Quads);
            copyList(blackBoard.LineList, mainBlackBoard.LineList, mark.Lines);
            copyList(blackBoard.TextList, mainBlackBoard.TextList, mark.Texts);
            copyList(blackBoard.OutlineList, mainBlackBoard.OutlineList, mark.Outlines);

            if(view.Desc.ShowUI)
            {
                blackBoard.ShowUI = mainBlackBoard.ShowUI;
                blackBoard.UIResolution = { mainBlackBoard.ScreenWidth, mainBlackBoard.ScreenHeight }; // Laid out once (UI view size), stretched to this view
                blackBoard.UISpriteList.assign(mainBlackBoard.UISpriteList.begin(), mainBlackBoard.UISpriteList.end());
                blackBoard.UITextList.assign(mainBlackBoard.UITextList.begin(), mainBlackBoard.UITextList.end());
                blackBoard.UIDrawOrder.assign(mainBlackBoard.UIDrawOrder.begin(), mainBlackBoard.UIDrawOrder.end());
            }

            view.Graph->Execute(mCurrentFrameCtx);

            // ImGui samples it in the main graph's swapchain pass
            mRHI->CmdTransitionImageLayout(mCurrentFrameCtx, blackBoard.FinalImage, ImageUsage::SAMPLED);
            view.HasOutput = true;
        }
    }

    void Renderer::Shutdown()
    {
        SURGE_PROFILE_FUNC("Renderer::Shutdown()");
        FrameBlackboard& blackBoard = mMainRenderGraph.GetBlackboard();

        mUIManager.Shutdown();

        mRHI->WaitIdle();
        for(Scope<RenderView>& view : mViews)
            DestroyViewImmediate(*view); // No frame end left for the ones DestroyView() queued
        mViews.clear();
        mMainRenderGraph.Shutdown();

        mRHI->DestroyImage(blackBoard.WhiteImage);

        for(Uint i = 0; i < RHISettings::FRAMES_IN_FLIGHT; i++)
            mRHI->DestroyBuffer(blackBoard.FrameUBOs[i]);

        mRHI->DestroySampler(blackBoard.DefaultSampler);
        mRHI->DestroySampler(blackBoard.TextSampler);
        mRHI->DestroySampler(blackBoard.PointClampSampler);
        mRHI->DestroySampler(blackBoard.LinearClampSampler);
        mRHI->Shutdown();
    }

    void Renderer::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        // Hover follows the mouse and the primary touch (mouse events synthesized from touch), presses come from the left mouse
        // button and from every touch, the synthesized clicks are skipped so a tap is not processed twice
        dispatcher.Dispatch<MouseMovedEvent>([&](MouseMovedEvent& e) { mUIManager.ProcessMouseMove(e.GetX(), e.GetY()); });
        dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e) {
            if(!e.IsFromTouch() && e.GetMouseButton() == static_cast<MouseCode>(Mouse::ButtonLeft))
                mUIManager.ProcessPointerDown(UI::Manager::MOUSE_POINTER_ID, e.GetX(), e.GetY());
        });
        dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e) {
            if(!e.IsFromTouch() && e.GetMouseButton() == static_cast<MouseCode>(Mouse::ButtonLeft))
                mUIManager.ProcessPointerUp(UI::Manager::MOUSE_POINTER_ID, e.GetX(), e.GetY(), false);
        });
        dispatcher.Dispatch<TouchPressedEvent>([&](TouchPressedEvent& e) { mUIManager.ProcessPointerDown(e.GetPointerID(), e.GetX(), e.GetY()); });
        dispatcher.Dispatch<TouchReleasedEvent>([&](TouchReleasedEvent& e) { mUIManager.ProcessPointerUp(e.GetPointerID(), e.GetX(), e.GetY(), e.IsCanceled()); });
        dispatcher.Dispatch<WindowLostFocusEvent>([&](WindowLostFocusEvent&) { mUIManager.ProcessPointerUp(UI::Manager::MOUSE_POINTER_ID, -1.0f, -1.0f, true); });
    }

} // namespace Surge
