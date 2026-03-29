#include "Reflect.h"

namespace HBL2::Reflect
{
    std::vector<TypeEntry>& GetRegistry()
    {
        static std::vector<TypeEntry> s_Instance;
        s_Instance.reserve(256);
        return s_Instance;
    }
}
