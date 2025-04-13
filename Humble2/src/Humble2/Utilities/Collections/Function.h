#pragma once

#include "Utilities/Allocators/BaseAllocator.h"
#include "Utilities/Allocators/StandardAllocator.h"

#include <type_traits>
#include <utility>
#include <cstring>
#include <new>

namespace HBL2
{
	/**
	 * @brief A small-buffer optimized function wrapper, similar to std::function.
	 * 
	 * Stores callables (lambdas, function pointers, etc.) and supports copy/move, calling,
	 * and optional allocator-based heap fallback for larger callables.
	 * 
	 * @tparam Signature The function signature, e.g. void(int)
	 * @tparam TAllocator The allocator type to use for heap allocations.
	 */
    template<typename Signature, typename TAllocator = StandardAllocator>
    class Function;

	/**
	* @brief A small-buffer optimized function wrapper, similar to std::function.
	* 
	* Stores callables (lambdas, function pointers, etc.) and supports copy/move, calling,
	* and optional allocator-based heap fallback for larger callables.
	* 
	* @tparam R The function return type.
	* @tparam Args The function arguments.
	* @tparam TAllocator The allocator type to use for heap allocations.
	*/
    template<typename R, typename... Args, typename TAllocator>
    class Function<R(Args...), TAllocator>
    {
        static constexpr size_t BufferSize = 64;
        static constexpr size_t BufferAlign = alignof(std::max_align_t);

        using CallFn = R(*)(void*, Args&&...);
        using CopyFn = void(*)(void*, void*, TAllocator*);
        using DestroyFn = void(*)(void*, TAllocator*);

    public:

        /**
         * @brief Default constructor.
         *
         * @param allocator Optional allocator for heap-based callables.
         */
        Function(TAllocator* allocator = nullptr)
            : m_Allocator(allocator)
        {
        }

        /**
		 * @brief Constructs a Function from any callable (lambda, functor, etc.)
		 * 
		 * If the callable fits in the internal buffer, it's stored in-place. Otherwise, it's heap-allocated.
		 *
		 * @tparam F Callable type
		 * @param func The callable to store
		 * @param allocator Optional allocator
		 */
        template<typename F>
        Function(F&& func, TAllocator* allocator = nullptr)
            : m_Allocator(allocator)
        {
            using Functor = std::decay_t<F>;
            const bool fitsInBuffer = sizeof(Functor) <= BufferSize && alignof(Functor) <= BufferAlign;

            if (fitsInBuffer)
            {
                new (&m_Buffer) Functor(std::forward<F>(func));
                m_Storage = &m_Buffer;
                m_UsingHeap = false;
            }
            else
            {
                m_Storage = Allocate<Functor>(m_Allocator, sizeof(Functor));
                new (m_Storage) Functor(std::forward<F>(func));
                m_UsingHeap = true;
            }

            m_Call = [](void* storage, Args&&... args) -> R
			{
                return (*reinterpret_cast<Functor*>(storage))(std::forward<Args>(args)...);
            };

            m_Copy = [](void* dst, void* src, TAllocator* allocator)
			{
                Functor* source = reinterpret_cast<Functor*>(src);
                const bool fits = sizeof(Functor) <= BufferSize && alignof(Functor) <= BufferAlign;

                if (fits)
                {
                    new (dst) Functor(*source);
                }
                else
                {
                    void* mem = Allocate<Functor>(allocator, sizeof(Functor));
                    new (mem) Functor(*source);
                    *reinterpret_cast<void**>(dst) = mem;
                }
            };

            m_Destroy = [](void* storage, TAllocator* allocator)
			{
                Functor* f = reinterpret_cast<Functor*>(storage);
                f->~Functor();

                const bool fits = sizeof(Functor) <= BufferSize && alignof(Functor) <= BufferAlign;
                if (!fits && allocator)
                {
					Deallocate<Functor>(allocator, f);
                }
            };
        }

        /**
         * @brief Copy constructor.
         *
         * Performs a deep copy of the stored callable.
         *
         * @param other Function to copy from.
         */
        Function(const Function& other)
            : m_Allocator(other.m_Allocator),
            m_Call(other.m_Call),
            m_Copy(other.m_Copy),
            m_Destroy(other.m_Destroy),
            m_UsingHeap(other.m_UsingHeap)
        {
            if (!m_Call)
                return;

            if (m_UsingHeap)
            {
                m_Storage = nullptr; // will be assigned by m_Copy
                m_Copy(&m_Storage, other.m_Storage, m_Allocator);
            }
            else
            {
                m_Copy(&m_Buffer, other.m_Storage, m_Allocator);
                m_Storage = &m_Buffer;
            }
        }

        /**
         * @brief Move constructor.
         *
         * Transfers ownership of the callable from another function.
         * Leaves the source function in an empty state.
         *
         * @param other Function to move from.
         */
        Function(Function&& other) noexcept
            : m_Call(other.m_Call),
            m_Copy(other.m_Copy),
            m_Destroy(other.m_Destroy),
            m_Storage(other.m_Storage),
            m_Allocator(other.m_Allocator),
            m_UsingHeap(other.m_UsingHeap)
        {
            other.Clear();
        }

        /**
         * @brief Copy assignment operator.
         *
         * Destroys the existing callable and copies from another.
         *
         * @param other Function to copy from.
         * @return Reference to this function.
         */
        Function& operator=(const Function& other)
        {
            if (this == &other) return *this;

            Reset();

            m_Call = other.m_Call;
            m_Copy = other.m_Copy;
            m_Destroy = other.m_Destroy;
            m_Allocator = other.m_Allocator;
            m_UsingHeap = other.m_UsingHeap;

            if (!m_Call) return *this;

            if (m_UsingHeap)
            {
                m_Storage = nullptr;
                m_Copy(&m_Storage, other.m_Storage, m_Allocator);
            }
            else
            {
                m_Copy(&m_Buffer, other.m_Storage, m_Allocator);
                m_Storage = &m_Buffer;
            }

            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * Destroys the existing callable and takes ownership from another.
         *
         * @param other Function to move from.
         * @return Reference to this function.
         */
        Function& operator=(Function&& other) noexcept
        {
            if (this == &other) return *this;

            Reset();

            m_Call = other.m_Call;
            m_Copy = other.m_Copy;
            m_Destroy = other.m_Destroy;
            m_Storage = other.m_Storage;
            m_Allocator = other.m_Allocator;
            m_UsingHeap = other.m_UsingHeap;

            other.Clear();
            return *this;
        }

        /**
         * @brief Destructor.
         *
         * Destroys the stored callable and deallocates memory if needed.
         */
        ~Function()
        {
            Reset();
        }

        /**
         * @brief Invokes the stored callable.
         *
         * Calls the stored function with provided arguments.
         *
         * @param args Arguments to pass to the callable.
         * @return Result of invoking the callable.
         * @throws std::bad_function_call If the function is empty.
         */
        R operator()(Args... args) const
        {
            HBL2_CORE_ASSERT(m_Call && "Attempted to call an empty HBL2::Function");
            return m_Call(m_Storage, std::forward<Args>(args)...);
        }

        /**
         * @brief Checks whether the function contains a callable.
         *
         * @return True if a callable is stored, false otherwise.
         */
        operator bool() const { return m_Call != nullptr; }

    private:

        /**
         * @brief Destroys the stored callable.
         *
         * Calls the destructor and deallocates memory if heap-allocated.
         */
        void Reset()
        {
            if (m_Destroy && m_Storage)
            {
                m_Destroy(m_Storage, m_Allocator);
            }

            Clear();
        }

        /**
         * @brief Clears internal state.
         *
         * Resets function pointers and storage pointer without destroying memory.
         */
        void Clear()
        {
            m_Call = nullptr;
            m_Copy = nullptr;
            m_Destroy = nullptr;
            m_Storage = nullptr;
            m_UsingHeap = false;
        }

        /**
         * @brief Allocates memory for a callable.
         *
         * Uses the provided allocator or default new operator.
         *
         * @tparam U Type to allocate for.
         * @param size Size of the allocation in bytes.
         * @return Pointer to allocated memory.
         */
		template<typename U>
		static void* Allocate(TAllocator* allocator, uint64_t size)
		{
			if (allocator == nullptr)
			{
				void* data = (U*)operator new(size);
				memset(data, 0, size);				
				return data;
			}

			return allocator->Allocate<U>(size);
		}

        /**
         * @brief Deallocates memory for a callable.
         *
         * Uses the provided allocator or default delete operator.
         *
         * @tparam U Type to deallocate.
         * @param ptr Pointer to the memory to deallocate.
         */
		template<typename U>
		static void Deallocate(TAllocator* allocator, U* ptr)
		{
			if (allocator == nullptr)
			{
				if constexpr (std::is_array_v<U>)
				{
					delete[] ptr;
				}
				else
				{
					delete ptr;
				}

				return;
			}

			allocator->Deallocate<U>(ptr);
		}

    private:
        alignas(BufferAlign) char m_Buffer[BufferSize];

        void* m_Storage = nullptr;
        bool m_UsingHeap = false;

        CallFn m_Call = nullptr;
        CopyFn m_Copy = nullptr;
        DestroyFn m_Destroy = nullptr;

        TAllocator* m_Allocator = nullptr;
    };

    /**
     * @brief Helper to construct a Function from a callable.
     *
     * Deduces types and forwards the callable and allocator.
     *
     * @tparam R Return type.
     * @tparam Args Argument types.
     * @tparam TAllocator Allocator type.
     * @tparam F Callable type.
     * @param fn Callable object.
     * @param allocator Optional allocator.
     * @return A constructed Function.
     */
    template<typename R, typename... Args, typename TAllocator, typename F>
    auto MakeFunction(F&& fn, TAllocator* allocator = nullptr)
    {
        return Function<R(Args...), TAllocator>(std::forward<F>(fn), allocator);
    }
}
