// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Panels/SceneHierarchyPanel.hpp"

namespace Surge
{
    class InspectorPanel : public IPanel
    {
    public:
        InspectorPanel() = default;
        ~InspectorPanel() = default;

        virtual void Init(void* panelInitArgs);
        virtual void OnEvent(Event& e) override {};
        virtual void Render(bool* show);
        virtual void Shutdown();

        static PanelCode GetStaticCode() { return PanelCode::Inspector; }
        void SetHierarchy(SceneHierarchyPanel* hierarchy) { mHierarchy = hierarchy; }

    private:
        void DrawComponents(Entity& entity);

    private:
        PanelCode mCode;
        SceneHierarchyPanel* mHierarchy;
    };
} // namespace Surge