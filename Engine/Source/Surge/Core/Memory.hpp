// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Core.hpp"

namespace Surge
{
    class SURGE_API RefCounted
    {
    public:
        void IncRefCount() const
        {
            mRefCount++;
        }
        void DecRefCount() const
        {
            mRefCount--;
        }
        void ZeroRefCount() const
        {
            mRefCount = 0;
        }

        void IncWeakRefCount() const
        {
            mWeakRefCount++;
        }
        void DecWeakRefCount() const
        {
            mWeakRefCount--;
        }
        void ZeroWeakRefCount() const
        {
            mWeakRefCount = 0;
        }

        Uint GetRefCount() const { return mRefCount; }
    private:
        mutable Uint mRefCount = 0;
        mutable Uint mWeakRefCount = 0; // NOTE(AC3R): This is mostly for debugging purposes, the std::weak_ptr also has a weakref count
    };

    template<typename T>
    class SURGE_API Ref
    {
    public:
        /* CONSTRUCTORS */
        Ref()
            : mInstance(nullptr) {}

        Ref(std::nullptr_t n)
            : mInstance(nullptr) {}

        Ref(T* instance)
            : mInstance(instance)
        {
            static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");
            IncRef();
        }

        template<typename Ts>
        Ref(const Ref<Ts>& other)
        {
            mInstance = (T*)other.mInstance;
            IncRef();
        }

        Ref(const Ref<T>& other)
            : mInstance(other.mInstance)
        {
            IncRef();
        }

        template<typename Ts>
        Ref(Ref<Ts>&& other)
        {
            mInstance = (T*)other.mInstance;
            other.mInstance = nullptr;
        }

        /* COPY OPERATOR */
        Ref& operator=(std::nullptr_t)
        {
            DecRef();
            mInstance = nullptr;
            return *this;
        }

        Ref& operator=(const Ref<T>& other)
        {
            other.IncRef();
            DecRef();

            mInstance = other.mInstance;
            return *this;
        }

        template<typename Ts>
        Ref& operator=(const Ref<Ts>& other)
        {
            other.IncRef();
            DecRef();

            mInstance = other.mInstance;
            return *this;
        }

        /* MOVE OPERATOR */
        template<typename Ts>
        Ref& operator=(Ref<Ts>&& other)
        {
            DecRef();

            mInstance = other.mInstance;
            other.mInstance = nullptr;
            return *this;
        }

        /* OTHER OPERATORS */
        operator bool() { return mInstance != nullptr; }
        operator bool() const { return mInstance != nullptr; }

        T* operator->() { return mInstance; }
        const T* operator->() const { return mInstance; }

        T& operator*() { return *mInstance; }
        const T& operator*() const { return *mInstance; }


        /* FUNCTIONS */
        T* Raw() { return mInstance; }
        [[nodiscard]] const T* Raw() const { return mInstance; }

        void Release()
        {
            delete mInstance;
            mInstance->ZeroRefCount();
        }

        void Reset(T* instance = nullptr)
        {
            DecRef();
            mInstance = instance;
        }

        template<typename... Args>
        static Ref<T> Create(Args&&... args)
        {
            return Ref<T>(new T(std::forward<Args>(args)...));
        }

        template<typename Ts>
        [[nodiscard]] Ref<Ts> As() const
        {
            return Ref<Ts>(*this);
        }
        

        /* DESCTRUCTOR */
        ~Ref()
        {
            DecRef();
        }

    private:
        void IncRef() const
        {
            if (mInstance)
                mInstance->IncRefCount();
        }

        void DecRef() const
        {
            if (mInstance)
            {
                mInstance->DecRefCount();
                if (mInstance->GetRefCount() == 0)
                    delete mInstance;
            }
        }

        friend class Ref;
        T* mInstance;
    };

    template<typename T>
    class SURGE_API WeakRef
    {
    public:

        /* CONSTRUCTORS */
        WeakRef()
            : mInstance(nullptr) {}

        WeakRef(std::nullptr_t n)
            : mInstance(nullptr) {}

        WeakRef(Ref<T> instance)
            : mInstance(instance.Raw())
        {
            static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");
            IncWeakRef();
        }

        WeakRef(Ref<T>&& other)
        {
            mInstance = other.Raw();
            other.Raw() = nullptr;
        }

        /* COPY OPERATOR */
        WeakRef& operator=(Ref<T> other)
        {
            DecWeakRef();
            other.Raw()->IncWeakRefCount();

            mInstance = other.Raw();
            return *this;
        }
        
        WeakRef& operator=(const WeakRef<T>& other)
        {
            DecWeakRef();
            other.mInstance->IncWeakRefCount();

            mInstance = other.mInstance;
            return *this;
        }

        /* MOVE OPERATOR */
        WeakRef& operator=(Ref<T>&& other)
        {
            DecWeakRef();

            mInstance = other.Raw();
            other.Raw() = nullptr;
            return *this;
        }

        WeakRef& operator=(WeakRef<T>&& other)
        {
            DecWeakRef();

            mInstance = other.mInstance;
            other.mInstance = nullptr;
            return *this;
        }


        /* OTHER OPERATORS */
        operator bool() { return mInstance != nullptr; }
        operator bool() const { return mInstance != nullptr; }

        T* operator->() { return mInstance; }
        const T* operator->() const { return mInstance; }

        T& operator*() { return *mInstance; }
        const T& operator*() const { return *mInstance; }


        /* FUNCTIONS */
        bool Expired() const { return mInstance != nullptr; }
        void Reset() noexcept { mInstance = nullptr; }

        Ref<T> AsRef() { return Ref<T>(mInstance); }

        ~WeakRef()
        {
            DecWeakRef();
        }
    private:
        void IncWeakRef() const
        {
            if (mInstance)
                mInstance->IncWeakRefCount();
        }

        void DecWeakRef() const
        {
            if (mInstance)
                mInstance->DecWeakRefCount();
        }

        friend class WeakRef;
        T* mInstance;
    };
}
