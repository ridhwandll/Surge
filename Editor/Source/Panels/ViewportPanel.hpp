// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include <glm/glm.hpp>

namespace Surge
{
    class ViewportPanel : public IPanel
    {
    public:
        ViewportPanel() = default;
        virtual ~ViewportPanel() override = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;
        virtual PanelCode GetCode() override { return GetStaticCode(); }

        const glm::vec2& GetViewportSize() const { return mViewportSize; }

    public:
        static PanelCode GetStaticCode() { return PanelCode::Viewport; }

    private:
        glm::vec2 mViewportSize = glm::vec2(0.0f);
    };

} // namespace Surge
