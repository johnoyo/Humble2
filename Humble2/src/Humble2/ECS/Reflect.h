#pragma once

#include "Registry.h"
#include "Humble2API.h"

#include <cstddef>
#include <cassert>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>
#include <functional>

namespace HBL2::Reflect
{
    namespace Detail
    {
        template<typename T>
        constexpr std::string_view TypeName() noexcept
        {
#if defined(_MSC_VER)
            std::string_view sig = __FUNCSIG__;
            auto start = sig.find("TypeName<") + 9;
            auto end = sig.rfind(">(");
#elif defined(__clang__)
            std::string_view sig = __PRETTY_FUNCTION__;
            auto start = sig.find("T = ") + 4;
            auto end = sig.rfind("]");
#elif defined(__GNUC__)
            std::string_view sig = __PRETTY_FUNCTION__;
            auto start = sig.find("T = ") + 4;
            auto end = sig.rfind("]");
#else
            #error "Unsupported compiler"
#endif
            return sig.substr(start, end - start);
        }

        constexpr std::size_t FNV1a(std::string_view str) noexcept
        {
            std::size_t hash = 14695981039346656037ULL;
            for (char c : str)
            {
                hash ^= static_cast<std::size_t>(c);
                hash *= 1099511628211ULL;
            }
            return hash;
        }
    }

    template<typename T>
    struct TypeIndex
    {
        static constexpr std::size_t value = Detail::FNV1a(Detail::TypeName<T>());
    };

    struct Any
    {
        void* Ptr = nullptr;
        std::size_t TypeId = 0;

        Any() = default;

        template<typename T>
        static Any From(T& obj) noexcept
        {
            Any a;
            a.Ptr = &obj;
            a.TypeId = TypeIndex<std::remove_const_t<T>>::value;
            return a;
        }

        template<typename T>
        static Any FromConst(const T& obj) noexcept
        {
            Any a;
            a.Ptr = const_cast<T*>(&obj);
            a.TypeId = TypeIndex<const T>::value;
            return a;
        }

        template<typename T>
        T* TryGetAs() noexcept
        {
            size_t typeIndex = TypeIndex<std::remove_const_t<T>>::value;
            if (TypeId == typeIndex)
            {
                return static_cast<T*>(Ptr);
            }

            return nullptr;
        }

        template<typename T>
        const T* TryGetAs() const noexcept
        {
            size_t typeIndex = TypeIndex<std::remove_const_t<T>>::value;
            if (TypeId == typeIndex)
            {
                return static_cast<const T*>(Ptr);
            }

            return nullptr;
        }

        template<typename T>
        bool Is() const noexcept
        {
            size_t typeIndex = TypeIndex<std::remove_const_t<T>>::value;
            return TypeId == typeIndex;
        }

        template<typename T>
        bool Set(const T& newValue)
        {
            if (!Is<T>())
            {
                return false;
            }

            *static_cast<T*>(Ptr) = newValue;
            return true;
        }

        bool IsValid() const noexcept { return Ptr != nullptr; }
    };

    // Field — compile-time descriptor for one member
    // Supports chained accessors for nested access:
    //   Field{"vel.x", &Physics::velocity, &Vec3::x}
    template<typename... Accessors>
    class Field
    {
    public:
        constexpr Field(std::string_view name, Accessors... accessors) noexcept
            : m_Name(name), m_Accessors(accessors...)
        {}

        constexpr std::string_view Name() const noexcept { return m_Name; }

        template<typename U>
        constexpr decltype(auto) ExtractFrom(U&& obj) const noexcept
        {
            return std::apply(
                [&](auto... accs) -> decltype(auto)
                {
                    return Chain(std::forward<U>(obj), accs...);
                },
                m_Accessors
            );
        }

    private:
        // Single accessor — base case
        template<typename U, typename A>
        static constexpr decltype(auto) Chain(U&& obj, A acc) noexcept
        {
            return std::forward<U>(obj).*acc;
        }

        // Multiple accessors — recurse, forwarding the intermediate reference
        template<typename U, typename A, typename... As>
        static constexpr decltype(auto) Chain(U&& obj, A acc, As... rest) noexcept
        {
            return Chain(std::forward<U>(obj).*acc, rest...);
        }

    private:
        std::string_view         m_Name;
        std::tuple<Accessors...> m_Accessors;
    };

    // Schema — compile-time ordered list of Fields
    // ForEach is fully unrolled at compile time.
    // Get/Set/Has use short-circuit fold — N inline string comparisons, no heap.
    template<typename... Fields>
    class Schema
    {
    public:
        constexpr Schema(Fields... fields) noexcept
            : m_Fields(std::move(fields)...)
        {}

        // Compile-time typed iteration — callable: void(string_view name, auto& value)
        template<typename U, typename Callable>
        constexpr void ForEach(U&& obj, Callable&& callable) const
        {
            std::apply(
                [&](const auto&... fs)
                {
                    (std::invoke(callable, fs.Name(), fs.ExtractFrom(std::forward<U>(obj))), ...);
                },
                m_Fields
            );
        }

        // Runtime get by name — returns non-owning Any, invalid if not found
        template<typename U>
        Any Get(U& obj, std::string_view name) const noexcept
        {
            Any result;
            std::apply(
                [&](const auto&... fs)
                {
                    ([&]() -> bool
                    {
                        if (fs.Name() != name)
                        {
                            return false;
                        }

                        result = Any::From(fs.ExtractFrom(obj));
                        return true;
                    }() || ...);
                },
                m_Fields
            );
            return result;
        }

        // Runtime set by name from Any — false on name/type mismatch
        template<typename U>
        bool Set(U& obj, std::string_view name, const Any& value) const noexcept
        {
            bool found = false;
            std::apply(
                [&](const auto&... fs)
                {
                    ([&]() -> bool
                    {
                        if (found || fs.Name() != name)
                        {
                            return false;
                        }

                        using FieldT = std::remove_reference_t<decltype(fs.ExtractFrom(obj))>;
                        if (const auto* typed = value.TryGetAs<FieldT>())
                        {
                            fs.ExtractFrom(obj) = *typed;
                            found = true;
                        }
                        return found;
                    }() || ...);
                },
                m_Fields
            );
            return found;
        }

        // Runtime set by name from typed value — false if name not found
        template<typename U, typename T>
        bool Set(U& obj, std::string_view name, const T& value) const noexcept
        {
            bool found = false;
            std::apply(
                [&](const auto&... fs)
                {
                    ([&]() -> bool
                    {
                        if (found || fs.Name() != name)
                        {
                            return false;
                        }

                        using FieldT = std::remove_reference_t<decltype(fs.ExtractFrom(obj))>;
                        if constexpr (std::is_assignable_v<FieldT&, const T&>)
                        {
                            fs.ExtractFrom(obj) = value;
                            found = true;
                        }
                        return found;
                    }() || ...);
                },
                m_Fields
            );
            return found;
        }

        // Runtime field existence check
        bool Has(std::string_view name) const noexcept
        {
            bool found = false;
            std::apply(
                [&](const auto&... fs)
                {
                    ((found = found || (fs.Name() == name)), ...);
                },
                m_Fields
            );
            return found;
        }

        static constexpr std::size_t FieldCount() noexcept { return sizeof...(Fields); }

    private:
        std::tuple<Fields...> m_Fields;
    };

    // Deduction guides
    template<typename... Accessors>
    Field(std::string_view, Accessors...) -> Field<Accessors...>;
    template<typename... Fields>
    Schema(Fields...) -> Schema<Fields...>;

    template<typename T, typename Callable>
    void ForEach(T& obj, Callable&& callable)
    {
        T::schema.ForEach(obj, std::forward<Callable>(callable));
    }

    template<typename T, typename Callable>
    void ForEach(const T& obj, Callable&& callable)
    {
        T::schema.ForEach(obj, std::forward<Callable>(callable));
    }

    template<typename T>
    Any Get(T& obj, std::string_view name) noexcept
    {
        return T::schema.Get(obj, name);
    }

    template<typename T>
    bool Set(T& obj, std::string_view name, const Any& value) noexcept
    {
        return T::schema.Set(obj, name, value);
    }

    template<typename T, typename V>
    bool Set(T& obj, std::string_view name, const V& value) noexcept
    {
        return T::schema.Set(obj, name, value);
    }

    template<typename T>
    Any ForwardAsMeta(T& obj) noexcept { return Any::From(obj); }

    template<typename T>
    Any ForwardAsMeta(const T& obj) noexcept { return Any::FromConst(obj); }

    struct FieldEntry
    {
        std::string_view name;
        std::size_t typeId;
    };

    using FieldCallback = void(*)(void* userdata, std::string_view name, Any value);

    struct TypeEntry
    {
        std::size_t typeId = 0;
        std::string_view typeName;
        std::size_t size = 0;
        std::size_t alignment = 0;
        std::vector<FieldEntry> fields;

        using ByteStorage = std::unordered_map<std::string, std::unordered_map<HBL2::Entity, std::vector<std::byte>>>;

        Any  (*get)                (void* obj, std::string_view name)                                               = nullptr;
        bool (*setFromAny)         (void* obj, std::string_view name, const Any&)                                   = nullptr;
        bool (*setFromPtr)         (void* obj, std::string_view name, const void* value, std::size_t valueTypeId)   = nullptr;
        Any  (*addToRegistry)      (Registry* r, Entity e)                                                          = nullptr;
        Any  (*getFromRegistry)    (Registry* r, Entity e)                                                          = nullptr;
        void (*removeFromRegistry) (Registry* r, Entity e)                                                          = nullptr;
        bool (*hasInRegistry)      (Registry* r, Entity e)                                                          = nullptr;
        void (*clearStorage)       (Registry* r)                                                                    = nullptr;
        void (*serialize)          (Registry* r, ByteStorage& data, bool clearAfter)                                = nullptr;
        void (*deserialize)        (Registry* r, ByteStorage& data)                                                 = nullptr;
        void (*cloneToRegistry)    (Registry* src, Registry* dst)                                                   = nullptr;
        void (*copy)               (void* dst, const void* src)                                                     = nullptr;

        // Iterates all fields — invoke(userdata, name, Any-wrapped-field) per field
        void (*forEach)(void* obj, void* userdata, FieldCallback invoke)                                            = nullptr;
    };

    HBL2_API std::vector<TypeEntry>& GetRegistry();

    inline const TypeEntry* FindType(std::size_t typeId)
    {
        for (const auto& e : GetRegistry())
        {
            if (e.typeId == typeId)
            {
                return &e;
            }
        }
        return nullptr;
    }

    inline const TypeEntry* FindType(std::string_view typeName)
    {
        for (const auto& e : GetRegistry())
        {
            if (e.typeName == typeName)
            {
                return &e;
            }
        }
        return nullptr;
    }

    template<typename Callable>
    void ForEachRegisteredType(Callable&& callable)
    {
        for (const auto& entry : GetRegistry())
        {
            std::invoke(callable, entry);
        }
    }

    template<typename T>
    bool Register()
    {
        const std::size_t id = TypeIndex<T>::value;
        if (FindType(id))
        {
            return true;
        }

        TypeEntry entry;
        entry.typeId    = id;
        entry.typeName  = typeid(T).name();
        entry.size      = sizeof(T);
        entry.alignment = alignof(T);

        // Populate field list from schema
        T dummy{};
        T::schema.ForEach(dummy,
            [&](std::string_view name, auto& value)
            {
                using F = std::remove_reference_t<decltype(value)>;
                entry.fields.push_back({ name, TypeIndex<std::remove_const_t<F>>::value });
            }
        );

        entry.get = [](void* obj, std::string_view name) -> Any
        {
            return T::schema.Get(*static_cast<T*>(obj), name);
        };

        entry.setFromAny = [](void* obj, std::string_view name, const Any& val) -> bool
        {
            return T::schema.Set(*static_cast<T*>(obj), name, val);
        };

        entry.setFromPtr = [](void* obj, std::string_view name, const void* value, std::size_t valueTypeId) -> bool
        {
            bool found = false;
            T::schema.ForEach(*static_cast<T*>(obj),
                [&](std::string_view fieldName, auto& fieldValue)
                {
                    if (found || fieldName != name)
                    {
                        return;
                    }

                    using FieldT = std::remove_reference_t<decltype(fieldValue)>;
                    if (TypeIndex<std::remove_const_t<FieldT>>::value == valueTypeId)
                    {
                        fieldValue = *static_cast<const FieldT*>(value);
                        found = true;
                    }
                }
            );
            return found;
        };

        entry.addToRegistry = [](Registry* r, Entity e) -> Any
        {
            T& c = r->AddComponent<T>(e);
            return ForwardAsMeta(c);
        };

        entry.getFromRegistry = [](Registry* r, Entity e) -> Any
        {
            T& c = r->GetComponent<T>(e);
            return ForwardAsMeta(c);
        };

        entry.removeFromRegistry = [](Registry* r, Entity e)
        {
            r->RemoveComponent<T>(e);
        };

        entry.hasInRegistry = [](Registry* r, Entity e) -> bool
        {
            return r->HasComponent<T>(e);
        };

        entry.clearStorage = [](Registry* r)
        {
            r->ClearStorage<T>();
        };

        using ByteStorage = std::unordered_map<std::string, std::unordered_map<HBL2::Entity, std::vector<std::byte>>>;

        entry.serialize = [](Registry* r, ByteStorage& data, bool clearAfter)
        {
            r->Filter<T>().ForEach([&](Entity e, T& component)
            {
                data[typeid(T).name()][e] = Registry::Serialize(component);
            });

            if (clearAfter)
            {
                r->ClearStorage<T>();
            }
        };

        entry.deserialize = [](Registry* r, ByteStorage& data)
        {
            auto& map = data[typeid(T).name()];
            for (auto& [entity, bytes] : map)
            {
                T& c = r->AddComponent<T>(entity);
                c = Registry::Deserialize<T>(bytes);
            }
        };

        entry.cloneToRegistry = [](Registry* src, Registry* dst)
        {
            src->Filter<T>().ForEach([&](Entity e, T& component)
            {
                dst->AddComponent<T>(e, T(component));
            });
        };

        entry.copy = [](void* dst, const void* src)
        {
            *static_cast<T*>(dst) = *static_cast<const T*>(src);
        };

        entry.forEach = [](void* obj, void* userdata, FieldCallback invoke)
        {
            T::schema.ForEach(*static_cast<T*>(obj),
                [&](std::string_view name, auto& value)
                {
                    invoke(userdata, name, Any::From(value));
                }
            );
        };

        GetRegistry().push_back(std::move(entry));
        return true;
    }

    template<typename T>
    void Unregister()
    {
        const std::size_t id = TypeIndex<T>::value;
        auto& reg = GetRegistry();
        reg.erase(std::remove_if(reg.begin(), reg.end(), [id](const TypeEntry& e) { return e.typeId == id; }), reg.end());
    }

    inline void Unregister(TypeEntry* entryToRemove)
    {
        auto& reg = GetRegistry();
        reg.erase(std::remove_if(reg.begin(), reg.end(), [entryToRemove](const TypeEntry& e) { return e.typeId == entryToRemove->typeId; }), reg.end());
    }

    inline void Unregister(std::string_view typeName)
    {
        auto& reg = GetRegistry();
        reg.erase(std::remove_if(reg.begin(), reg.end(), [typeName](const TypeEntry& e) { return e.typeName == typeName; }), reg.end());
    }
}

#define REGISTER_HBL2_COMPONENT(Type)       \
    extern "C" __declspec(dllexport)        \
    void RegisterComponent_##Type()         \
    {                                       \
        HBL2::Reflect::Register<Type>();    \
    }
