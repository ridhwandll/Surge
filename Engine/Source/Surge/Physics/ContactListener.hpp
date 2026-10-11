// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Vector.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <mutex>
#include <unordered_map>

namespace Surge
{
    enum class CollisionEventType : uint8_t
    {
        ENTER,
        EXIT
    };

    struct CollisionEvent
    {
        uint64_t EnttIDA;
        uint64_t EnttIDB;
        CollisionEventType Type = CollisionEventType::ENTER;
        bool IsTrigger = false; // One of the bodies is a sensor
    };

    class ContactListener : public JPH::ContactListener
    {
    public:
        virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
        virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
        virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

        // Main thread: a body is being destroyed, its open contacts end now (Jolt may not report them anymore)
        void OnBodyRemoved(uint32_t bodyID);

        // Main thread: drops every pending event and open contact (a new play session starts)
        void Reset();

        // Called by the Main Thread to get the events and clear the queue
        Vector<CollisionEvent> FlushEvents();

    private:
        // Jolt reports contacts per sub shape pair, counted per body pair: ENTER on the first, EXIT when the last one ends
        struct BodyPairContact
        {
            uint64_t EnttIDA;
            uint64_t EnttIDB;
            Uint SubShapeContacts = 0;
            bool IsTrigger = false;
        };

        static uint64_t PairKey(uint32_t bodyA, uint32_t bodyB);

    private:
        std::mutex mMutex; // Jolt calls Added/Removed from its worker threads
        Vector<CollisionEvent> mCollisionQueue;
        std::unordered_map<uint64_t, BodyPairContact> mOpenContacts;
    };
}
