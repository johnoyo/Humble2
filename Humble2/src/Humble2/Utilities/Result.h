#pragma once
#include <iostream>
#include <string>
#include <utility>

namespace HBL2
{
    template <typename T>
    struct OkValue
    {
        explicit OkValue(T value) : Value(std::move(value)) {}
        T Value;
    };

    template <typename E>
    struct ErrorValue
    {
        explicit ErrorValue(E value) : Value(std::move(value)) {}
        E Value;
    };

    template <typename T, typename E = std::string>
    class Result
    {
    public:
        Result() = default;

        // Constructors for Ok and Err
        Result(OkValue<T> okay) : isGood(true) { new (&data.value) T(std::move(okay.Value)); }
        Result(ErrorValue<E> notOkay) : isGood(false) { new (&data.errorValue) E(std::move(notOkay.Value)); }

        // Move constructor
        Result(Result&& other) : isGood(other.isGood)
        {
            if (isGood)
            {
                new (&data.value) T(std::move(other.data.value));
            }
            else
            {
                new (&data.errorValue) E(std::move(other.data.errorValue));
            }
        }

        // Move assignment
        Result& operator=(Result&& other)
        {
            if (this != &other)
            {
                this->~Result();
                isGood = other.isGood;
                if (isGood)
                {
                    new (&data.value) T(std::move(other.data.value));
                }
                else
                {
                    new (&data.errorValue) E(std::move(other.data.errorValue));
                }
            }
            return *this;
        }

        // Check state
        bool IsOk() const { return isGood; }

        // Get value (only if `good() == true`)
        T& Unwrap() { return data.value; }
        const T& Unwrap() const { return data.value; }

        // Get error (only if `good() == false`)
        E& Error() { return data.errorValue; }
        const E& Error() const { return data.errorValue; }

    private:
        struct
        {
            T value;
            E errorValue;
        } data;
        bool isGood;
    };

    // Helper functions
    template <typename T>
    static inline Result<T> Ok(T value) { return OkValue<T>(value); }
    static inline ErrorValue<std::string> Error(const std::string& why) { return ErrorValue<std::string>(why); }
}