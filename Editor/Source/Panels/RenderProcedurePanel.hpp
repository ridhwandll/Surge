// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"

namespace Surge
{
    class RenderProcedurePanel : public IPanel
    {
    public:
        RenderProcedurePanel() = default;
        virtual ~RenderProcedurePanel() override = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent(Event& e) override {};
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;

    public:
        static PanelCode GetStaticCode() { return PanelCode::RenderProcedure; }

    private:
        PanelCode mCode;
    };

} // namespace Surge
