#include <iostream>
#include <string>
#include <vector>
#include <map>

int main() {
    const char** names = new const char*[3] {"guillem", "isaac", "toni"};

    std::multimap<int, std::string> m;
    m.emplace(10, "hello");
    m.emplace(3, "bye");
    m.emplace(5, "isaac");
    m.emplace(20, "guillem");

    for (auto el : m){
        std::cout << el.first << ", " << el.second << std::endl;
    }

    std::vector<const char*> names_v(names, names+3);


    for (const char* name : names_v) {
        std::cout << name << std::endl;
    }

    return 0;
}
