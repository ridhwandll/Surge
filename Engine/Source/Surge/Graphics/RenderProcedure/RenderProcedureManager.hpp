// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderProcedure/RenderProcedure.hpp"
#include "SurgeReflect/SurgeReflect.hpp"
#include "Surge/Debug/Profiler.hpp"

namespace Surge
{
    class RenderProcedureManager
    {
    public:
        RenderProcedureManager() = default;
        ~RenderProcedureManager() = default;

        FORCEINLINE void Init(Scope<RendererData>& data)
        {
            SG_ASSERT_NOMSG(data);
            mRendererData = data.get();
        }

        template <typename... Procedures>
        FORCEINLINE void Sort()
        {
            (mProcOrder.push_back(SurgeReflect::GetReflection<Procedures>()->GetHash()), ...);
        }

        template <typename T> // TODO: Use C++20 "concept"s when we switch
        FORCEINLINE T* AddProcedure()
        {
            static_assert(std::is_base_of<RenderProcedure, T>::value, "Class must derive from RenderProcedure");

            T* procInstance = new T();
            procInstance->Init(mRendererData);

            const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<T>();
            SG_ASSERT(clazz, "No Reflection found!");
            const SurgeReflect::ClassHash& hash = clazz->GetHash();

            mProcedures[hash] = procInstance;
            return procInstance;
        }

        template <typename T> // TODO: Use C++20 "concept"s when we switch
        FORCEINLINE T* GetProcedure()
        {
            static_assert(std::is_base_of<RenderProcedure, T>::value, "Class must derive from RenderProcedure");
            const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<T>();
            SG_ASSERT(clazz, "No Reflection found!");
            const SurgeReflect::ClassHash& hash = clazz->GetHash();

            auto itr = mProcedures.find(hash);
            if (itr != mProcedures.end())
                return static_cast<T*>(itr->second);

            return nullptr;
        }

        FORCEINLINE void UpdateAll()
        {
            SURGE_PROFILE_FUNC("RenderProcedureManager::UpdateAll");
            SG_ASSERT(!mProcOrder.empty(), "Empty ProcOrder! Have you forgot to call Sort()?");

            for (const SurgeReflect::ClassHash& hash : mProcOrder)
            {
                RenderProcedure* procedure = mProcedures.at(hash);
                procedure->Update();
            }
        }

        template <typename T>
        FORCEINLINE typename T::InternalData* GetRenderProcData()
        {
            static_assert(std::is_base_of<RenderProcedure, T>::value, "Class must derive from RenderProcedure");
            const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<T>();
            SG_ASSERT(clazz, "No Reflection found!");
            const SurgeReflect::ClassHash& hash = clazz->GetHash();

            auto itr = mProcedures.find(hash);
            if (itr != mProcedures.end())
                return static_cast<typename T::InternalData*>(itr->second->GetInternalDataBlock());

            return nullptr;
        }

        template <typename T>
        void RestartProcedure()
        {
            Surge::Core::AddFrameEndCallback([this]() {
                T* proc = GetProcedure<T>();
                SG_ASSERT(proc, "Attempted to restart invalid Procedure!");

                proc->Shutdown();
                proc->Init(mRendererData);
            });
        }

        FORCEINLINE void
        Shutdown()
        {
            for (const SurgeReflect::ClassHash& hash : mProcOrder)
            {
                RenderProcedure* procedure = mProcedures.at(hash);
                procedure->Shutdown();
                delete procedure;
            }

            mProcOrder.clear();
            mProcedures.clear();
        }

    private:
        RendererData* mRendererData;
        Vector<SurgeReflect::ClassHash> mProcOrder;
        HashMap<SurgeReflect::ClassHash, RenderProcedure*> mProcedures;
    };

} // namespace Surge
