#pragma once
#include "SurgeReflect/SurgeReflect.hpp"

namespace Surge
{
    class SurgeBehaviour
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

        // SURGE INTERNAL API [DO NOT TOUCH!]
        virtual void SetupInternalAPI(SurgeReflect::Registry* reg) = 0;
    };

} // namespace Surge

#define SURGE_REGISTER_SCRIPT(CLASS_NAME)                                               \
    extern "C"                                                                          \
    {                                                                                   \
        SCRIPT_API ::Surge::SurgeBehaviour* CreateScript(::SurgeReflect::Registry* reg) \
        {                                                                               \
            auto* script = new CLASS_NAME;                                              \
            script->SetupInternalAPI(reg);                                              \
            return script;                                                              \
        }                                                                               \
                                                                                        \
        SCRIPT_API void DestroyScript(::Surge::SurgeBehaviour* script)                  \
        {                                                                               \
            delete script;                                                              \
        }                                                                               \
                                                                                        \
        SCRIPT_API ::SurgeReflect::Class* GetReflection(::SurgeReflect::Registry* reg)  \
        {                                                                               \
            return ::SurgeReflect::GetReflectionFromRegistry<CLASS_NAME>(reg);          \
        }                                                                               \
    }
