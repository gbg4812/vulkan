#include <iostream>
#include <type_traits>

template <typename C, typename T>
void callBack(int cause, C callback, T data) {
    static_assert(std::is_invocable_v<C, T>);
    callback();
}

void my_callback() { std::cout << "my callback" << std::endl; }

int main() {
    callBack(0, my_callback, 1.0f);
    return 0;
}
