// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderProcedure/RenderProcedure.hpp"
#include "SurgeReflect/SurgeReflect.hpp"

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
                return itr->second;

            return nullptr;
        }

        FORCEINLINE void UpdateAll()
        {
            for (auto& [hash, proc] : mProcedures)
                proc->Update();
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

        FORCEINLINE void Shutdown()
        {
            for (auto& [hash, proc] : mProcedures)
            {
                proc->Shutdown();
                delete proc;
            }

            mProcedures.clear();
        }

    private:
        RendererData* mRendererData;
        HashMap<SurgeReflect::ClassHash, RenderProcedure*> mProcedures;
    };

} // namespace Surge
