// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t id);
        UUID(const UUID& other);

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
