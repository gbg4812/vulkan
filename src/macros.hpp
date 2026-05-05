#pragma once

#define RESOURCE_CONSTR(x) \
    x() : Resource(){};    \
    x(std::string name, uint32_t rid) : Resource(name, rid){};

#define RESOURCE_HANDLE(x)                                            \
    struct x : public ResourceHandle {                                \
        public:                                                       \
        x() : ResourceHandle(){};                                     \
        x(uint32_t rid, size_t index) : ResourceHandle(rid, index){}; \
    }

#define CREATE_AND_GET(instance, handle, manager, name) \
    auto handle = manager.create(name);                 \
    auto& instance = manager.get(handle);
