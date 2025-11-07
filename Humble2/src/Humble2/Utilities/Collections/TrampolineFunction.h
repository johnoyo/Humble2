#pragma once

#include <cstdint>
#include <utility>

namespace HBL2
{
    /// <summary>
    /// A minimal trampoline-style function wrapper.
    /// Stores a void* context and a raw function pointer that knows how to call it.
    /// This is ideal for coroutines, game engines, or system-level callbacks.
    /// </summary>
    template<typename Ret, typename... Args>
    class TrampolineFunction
    {
    public:
        using FnPtr = Ret(*)(void*, Args...);

        TrampolineFunction() = default;

        TrampolineFunction(void* ctx, FnPtr fn)
            : m_Context(ctx), m_Function(fn)
        {

        }

        template<typename Callable>
        static TrampolineFunction FromCallable(Callable* callable)
        {
            return TrampolineFunction(
                static_cast<void*>(callable),
                [](void* ctx, Args... args) -> Ret
                {
                    return (*static_cast<Callable*>(ctx))(std::forward<Args>(args)...);
                }
            );
        }

        Ret operator()(Args... args) const
        {
            return m_Function(m_Context, std::forward<Args>(args)...);
        }

        explicit operator bool() const { return m_Function != nullptr; }

    private:
        void* m_Context = nullptr;
        FnPtr m_Function = nullptr;
    };

    template<typename Component, typename Fn>
    static TrampolineFunction<void, void*> MakeRawCallback(Fn* fnPtr)
    {
        return TrampolineFunction<void, void*>(
            fnPtr, 
            +[](void* ctx, void* element) {
                auto& f = *static_cast<Fn*>(ctx);
                f(*static_cast<Component*>(element));
            });
    }

}