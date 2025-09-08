#pragma once
#include <set>

#include "Element.hpp"

namespace gbg {

int firstFreeIndex(const std::set<std::shared_ptr<Element>>& st) {
    auto it = st.begin();
    auto last = (*it++)->getId();
    while (it != st.end()) {
        if ((*it)->getId() - last > 1) {
            return last + 1;
        }
        last = (*it)->getId();
        it++;
    }
    return last + 1;
}
}  // namespace gbg
