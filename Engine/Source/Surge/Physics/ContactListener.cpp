// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ContactListener.hpp"
#include <Jolt/Physics/Collision/Shape/SubShapeIDPair.h>
#include <algorithm>

namespace Surge
{
    uint64_t ContactListener::PairKey(uint32_t bodyA, uint32_t bodyB)
    {
        const uint64_t low = std::min(bodyA, bodyB);
        const uint64_t high = std::max(bodyA, bodyB);
        return (high << 32) | low;
    }

    JPH::ValidateResult ContactListener::OnContactValidate(const JPH::Body& /*inBody1*/, const JPH::Body& /*inBody2*/, JPH::RVec3Arg /*inBaseOffset*/, const JPH::CollideShapeResult& /*inCollisionResult*/)
    {
        // Accept all collisions by default
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void ContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& /*inManifold*/, JPH::ContactSettings& /*ioSettings*/)
    {
        const uint64_t key = PairKey(inBody1.GetID().GetIndexAndSequenceNumber(), inBody2.GetID().GetIndexAndSequenceNumber());

        std::lock_guard<std::mutex> lock(mMutex);
        BodyPairContact& contact = mOpenContacts[key];
        if(contact.SubShapeContacts++ > 0)
            return; // The pair already touches through another sub shape

        contact.EnttIDA = static_cast<uint64_t>(inBody1.GetUserData());
        contact.EnttIDB = static_cast<uint64_t>(inBody2.GetUserData());
        contact.IsTrigger = inBody1.IsSensor() || inBody2.IsSensor();
        mCollisionQueue.push_back({ contact.EnttIDA, contact.EnttIDB, CollisionEventType::ENTER, contact.IsTrigger });
    }

    void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
    {
        // Bodies may already be gone here, everything needed was stored when the contact started
        const uint64_t key = PairKey(inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber(), inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber());

        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mOpenContacts.find(key);
        if(it == mOpenContacts.end())
            return;

        if(--it->second.SubShapeContacts == 0)
        {
            mCollisionQueue.push_back({ it->second.EnttIDA, it->second.EnttIDB, CollisionEventType::EXIT, it->second.IsTrigger });
            mOpenContacts.erase(it);
        }
    }

    void ContactListener::OnBodyRemoved(uint32_t bodyID)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        for(auto it = mOpenContacts.begin(); it != mOpenContacts.end();)
        {
            const uint32_t low = static_cast<uint32_t>(it->first & 0xFFFFFFFFull);
            const uint32_t high = static_cast<uint32_t>(it->first >> 32);
            if(low == bodyID || high == bodyID)
            {
                mCollisionQueue.push_back({ it->second.EnttIDA, it->second.EnttIDB, CollisionEventType::EXIT, it->second.IsTrigger });
                it = mOpenContacts.erase(it);
            }
            else
                ++it;
        }
    }

    void ContactListener::Reset()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mCollisionQueue.clear();
        mOpenContacts.clear();
    }

    Surge::Vector<Surge::CollisionEvent> ContactListener::FlushEvents()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        Surge::Vector<Surge::CollisionEvent> events = mCollisionQueue;
        mCollisionQueue.clear();
        return events;
    }

}
