#pragma once

#include "SparseComponentStorage.h"

namespace HBL2
{
    template<typename T, typename = void>
    struct StorageForImpl { using type = SparseComponentStorage<T>; };

    template<typename T>
    struct StorageForImpl<T, std::void_t<typename T::storage_type>>
    {
        using type = typename T::storage_type;
    };

    template<typename T>
    using StorageFor = typename StorageForImpl<T>::type;
}