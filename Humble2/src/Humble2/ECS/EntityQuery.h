#pragma once

#include "Entity.h"

#include "Utilities\Collections\StaticFunction.h"

namespace HBL2
{
	class EntityQuery
	{
    public:
        EntityQuery(int32_t* generations, bool* used, uint32_t maxEntities)
            : m_Generations(generations), m_Used(used), m_MaxEntities(maxEntities)
        {
        }

        void ForEach(StaticFunction<void(Entity), 128>&& func) const
        {
            for (uint32_t i = 1; i < m_MaxEntities; ++i)
            {
                if (m_Used[i])
                {
                    func({ (int32_t)i, m_Generations[i] });
                }
            }
        }

    private:
        int32_t* m_Generations = nullptr;
        bool* m_Used = nullptr;
        uint32_t  m_MaxEntities = 0;
    };
}