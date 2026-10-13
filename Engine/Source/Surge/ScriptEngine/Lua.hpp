// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

// Must be defined before ANY inclusion of sol.hpp
#define SOL_USING_CXX_LUA   0    // Lua compiled as C, not C++
#define SOL_PRINT_ERRORS    0

#ifdef SURGE_DEBUG
#define SOL_ALL_SAFETIES_ON 1
#else
#define SOL_SAFE_NUMERICS        0
#define SOL_SAFE_GETTER          0
#define SOL_SAFE_FUNCTION        0
#define SOL_SAFE_FUNCTION_CALLS  0
#define SOL_SAFE_USERTYPE        0
#endif

#include <sol2/sol.hpp>
#include "Surge/Core/Memory.hpp"

namespace sol
{
    // Ref<T> handed to Lua keeps the object alive (assets, widgets) for as long as Lua holds it
    template <typename T>
    struct unique_usertype_traits<Surge::Ref<T>>
    {
        typedef T type;
        typedef Surge::Ref<T> actual_type;
        static const bool is_shared = true;
        static bool is_null(const actual_type& ptr) { return !ptr; }
        static type* get(const actual_type& ptr) { return const_cast<type*>(ptr.get()); }
    };
}
