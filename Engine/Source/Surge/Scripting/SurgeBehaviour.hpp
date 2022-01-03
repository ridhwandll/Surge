#pragma once
#include "SurgeReflect/SurgeReflect.hpp"

namespace Surge
{
    class SURGE_API SurgeBehaviour
    {
    public:
        SurgeBehaviour() = default;
        virtual ~SurgeBehaviour() = default;

        // Invoked once on runtime start
        virtual void OnStart() {};

        // Invoked per frame
        virtual void OnUpdate() {};

        // Invoked once on runtime end
        virtual void OnDestroy() {};
    };

} // namespace Surge

#define SURGE_REGISTER_SCRIPT(CLASS_NAME)                              \
    extern "C"                                                         \
    {                                                                  \
        SCRIPT_API ::Surge::SurgeBehaviour* CreateScript()             \
        {                                                              \
            auto* script = new CLASS_NAME;                             \
            return script;                                             \
        }                                                              \
                                                                       \
        SCRIPT_API void DestroyScript(::Surge::SurgeBehaviour* script) \
        {                                                              \
            delete script;                                             \
        }                                                              \
                                                                       \
        SCRIPT_API ::SurgeReflect::Class* GetReflection()              \
        {                                                              \
            return ::SurgeReflect::GetReflection<CLASS_NAME>();        \
        }                                                              \
    }
