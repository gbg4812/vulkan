#include <iostream>
#include <memory>

#include "SharedSet.hpp"
int main(int argc, char* argv[]) {
    SharedSet<int> st;
    st.add(std::make_shared<int>(10));
    st.add(std::make_shared<int>(40));
    st.add(std::make_shared<int>(2));
    st.add(std::make_shared<int>(42));
    st.add(std::make_shared<int>(41));
    st.add(std::make_shared<int>(100));
    st.add(std::make_shared<int>(5));

    auto it = st.begin();
    while (it != st.end()) {
        std::cout << *(*it) << std::endl;
        it++;
    }
    return 0;
}
