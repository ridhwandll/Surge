// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Events/Event.hpp"

namespace Surge
{
    struct ClientOptions
    {
        bool EnableImGui = true;
    };

    class Client
    {
    public:
        Client() = default;
        virtual ~Client() = default;

        virtual void OnInitialize() {};
        virtual void OnUpdate() {};
        virtual void OnEvent(Event& e) {};
        virtual void OnImGuiRender() {};
        virtual void OnShutdown() {};

        void SetOptions(const ClientOptions& appCreateInfo) { mClientOptions = appCreateInfo; }
        const ClientOptions& GetAppOptions() const { return mClientOptions; }

    private:
        ClientOptions mClientOptions;
    };

    template <typename T>
    inline T* MakeClient()
    {
        static_assert(std::is_base_of_v<Client, T>, "Class MUST derive from Surge::Client");
        T* client = new T();
        return client;
    }

} // namespace Surge