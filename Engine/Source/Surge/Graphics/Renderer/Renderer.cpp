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

        FrameBlackboard& blackBoard = mGraph.GetBlackboard();

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

        mGraph.AddPass<ShadowPass>();
        mGraph.AddPass<OutlinePass>();
        mGraph.AddPass<GeometryPass>();
        mGraph.AddPass<SkyPass>();
        mGraph.AddPass<Renderer2DPass>();
        mGraph.AddPass<AmbientOcclusionPass>(); // After GeometryPass (needs MainPassDepthImage), before PostProcessPass (consumes AOImage)
        mGraph.AddPass<BloomPass>();            // After GeometryPass (needs MainPassColorImage), before PostProcessPass (consumes BloomImage)
        mGraph.AddPass<PostProcessPass>();
        mGraph.AddPass<UIOverlayPass>();
        mGraph.AddPass<SwapchainPass>();
        mGraph.Setup(mRHI.get());
        mGraph.Compile();

        mUIManager.Initialize();
    }

    void Renderer::BeginFrame(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& cameraNearFar)
    {
        SURGE_PROFILE_FUNC("Renderer::BeginFrame(Camera)");
        FrameBlackboard& blackBoard = mGraph.GetBlackboard();
        blackBoard.Env.HasEnvironment = false; //SHOULD we do it here? Seems hacky

        // Camera setup
        if (!RHISettings::RENDER_TO_SWAPCHAIN)
        {
            blackBoard.ScreenWidth = mScreenSize.x;
            blackBoard.ScreenHeight = mScreenSize.y;
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

        mUIManager.ExtractRenderData(mGraph.GetBlackboard());

        mGraph.Execute(mCurrentFrameCtx);

        mRHI->EndFrame(mCurrentFrameCtx); // Stops command buffer recording & presents image to swapchain
        mGraph.ClearLists();
    }

    void Renderer::OnWindowResize(Uint width, Uint height)
    {
        mScreenSize = { width, height };
        mGraph.OnWindowResize(width, height);
    }

    void Renderer::ForceResize(Uint width, Uint height)
    {
        mScreenSize = { width, height };
        mGraph.ForceResize(width, height);
    }

    void Renderer::SetRenderScale(float scale)
    {
        FrameBlackboard& blackBoard = mGraph.GetBlackboard();
        const float clamped = glm::clamp(scale, RenderScaleSettings::MIN_SCALE, RenderScaleSettings::MAX_SCALE);
        if(clamped == blackBoard.RenderScale)
            return;

        blackBoard.RenderScale = clamped;

        glm::uvec2 size = mScreenSize;
        if(size.x == 0 || size.y == 0)
            size = glm::uvec2(Core::GetWindow()->GetSize());
        mGraph.ForceResize(size.x, size.y);
    }

    void Renderer::Shutdown()
    {
        SURGE_PROFILE_FUNC("Renderer::Shutdown()");
        FrameBlackboard& blackBoard = mGraph.GetBlackboard();

        mUIManager.Shutdown();

        mRHI->WaitIdle();
        mGraph.Shutdown();

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
