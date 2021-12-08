// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderProcedure/RenderProcedureManager.hpp"

namespace Surge
{
    void RenderProcedureManager::UpdateAll()
    {
        SURGE_PROFILE_FUNC("RenderProcedureManager::UpdateAll");
        SG_ASSERT(!mProcOrder.empty(), "Empty ProcOrder! Have you forgot to call Sort()?");

        for (const SurgeReflect::ClassHash& hash : mProcOrder)
        {
            auto& [isActive, procedure] = mProcedures.at(hash);
            if (!isActive)
                continue;

            procedure->Update();
        }
    }

} // namespace Surge
