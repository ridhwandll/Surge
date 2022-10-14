#pragma once
#include "SurgeReflect/SurgeReflect.hpp"
#include "Surge/ECS/Scene.hpp"

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

        Entity& GetEntity() { return mEntity; }

    protected:
        Entity mEntity;
    };

} // namespace Surge

#define SURGE_REGISTER_SCRIPT(CLASS_NAME)                              \
    extern "C"                                                         \
    {                                                                  \
        SCRIPT_API ::Surge::SurgeBehaviour* CreateScript(Entity& e)    \
        {                                                              \
            auto* script = new CLASS_NAME;                             \
            script->GetEntity() = e;                                   \
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
    }                                                                  \
    SURGE_REFLECT_CLASS_REGISTER_BEGIN(CLASS_NAME)                     \
    .AddFunction<&CLASS_NAME::OnStart>("OnStart")                      \
    .AddFunction<&CLASS_NAME::OnUpdate>("OnUpdate")                    \
    .AddFunction<&CLASS_NAME::OnDestroy>("OnDestroy")                  \
    SURGE_REFLECT_CLASS_REGISTER_END(CLASS_NAME)                       \

