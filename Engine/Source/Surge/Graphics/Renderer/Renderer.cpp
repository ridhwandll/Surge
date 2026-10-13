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
#include "Surge/Graphics/RenderGraph/Passes/ParticlePass.hpp"

#include "Surge/Graphics/UISystem/UIManager.hpp"
#include "Surge/Core/Input/Input.hpp"

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
        mShaderManager.Load("Particle.glsl");

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

        // Default particle: white with a soft round alpha falloff
        {
            constexpr Uint size = 64;
            Vector<Byte> pixels(size * size * 4);
            for(Uint y = 0; y < size; y++)
            {
                for(Uint x = 0; x < size; x++)
                {
                    const glm::vec2 p = (glm::vec2(static_cast<float>(x), static_cast<float>(y)) + 0.5f) / static_cast<float>(size) * 2.0f - 1.0f;
                    const float falloff = glm::clamp(1.0f - glm::length(p), 0.0f, 1.0f);
                    Byte* pixel = &pixels[(y * size + x) * 4];
                    pixel[0] = pixel[1] = pixel[2] = 255;
                    pixel[3] = static_cast<Byte>(falloff * falloff * (3.0f - 2.0f * falloff) * 255.0f + 0.5f);
                }
            }

            ImageDesc particleDesc = {};
            particleDesc.Width = size;
            particleDesc.Height = size;
            particleDesc.Format = ImageFormat::RGBA8_UNORM;
            particleDesc.Usage = ImageUsage::SAMPLED | ImageUsage::TRANSFER_DST;
            particleDesc.DebugName = "ParticleDefaultTexture";
            particleDesc.InitialData = pixels.data();
            particleDesc.DataSize = static_cast<Uint>(pixels.size());
            particleDesc.Sampler = blackBoard.LinearClampSampler;
            blackBoard.ParticleDefaultImage = mRHI->CreateImage(particleDesc);
        }

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
        graph.AddPass<ParticlePass>();         // After the opaque passes: no depth writes, premultiplied alpha
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
        blackBoard.CameraFrustum.Extract(blackBoard.ViewProjection);

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
        mMainRenderGraph.GetBlackboard().HasDirectionalLight = false; // Follows LightList: no sun next frame = no shadow cascades rendered
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
        mOverlayMark.ParticleEmitters = blackBoard.ParticleEmitterList.size();
        mOverlayMark.ParticleRecords = blackBoard.ParticleList.size();
        mOverlayMark.Set = true;
    }

    ParticleSubmission Renderer::SubmitParticles(const ParticleEmitterCmd& cmd, Uint count, RenderViewID view)
    {
        FrameBlackboard* blackBoard = &mMainRenderGraph.GetBlackboard();
        if(view != INVALID_RENDER_VIEW)
        {
            blackBoard = FindIsolatedBlackboard(view);
            if(!blackBoard)
                return {};
        }

        // The emitter index is 16 bits in ParticleRecord
        if(count == 0 || blackBoard->ParticleEmitterList.size() >= 0xFFFF)
            return {};

        ParticleSubmission submission;
        submission.EmitterIndex = static_cast<Uint>(blackBoard->ParticleEmitterList.size());

        ParticleEmitterCmd& emitter = blackBoard->ParticleEmitterList.emplace_back(cmd);
        emitter.FirstRecord = static_cast<Uint>(blackBoard->ParticleList.size());
        emitter.RecordCount = count;
        blackBoard->ParticleList.resize(blackBoard->ParticleList.size() + count); // Capacity is kept across frames, no allocation after warm-up
        submission.Records = blackBoard->ParticleList.data() + emitter.FirstRecord;
        return submission;
    }

    void Renderer::SubmitMesh(const glm::mat4& transform, const Mesh* mesh, bool dropShadow, RenderViewID view)
    {
        if(FrameBlackboard* blackBoard = FindIsolatedBlackboard(view))
            blackBoard->MeshList.emplace_back(MeshSubmitCmd { transform, mesh, dropShadow });
    }

    void Renderer::SubmitLight(const Light& light, RenderViewID view)
    {
        FrameBlackboard* blackBoard = FindIsolatedBlackboard(view);
        if(!blackBoard)
            return;

        if(light.PositionType.w == (float)LightType::DIRECTIONAL)
        {
            blackBoard->HasDirectionalLight = true; // Reset with the view's lists after it executed
            blackBoard->DirectionalLightDir = glm::vec3(light.PositionType);
        }
        blackBoard->LightList.emplace_back(light);
    }

    void Renderer::SubmitEnvironment(const Environnment& env, RenderViewID view)
    {
        if(FrameBlackboard* blackBoard = FindIsolatedBlackboard(view))
        {
            blackBoard->Env = env; // Reset with the view's lists after it executed
            blackBoard->Env.HasEnvironment = true;
        }
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
        blackBoard.ParticleDefaultImage = mainBoard.ParticleDefaultImage;
        blackBoard.DefaultSampler = mainBoard.DefaultSampler;
        blackBoard.TextSampler = mainBoard.TextSampler;
        blackBoard.PointClampSampler = mainBoard.PointClampSampler;
        blackBoard.LinearClampSampler = mainBoard.LinearClampSampler;
        blackBoard.ShadowSettings_ = mainBoard.ShadowSettings_;
        blackBoard.Lighting = mainBoard.Lighting;
        blackBoard.Particles = mainBoard.Particles;
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

    FrameBlackboard* Renderer::FindIsolatedBlackboard(RenderViewID id) const
    {
        RenderView* view = FindView(id);
        if(!view || view->Destroyed || !view->Desc.Isolated)
            return nullptr;
        return &view->Graph->GetBlackboard();
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
            {
                if(view.Desc.Isolated)
                {
                    view.Graph->ClearLists(); // Its submissions were for this frame only
                    view.Graph->GetBlackboard().HasDirectionalLight = false;
                    view.Graph->GetBlackboard().Env = Environnment {};
                }
                continue;
            }

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
            blackBoard.Particles = mainBlackBoard.Particles;
            blackBoard.ParticleStatistics = mainBlackBoard.ParticleStatistics;
            blackBoard.PostProcessSettings_ = mainBlackBoard.PostProcessSettings_;
            blackBoard.AOSettings = mainBlackBoard.AOSettings;
            blackBoard.Bloom = mainBlackBoard.Bloom;

            // Isolated views draw only what was submitted to them (previews): no scene lights, sky or environment
            if(!view.Desc.Isolated)
            {
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
                copyList(blackBoard.ParticleEmitterList, mainBlackBoard.ParticleEmitterList, mark.ParticleEmitters);
                copyList(blackBoard.ParticleList, mainBlackBoard.ParticleList, mark.ParticleRecords);
            }

            if(view.Desc.ShowUI)
            {
                blackBoard.ShowUI = mainBlackBoard.ShowUI;
                blackBoard.UIResolution = { mainBlackBoard.ScreenWidth, mainBlackBoard.ScreenHeight }; // Laid out once (UI view size), stretched to this view
                blackBoard.UISpriteList.assign(mainBlackBoard.UISpriteList.begin(), mainBlackBoard.UISpriteList.end());
                blackBoard.UITextList.assign(mainBlackBoard.UITextList.begin(), mainBlackBoard.UITextList.end());
                blackBoard.UIDrawOrder.assign(mainBlackBoard.UIDrawOrder.begin(), mainBlackBoard.UIDrawOrder.end());
                blackBoard.UIClipRects.assign(mainBlackBoard.UIClipRects.begin(), mainBlackBoard.UIClipRects.end());
            }

            view.Graph->Execute(mCurrentFrameCtx);

            // ImGui samples it in the main graph's swapchain pass
            mRHI->CmdTransitionImageLayout(mCurrentFrameCtx, blackBoard.FinalImage, ImageUsage::SAMPLED);
            view.HasOutput = true;

            if(view.Desc.Isolated)
            {
                view.Graph->ClearLists();
                blackBoard.HasDirectionalLight = false;
                blackBoard.Env = Environnment {};
            }
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
        mRHI->DestroyImage(blackBoard.ParticleDefaultImage);

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
        dispatcher.Dispatch<MouseMovedEvent>([&](MouseMovedEvent& e) {
            mUIManager.ProcessMouseMove(e.GetX(), e.GetY());
            if(!e.IsFromTouch())
                mUIManager.ProcessPointerMove(UI::Manager::MOUSE_POINTER_ID, e.GetX(), e.GetY());
        });
        dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& e) {
            const auto [x, y] = Input::GetMousePosition();
            mUIManager.ProcessScroll(x, y, e.GetDelta());
        });
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
        dispatcher.Dispatch<TouchMovedEvent>([&](TouchMovedEvent& e) { mUIManager.ProcessPointerMove(e.GetPointerID(), e.GetX(), e.GetY()); });

        // Keys reach the focused text field, Back/Escape closes popups first (Handled keeps Android Back from exiting the app)
        dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e) {
            if(mUIManager.ProcessKeyPressed(e.GetKeyCode()))
                e.Handled = true;
        });
        dispatcher.Dispatch<KeyTypedEvent>([&](KeyTypedEvent& e) { mUIManager.ProcessTextInput(static_cast<Uint>(e.GetKeyCode())); });
        dispatcher.Dispatch<WindowLostFocusEvent>([&](WindowLostFocusEvent&) { mUIManager.ProcessPointerUp(UI::Manager::MOUSE_POINTER_ID, -1.0f, -1.0f, true); });
    }

} // namespace Surge
