// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Utility/ImGuiAux.hpp"

namespace Surge
{
    class ProjectSettingsPanel : public IPanel
    {
    public:
        ProjectSettingsPanel() = default;
        virtual ~ProjectSettingsPanel() override = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent(Event& e) override {};
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;

    public:
        static PanelCode GetStaticCode() { return PanelCode::ProjectSettings; }

    private:
        PanelCode mCode;
        UUID mSelectedSceneUUID;

        ImGuiAux::RenamingMechanism mRenamingMech;
    };

} // namespace Surge
