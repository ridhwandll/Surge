// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/UUID.hpp"
#include <random>

namespace Surge
{
    static std::random_device sRandomDevice;
    static std::mt19937_64 engine(sRandomDevice());
    static std::uniform_int_distribution<uint64_t> sUniformDistribution;

    UUID::UUID()
        : mID(sUniformDistribution(engine)) {}

    UUID::UUID(uint64_t id)
        : mID(id) {}

    UUID::UUID(const UUID& other)
        : mID(other.mID) {}

} // namespace Surge
