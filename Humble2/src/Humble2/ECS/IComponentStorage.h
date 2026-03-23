#pragma once

#include "Entity.h"
#include "TwoLevelBitset.h"

#include "Utilities\Collections\StaticFunction.h"

namespace HBL2
{
    class IComponentStorage
    {
    public:
        virtual ~IComponentStorage() = default;

        virtual void* Add(Entity e) = 0;
        virtual void Remove(Entity e) = 0;
        virtual void* Get(Entity e) = 0;
        virtual bool Has(Entity e) const = 0;
        virtual void Iterate(StaticFunction<void(void*), 128>&& func) = 0;
        virtual void Clear() = 0;

        virtual const std::type_info& TypeInfo() const = 0;

        inline const TwoLevelBitset& Mask() const { return m_Mask; }

    protected:
        TwoLevelBitset m_Mask;
    };
}