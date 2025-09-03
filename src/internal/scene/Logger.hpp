#pragma once
#include <iostream>
#include <list>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"
#include "glm/glm.hpp"

#ifndef NDEBUG
#define LOG_VAR(x) std::cout << #x << " = " << x << std::endl;
#define LOG(x) std::cout << x << std::endl;
#else
#define LOG_VAR(x)
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

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& lst) {
    os << "(";
    for (const auto& el : lst) {
        os << el << ", ";
    }
    os << ")" << std::endl;

    return os;
}
