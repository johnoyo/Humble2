#pragma once

#include <iostream>
#include <string>
#include <utility>

namespace HBL2
{
    template <typename T>
    class Result
    {
        static_assert(std::is_default_constructible_v<T>);

    public:
        Result() = default;
        explicit Result(T value) : m_IsOk(true), m_Value(std::move(value)) {}
        explicit Result(T&& value) : m_IsOk(true), m_Value(std::forward<T>(value)) {}
        Result(std::string errorValue) : m_IsOk(false), m_ErrorValue(std::move(errorValue)) {}
        Result(Result&& other) : m_IsOk(other.m_IsOk)
        {
            if (m_IsOk)
            {
                m_Value = std::move(other.m_Value);
            }
            else
            {
                m_ErrorValue = std::move(other.m_ErrorValue);
            }

            other.m_IsOk = false;
        }

        Result(const Result<T>& other) = delete;

        ~Result()
        {
            if (m_IsOk)
            {
                m_Value.~T();
            }
            else
            {
                using namespace std;
                m_ErrorValue.~string();
            }
        }

        Result<T>& operator=(const Result<T>& other) = delete;

        Result& operator=(Result&& other)
        {
            if (this != &other)
            {
                m_IsOk = other.IsOk();

                if (m_IsOk)
                {
                    m_Value = std::move(other.m_Value);
                }
                else
                {
                    m_ErrorValue = std::move(other.m_ErrorValue);
                }

                other.m_IsOk = false;
            }

            return *this;
        }

        bool IsOk() const { return m_IsOk; }

        T& Unwrap()
        {
            HBL2_CORE_ASSERT(IsOk(), "Cannot unwrap a result that is an error!");
            return m_Value;
        }
        const T& Unwrap() const
        {
            HBL2_CORE_ASSERT(IsOk(), "Cannot unwrap a result that is an error!");
            return m_Value;
        }

        T* UnwrapIfOk()
        {
            if (!IsOk())
            {
                return nullptr;
            }

            return &m_Value;
        }
        const T* UnwrapIfOk() const
        {
            if (!IsOk())
            {
                return nullptr;
            }

            return &m_Value;
        }

        std::string& GetError()
        {
            HBL2_CORE_ASSERT(!IsOk(), "Cannot get the error from an ok result!");
            return m_ErrorValue;
        }
        const std::string& GetError() const
        {
            HBL2_CORE_ASSERT(!IsOk(), "Cannot get the error from an ok result!");
            return m_ErrorValue;
        }

    private:
        union
        {
            T m_Value;
            std::string m_ErrorValue;
        };

        bool m_IsOk = false;
    };

    template<typename T>
    static inline Result<T> Ok(const T& value) { return Result<T>(value); }

    static inline std::string Error(const std::string& why) { return why; }
}