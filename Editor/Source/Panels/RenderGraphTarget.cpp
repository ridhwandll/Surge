// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/RenderGraphTarget.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Viewport/SceneViewport.hpp"
#include "Editor.hpp"

#include <imgui.h>
#include <format>

namespace Surge
{
    RenderGraph* RenderGraphTarget::Resolve()
    {
        Renderer* renderer = Core::GetRenderer();
        if(mMode == Mode::VIEW && !renderer->GetViewGraph(mView))
            mMode = Mode::ACTIVE_SCENE_VIEW; // Its viewport was closed

        RenderViewID id = INVALID_RENDER_VIEW;
        if(mMode == Mode::VIEW)
            id = mView;
        else if(mMode == Mode::ACTIVE_SCENE_VIEW)
        {
            const SceneViewport* sceneView = static_cast<Editor*>(Core::GetClient())->GetViewports().GetActiveSceneView();
            id = sceneView ? sceneView->GetViewID() : INVALID_RENDER_VIEW;
        }

        RenderGraph* graph = id != INVALID_RENDER_VIEW ? renderer->GetViewGraph(id) : nullptr;
        mResolvedView = graph ? id : INVALID_RENDER_VIEW;

        if(mLabelMode != mMode || mLabelView != mResolvedView)
        {
            mLabelMode = mMode;
            mLabelView = mResolvedView;

            String viewName = "Main Graph";
            renderer->ForEachView([&](RenderViewID viewID, const RenderViewDesc& desc) {
                if(viewID == mResolvedView)
                    viewName = desc.DebugName;
            });
            mLabel = mMode == Mode::ACTIVE_SCENE_VIEW ? std::format("Active Scene View ({})", viewName) : viewName;
        }

        return graph ? graph : &renderer->GetMainRenderGraph();
    }

    void RenderGraphTarget::DrawSelector(float width)
    {
        ImGui::SetNextItemWidth(width);
        if(!ImGui::BeginCombo("##RenderGraphTarget", mLabel.empty() ? "Main Graph" : mLabel.c_str()))
        {
            ImGui::SetItemTooltip("Graph to inspect: every viewport renders with its own copy of the scene passes");
            return;
        }

        if(ImGui::Selectable("Active Scene View", mMode == Mode::ACTIVE_SCENE_VIEW))
            mMode = Mode::ACTIVE_SCENE_VIEW;
        ImGui::SetItemTooltip("Follows the Scene view you hovered or focused last");
        ImGui::Separator();

        Core::GetRenderer()->ForEachView([this](RenderViewID id, const RenderViewDesc& desc) {
            ImGui::PushID(static_cast<int>(id));
            if(ImGui::Selectable(desc.DebugName.c_str(), mMode == Mode::VIEW && mView == id))
            {
                mMode = Mode::VIEW;
                mView = id;
            }
            ImGui::PopID();
        });

        ImGui::Separator();
        if(ImGui::Selectable("Main Graph", mMode == Mode::MAIN))
            mMode = Mode::MAIN;
        ImGui::SetItemTooltip("Presents the frame. In the Editor it only draws ImGui, its scene passes are skipped");
        ImGui::EndCombo();
    }

} // namespace Surge
