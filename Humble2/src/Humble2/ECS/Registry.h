#pragma once

#include "Entity.h"
#include "IComponentStorage.h"
#include "SparseComponentStorage.h"

#include "ViewQuery.h"
#include "FilterQuery.h"
#include "StorageInfo.h"

#include "Core\Allocators.h"
#include "Scene\TypeResolver.h"

namespace HBL2
{
	class Registry
	{
	public:
        Registry() = default;

        Registry(uint32_t maxEntities, uint32_t maxComponents)
        {
            Initialize(maxEntities, maxComponents);
        }

        void Initialize(uint32_t maxEntities, uint32_t maxComponents)
        {
            m_MaxEntities = maxEntities;
            m_MaxComponents = maxComponents;

            uint64_t bytes = 0;

            // Bytes for EntityManager memory.
            bytes += maxEntities * sizeof(bool);    // m_Used
            bytes += maxEntities * sizeof(int32_t); // m_Gen
            bytes += maxEntities * sizeof(int32_t); // m_NextFree

            // Bytes for array for component storages.
            bytes += m_MaxComponents * sizeof(IComponentStorage*);
            bytes += m_MaxComponents * sizeof(void*);

            // Bytes for type resolver.
            bytes += m_MaxComponents * sizeof(std::type_index);
            bytes += m_MaxComponents * sizeof(std::uint32_t);

            uint64_t registryArenaBytes = bytes;

            // Bytes for the component storages, allocate enough as if all are SparseComponentStorage,
            // which is the default and worst case.
            for (uint32_t i = 0; i < m_MaxComponents; ++i)
            {
                bytes += maxEntities * sizeof(uint32_t);    // m_EntityToIndex
                bytes += maxEntities * sizeof(EntityRef);   // m_Entities
                bytes += maxEntities * 128_B;               // m_Packed (we choose 128 bytes as the worst case average size of components)

                // Bytes for the TwoLevelBitset.
                uint32_t l1Count = maxEntities / 64;
                uint32_t l0Count = maxEntities / 4096;

                bytes += l0Count * sizeof(uint64_t) + 63;
                bytes += l1Count * sizeof(uint64_t) + 63;
            }

            m_Reservation = Allocator::Arena.Reserve("RegistryPool", bytes);
            m_Arena.Initialize(&Allocator::Arena, registryArenaBytes, m_Reservation);

            m_TypeResolver.Initialize(&m_Arena, m_MaxComponents);
            InitializeEntities(&m_Arena);

            m_ComponentStorages = (IComponentStorage**)m_Arena.Alloc(m_MaxComponents * sizeof(IComponentStorage*));
            m_ConcreteStorages = (void**)m_Arena.Alloc(m_MaxComponents * sizeof(void*));

            std::memset(m_ConcreteStorages, 0, m_MaxComponents * sizeof(void*));
            std::memset(m_ComponentStorages, 0, m_MaxComponents * sizeof(IComponentStorage*));
        }

        void Clear()
        {
            for (uint32_t i = 0; i < m_MaxComponents; ++i)
            {
                if (m_ComponentStorages[i])
                {
                    m_ComponentStorages[i]->Clear();
                }
            }

            ClearEntities();
            m_TypeResolver.Clear();

            m_Arena.Destroy();
            m_Reservation = nullptr;
        }

        EntityRef CreateEntity()
        {
            int32_t slot = FindEmptyEntity();

            if (slot)
            {
                m_Used[slot] = true;
                m_Gen[slot] += 1;
                m_FirstFree = m_NextFree[slot];

                return { slot, m_Gen[slot] };
            }

            return EntityRef::Null;
        }

        EntityRef CreateEntity(EntityRef hint)
        {
            if (int32_t slot = hint.Idx)
            {
                if (m_Used[slot] == false)
                {
                    m_Used[slot] = true;
                    m_Gen[slot] += 1;

                    return { slot, m_Gen[slot] };
                }
            }

            return CreateEntity();
        }

        void DestroyEntity(EntityRef ref)
        {
            if (int32_t slot = DereferenceEntity(ref))
            {
                m_Used[slot] = false;

                m_NextFree[slot] = m_FirstFree;
                m_FirstFree = slot;
            }
        }

        bool IsValid(EntityRef ref)
        {
            return DereferenceEntity(ref) != 0;
        }

        template<typename T>
        IComponentStorage* GetStorage()
        {
            uint32_t id = m_TypeResolver.Resolve<T>();
            return m_ComponentStorages[id];
        }

        template<typename T>
        T& AddComponent(EntityRef e, T&& comp = {})
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            void* ptr = arr->Add(e);

            HBL2_CORE_ASSERT(ptr != nullptr, "Error while adding component!");

            return *(new(ptr) T(std::forward<T>(comp)));
        }

        template<typename T>
        T* TryAddComponent(EntityRef e, T&& comp = {})
        {
            if (!IsValid(e))
            {
                return nullptr;
            }

            IComponentStorage* arr = EnsureStorage<T>();
            void* ptr = arr->Add(e);

            if (!ptr)
            {
                return nullptr;
            }

            return (T*)(new(ptr) T(std::forward<T>(comp)));
        }

        template<typename T, typename... Args>
        T& EmplaceComponent(EntityRef e, Args&&... args)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            void* ptr = arr->Add(e);

            HBL2_CORE_ASSERT(ptr != nullptr, "Error while emplacing component!");

            return *(new(ptr) T(std::forward<Args>(args)...));
        }

        template<typename T, typename... Args>
        T* TryEmplaceComponent(EntityRef e, Args&&... args)
        {
            if (!IsValid(e))
            {
                return nullptr;
            }

            IComponentStorage* arr = EnsureStorage<T>();
            void* ptr = arr->Add(e);

            if (!ptr)
            {
                return nullptr;
            }

            return (T*)(new(ptr) T(std::forward<Args>(args)...));
        }

        template<typename T>
        T& GetComponent(EntityRef e)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            return *(T*)arr->Get(e);
        }

        template<typename T>
        T* TryGetComponent(EntityRef e)
        {
            if (!IsValid(e))
            {
                return nullptr;
            }

            IComponentStorage* arr = EnsureStorage<T>();

            if (!arr->Has(e))
            {
                return nullptr;
            }

            return (T*)arr->Get(e);
        }

        template<typename T>
        bool HasComponent(EntityRef e)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            return arr->Has(e);
        }

        template<typename T>
        void RemoveComponent(EntityRef e)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();

            if (!arr->Has(e))
            {
                return;
            }

            arr->Remove(e);
        }

        template<typename Component>
        ViewQuery<Component> Filter()
        {
            return ViewQuery<Component>(
                &m_TypeResolver,
                m_Gen,
                static_cast<StorageFor<std::remove_const_t<Component>>*>(EnsureStorage<std::remove_const_t<Component>>()),
                { m_ComponentStorages, m_MaxComponents }
            );
        }

        template<typename... Components> requires (sizeof...(Components) > 1)
        FilterQuery<Components...> Filter()
        {
            return FilterQuery<Components...>(
                &m_TypeResolver,
                m_Gen,
                std::make_tuple(static_cast<StorageFor<std::remove_const_t<Components>>*>(EnsureStorage<std::remove_const_t<Components>>())...),
                { m_ComponentStorages, m_MaxComponents }
            );
        }

        template <typename T>
        static std::vector<std::byte> Serialize(const T& component)
        {
            std::vector<std::byte> data(sizeof(T));
            std::memcpy(data.data(), &component, sizeof(T));
            return data;
        }

        template <typename T>
        static T Deserialize(const std::vector<std::byte>& data)
        {
            T component;
            std::memcpy(&component, data.data(), sizeof(T));
            return component;
        }

    private:
        template<typename T>
        IComponentStorage* EnsureStorage()
        {
            uint32_t id = m_TypeResolver.Resolve<T>();

            if (!m_ComponentStorages[id])
            {
                auto* storage = m_Arena.AllocConstruct<StorageFor<T>>(m_MaxEntities, m_Reservation);

                m_ComponentStorages[id] = storage;
                m_ConcreteStorages[id] = storage;
            }

            return m_ComponentStorages[id];
        }

    private:
        void InitializeEntities(Arena* arena)
        {
            m_Used = (bool*)arena->Alloc(m_MaxEntities * sizeof(bool));
            m_Gen = (int32_t*)arena->Alloc(m_MaxEntities * sizeof(int32_t));
            m_NextFree = (int32_t*)arena->Alloc(m_MaxEntities * sizeof(int32_t));

            std::memset(m_Used, 0, m_MaxEntities * sizeof(bool));
            std::memset(m_Gen, 0, m_MaxEntities * sizeof(int32_t));

            for (int i = 1; i < m_MaxEntities - 1; i++)
            {
                m_NextFree[i] = i + 1;
            }

            // Sentinels.
            m_NextFree[0] = 0;
            m_NextFree[m_MaxEntities - 1] = 0;
        }

        void ClearEntities()
        {
            m_Used = nullptr;
            m_Gen = nullptr;

            m_FirstFree = 0;
            m_NextFree = nullptr;

            m_MaxEntities = 0;
        }

        int32_t FindEmptyEntity() const
        {
            return m_FirstFree;
        }

        int32_t DereferenceEntity(EntityRef ref) const
        {
            if (ref.Idx > 0 && ref.Idx < m_MaxEntities && m_Used[ref.Idx] && ref.Gen != 0 && m_Gen[ref.Idx] == ref.Gen)
            {
                return ref.Idx;
            }

            return 0;
        }

	private:
        PoolReservation* m_Reservation = nullptr;
        Arena m_Arena;

        uint32_t m_MaxEntities = 0;
        uint32_t m_MaxComponents = 0;

        TypeResolver m_TypeResolver;

        // Entities.
        bool* m_Used = nullptr;
        int32_t* m_Gen = nullptr;
        int32_t m_FirstFree = 1;
        int32_t* m_NextFree = nullptr;

        // Storages.
        void** m_ConcreteStorages = nullptr;
        IComponentStorage** m_ComponentStorages = nullptr;
	};
}