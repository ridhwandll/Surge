// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Surge/Core/Time/Timer.hpp"
#include "Surge/Core/Defines.hpp"

namespace Surge
{
    struct PanelData
    {
        IPanel* Panel = nullptr;
        bool Show = true;
    };

    class PanelManager
    {
    public:
        PanelManager() = default;
        ~PanelManager()
        {
            for (auto& [code, element] : mPanels)
            {
                element.Panel->Shutdown();
                delete element.Panel;
            }
        }

        template <typename T>
        T* PushPanel(void* panelInitArgs = nullptr)
        {
            static_assert(std::is_base_of_v<IPanel, T>, "T must derive from IPanel!");
            PanelData panelData {};
            panelData.Panel = new T;

            mPanels[T::GetStaticCode()] = panelData;
            panelData.Panel->Init(panelInitArgs);
            return static_cast<T*>(panelData.Panel);
        }

        template <typename T> //TODO: Switch to C++20 Concepts
        T* GetPanel() const
        {
            static_assert(std::is_base_of_v<IPanel, T>, "T must derive from IPanel!");
            auto itr = mPanels.find(T::GetStaticCode());
            if (itr != mPanels.end())
                return static_cast<T*>(itr->second.Panel);

            return nullptr;
        }

        void RenderAll()
        {
            for (auto& [code, element] : mPanels)
                element.Panel->Render(&element.Show);
        }

        void OnEvent(Event& e) const
        {
            for (auto& [code, element] : mPanels)
                element.Panel->OnEvent(e);
        }

        HashMap<PanelCode, PanelData>& GetAllPanels()
        {
            return mPanels;
        }

    private:
        HashMap<PanelCode, PanelData> mPanels;
    };

} // namespace Surge