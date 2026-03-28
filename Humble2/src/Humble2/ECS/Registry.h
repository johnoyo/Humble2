#pragma once

#include "Entity.h"
#include "IComponentStorage.h"
#include "SparseComponentStorage.h"

#include "ViewQuery.h"
#include "FilterQuery.h"
#include "EntityQuery.h"
#include "StorageInfo.h"
#include "TypeResolver.h"

#include "Core\Allocators.h"

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
            bytes += m_MaxComponents * sizeof(512_B) * 16; // Reserve space for allocating the storages (in the EnsureArray method).

            // Bytes for type resolver.
            bytes += m_MaxComponents * sizeof(std::type_index);
            bytes += m_MaxComponents * sizeof(uint32_t);
            bytes += m_MaxComponents * sizeof(uint32_t);

            uint64_t registryArenaBytes = bytes;

            // Bytes for the component storages, allocate enough as if all are SparseComponentStorage,
            // which is the default and worst case.
            for (uint32_t i = 0; i < m_MaxComponents; ++i)
            {
                bytes += maxEntities * sizeof(uint32_t); // m_EntityToIndex
                bytes += maxEntities * sizeof(Entity);   // m_Entities
                bytes += maxEntities * 128_B;            // m_Packed (we choose 128 bytes as the worst case average size of components)

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

                    m_ComponentStorages[i] = nullptr;
                    m_ConcreteStorages[i] = nullptr;
                }
            }

            ClearEntities();
            m_TypeResolver.Clear();

            m_Arena.Destroy();
            m_Reservation = nullptr;
        }

        Entity CreateEntity()
        {
            int32_t slot = FindEmptyEntity();

            if (slot)
            {
                m_Used[slot] = true;
                m_Gen[slot] += 1;
                m_FirstFree = m_NextFree[slot];

                return { slot, m_Gen[slot] };
            }

            return Entity::Null;
        }

        Entity CreateEntity(Entity hint)
        {
            if (int32_t slot = hint.Idx)
            {
                if (m_Used[slot] == false)
                {
                    m_Used[slot] = true;
                    m_Gen[slot] += 1;
                    m_FirstFree = m_NextFree[slot];

                    return { slot, m_Gen[slot] };
                }
            }

            return CreateEntity();
        }

        void DestroyEntity(Entity e)
        {
            if (int32_t slot = DereferenceEntity(e))
            {
                // Mark entity as unused and add it to the free list.
                m_Used[slot] = false;
                m_NextFree[slot] = m_FirstFree;
                m_FirstFree = slot;

                // Remove the entity from all the component storages.
                for (uint32_t i = 0; i < m_MaxComponents; ++i)
                {
                    if (m_ComponentStorages[i])
                    {
                        if (m_ComponentStorages[i]->Has(e))
                        {
                            m_ComponentStorages[i]->Remove(e);
                        }
                    }
                }
            }
        }

        bool IsValid(Entity e)
        {
            return DereferenceEntity(e) != 0;
        }

        template<typename T>
        IComponentStorage* GetStorage()
        {
            uint32_t id = m_TypeResolver.Resolve<T>();
            return m_ComponentStorages[id];
        }

        template<typename T>
        void ClearStorage()
        {
            uint32_t id = m_TypeResolver.Resolve<T>();

            if (m_ComponentStorages[id])
            {
                m_ComponentStorages[id]->Clear();

                m_ComponentStorages[id] = nullptr;
                m_ConcreteStorages[id] = nullptr;
            }
        }

        TypeResolver& GetTypeResolver()
        {
            return m_TypeResolver;
        }

        template<typename T>
        T& AddComponent(Entity e, T&& comp = {})
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            void* ptr = arr->Add(e);

            HBL2_CORE_ASSERT(ptr != nullptr, "Error while adding component!");

            return *(new(ptr) T(std::forward<T>(comp)));
        }

        template<typename T>
        T* TryAddComponent(Entity e, T&& comp = {})
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
        T& EmplaceComponent(Entity e, Args&&... args)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            void* ptr = arr->Add(e);

            HBL2_CORE_ASSERT(ptr != nullptr, "Error while emplacing component!");

            return *(new(ptr) T(std::forward<Args>(args)...));
        }

        template<typename T, typename... Args>
        T* TryEmplaceComponent(Entity e, Args&&... args)
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
        T& GetComponent(Entity e)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            return *(T*)arr->Get(e);
        }

        template<typename T>
        T* TryGetComponent(Entity e)
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
        bool HasComponent(Entity e)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();
            return arr->Has(e);
        }

        template<typename T>
        void RemoveComponent(Entity e)
        {
            HBL2_CORE_ASSERT(IsValid(e), "Error while adding component, invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();

            if (!arr->Has(e))
            {
                return;
            }

            arr->Remove(e);
        }

        template<typename T>
        T& GetOrAddComponent(Entity e)
        {
            HBL2_CORE_ASSERT(IsValid(e), "GetOrAddComponent: invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();

            if (arr->Has(e))
            {
                return *(T*)arr->Get(e);
            }

            void* ptr = arr->Add(e);
            HBL2_CORE_ASSERT(ptr != nullptr, "GetOrAddComponent: Add failed!");
            return *(new(ptr) T{});
        }

        template<typename T>
        T& AddOrReplaceComponent(Entity e, T&& comp = {})
        {
            HBL2_CORE_ASSERT(IsValid(e), "AddOrReplaceComponent: invalid entity!");

            IComponentStorage* arr = EnsureStorage<T>();

            if (arr->Has(e))
            {
                T& existing = *(T*)arr->Get(e);
                existing = std::forward<T>(comp);
                return existing;
            }

            void* ptr = arr->Add(e);
            HBL2_CORE_ASSERT(ptr != nullptr, "AddOrReplaceComponent: Add failed!");
            return *(new(ptr) T(std::forward<T>(comp)));
        }

        EntityQuery Entities() const
        {
            return EntityQuery(m_Gen, m_Used, m_MaxEntities);
        }

        template<typename Component>
        ViewQuery<Component> Filter()
        {
            return ViewQuery<Component>(
                &m_TypeResolver,
                m_Gen,
                static_cast<StorageFor<std::remove_const_t<Component>>*>(EnsureStorage<std::remove_const_t<Component>>()),
                m_ComponentStorages
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
            static_assert(!std::is_pointer_v<T>, "Cannot serialize pointer types");

            std::vector<std::byte> buffer;

            // Count fields first
            uint32_t fieldCount = 0;
            T::schema.ForEach(component, [&](std::string_view, auto&) { ++fieldCount; });

            // Write field count
            buffer.resize(sizeof(uint32_t));
            std::memcpy(buffer.data(), &fieldCount, sizeof(uint32_t));

            // Write each field: [name_hash][data_size][data]
            T::schema.ForEach(component, [&](std::string_view name, auto& value)
            {
                using FieldT = std::remove_reference_t<decltype(value)>;

                static_assert(std::is_trivially_copyable_v<FieldT>, "Field is not trivially copyable — implement a custom serializer for this type");

                uint64_t nameHash = HashFieldName(name);
                uint32_t dataSize = static_cast<uint32_t>(sizeof(FieldT));

                size_t offset = buffer.size();
                buffer.resize(offset + sizeof(uint64_t) + sizeof(uint32_t) + dataSize);

                std::memcpy(buffer.data() + offset, &nameHash, sizeof(uint64_t));
                std::memcpy(buffer.data() + offset + sizeof(uint64_t), &dataSize, sizeof(uint32_t));
                std::memcpy(buffer.data() + offset + sizeof(uint64_t) + sizeof(uint32_t), &value, dataSize);
            });

            return buffer;
        }

        template <typename T>
        static T Deserialize(const std::vector<std::byte>& buffer)
        {
            T component{}; // default-initialize — fields not in buffer keep defaults

            if (buffer.size() < sizeof(uint32_t))
            {
                return component;
            }

            // Read field count
            uint32_t fieldCount = 0;
            std::memcpy(&fieldCount, buffer.data(), sizeof(uint32_t));

            // Build a lookup table: nameHash -> (offset into buffer, size)
            struct FieldRecord { size_t offset; uint32_t size; };
            std::unordered_map<uint64_t, FieldRecord> records;
            records.reserve(fieldCount);

            size_t cursor = sizeof(uint32_t);
            for (uint32_t i = 0; i < fieldCount && cursor < buffer.size(); ++i)
            {
                if (cursor + sizeof(uint64_t) + sizeof(uint32_t) > buffer.size())
                {
                    break;
                }

                uint64_t nameHash = 0;
                uint32_t dataSize = 0;
                std::memcpy(&nameHash, buffer.data() + cursor, sizeof(uint64_t));
                std::memcpy(&dataSize, buffer.data() + cursor + sizeof(uint64_t), sizeof(uint32_t));

                cursor += sizeof(uint64_t) + sizeof(uint32_t);

                if (cursor + dataSize <= buffer.size())
                {
                    records[nameHash] = { cursor, dataSize };
                }

                cursor += dataSize;
            }

            // Apply each field from the new schema — unknown fields keep default value
            T::schema.ForEach(component, [&](std::string_view name, auto& value)
            {
                using FieldT = std::remove_reference_t<decltype(value)>;

                uint64_t nameHash = HashFieldName(name);
                auto it = records.find(nameHash);
                if (it == records.end())
                {
                    return; // field added in new version — keep default
                }

                const FieldRecord& rec = it->second;
                uint32_t copySize = std::min(rec.size, static_cast<uint32_t>(sizeof(FieldT)));
                std::memcpy(&value, buffer.data() + rec.offset, copySize);
            });

            return component;
        }

    private:
        template<typename T>
        IComponentStorage* EnsureStorage()
        {
            uint32_t id = m_TypeResolver.Resolve<T>();

            HBL2_CORE_ASSERT(id < m_MaxComponents, "Exceeded maximum number of components, consider increasing the max allowed!");

            if (!m_ComponentStorages[id])
            {
                // NOTE: If this allocation fails it means we ran out of memory due to many
                // component array clearings and re-allocations. (caused by script recompilations)
                auto* storage = m_Arena.AllocConstruct<StorageFor<T>>(m_MaxEntities, m_Reservation);

                m_ComponentStorages[id] = storage;
                m_ConcreteStorages[id] = storage;
            }

            return m_ComponentStorages[id];
        }

        // Simple FNV-1a hash for field name strings — constexpr, no allocations
        static constexpr uint64_t HashFieldName(std::string_view name) noexcept
        {
            uint64_t hash = 14695981039346656037ULL;
            for (char c : name)
            {
                hash ^= static_cast<uint64_t>(c);
                hash *= 1099511628211ULL;
            }
            return hash;
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

            m_FirstFree = 1;
            m_NextFree = nullptr;

            m_MaxEntities = 0;
        }

        int32_t FindEmptyEntity() const
        {
            return m_FirstFree;
        }

        int32_t DereferenceEntity(Entity ref) const
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