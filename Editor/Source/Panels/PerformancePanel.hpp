// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"

namespace Surge
{
    class PerformancePanel : public IPanel
    {
    public:
        PerformancePanel() = default;
        virtual ~PerformancePanel() override = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;
        virtual PanelCode GetCode() const override { return GetStaticCode(); }

    public:
        static PanelCode GetStaticCode() { return PanelCode::Performance; }

    private:
        PanelCode mCode;
    };
} // namespace Surge
