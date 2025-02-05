#include <iostream>
#include <vector>

#ifndef GBG_LOGGER
#define GBG_LOGGER

#ifndef NDEBUG
#define LOG(x) std::cout << #x << " = " << x << std::endl;
#else
#define LOG(x)
#endif

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "(";
    for (const auto& el : vec) {
        os << el << ", ";
    }
    os << ")" << std::endl;

    return os;
}

#endif
