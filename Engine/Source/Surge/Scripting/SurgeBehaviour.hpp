#pragma once
#include "SurgeReflect/SurgeReflect.hpp"
#include "Surge/Core/Core.hpp"

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
        virtual void SetupInternalAPI(Core::CoreData* engineData, SurgeReflect::Registry* reg) = 0;

    protected:
        Core::CoreData* mSurgeEngine;
    };

} // namespace Surge

#define SURGE_REGISTER_SCRIPT(CLASS_NAME)                                                                           \
    extern "C"                                                                                                      \
    {                                                                                                               \
        SCRIPT_API ::Surge::SurgeBehaviour* CreateScript(Core::CoreData* engineData, ::SurgeReflect::Registry* reg) \
        {                                                                                                           \
            auto* script = new CLASS_NAME;                                                                          \
            script->SetupInternalAPI(engineData, reg);                                                              \
            return script;                                                                                          \
        }                                                                                                           \
                                                                                                                    \
        SCRIPT_API void DestroyScript(::Surge::SurgeBehaviour* script)                                              \
        {                                                                                                           \
            delete script;                                                                                          \
        }                                                                                                           \
                                                                                                                    \
        SCRIPT_API ::SurgeReflect::Class* GetReflection(::SurgeReflect::Registry* reg)                              \
        {                                                                                                           \
            return ::SurgeReflect::GetReflectionFromRegistry<CLASS_NAME>(reg);                                      \
        }                                                                                                           \
    }

#define SURGE_SCRIPT_REFLECTION_BEGIN(ClassName)                                                    \
public:                                                                                             \
    virtual void SetupInternalAPI(Core::CoreData* engineData, SurgeReflect::Registry* reg) override \
    {                                                                                               \
        mSurgeEngine = engineData;                                                                  \
        SurgeReflect::Class& clazz = SurgeReflect::Class(#ClassName);                               \
        clazz

#define SURGE_SCRIPT_REFLECTION_END(ClassName)      \
    ;                                               \
    reg->RegisterReflectionClass(std::move(clazz)); \
    }
