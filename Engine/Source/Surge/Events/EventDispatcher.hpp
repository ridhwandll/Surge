// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Events/Event.hpp"
#include <functional>
#include <map>

namespace Surge
{
    using EventSubscribtion = std::function<void()>;
    using EventMap = std::multimap<const std::type_info*, const std::function<void(const Event&)>>;

    class EventDispatcher
    {
    public:
        template<typename T>
        static void Subscribe(const std::function<void(const Event&)>& function)
        {
            mEventMap.emplace(&typeid(T), function);
        }

        static void Emit(const Event& event)
        {
            auto range = mEventMap.equal_range(&typeid(event));
            for (auto it = range.first; it != range.second; ++it)
                it->second(event);
        }
    private:
        inline static EventMap mEventMap;
    };
}