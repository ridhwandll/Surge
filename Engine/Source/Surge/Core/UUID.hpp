// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#define NULL_UUID 0

namespace Surge
{
    class SURGE_API UUID
    {
    public:
        UUID();
        UUID(uint64_t id);
        UUID(const UUID& other);
        uint64_t Get() const { return mID; }

        operator uint64_t() { return mID; }
        operator const uint64_t() const { return mID; }

    private:
        uint64_t mID;
    };

} // namespace Surge

namespace std
{
    template <>
    struct hash<Surge::UUID>
    {
        std::size_t operator()(const Surge::UUID& uuid) const
        {
            return hash<uint64_t>()((uint64_t)uuid);
        }
    };

} // namespace std
