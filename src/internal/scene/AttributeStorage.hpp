#pragma once
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "gbg_traits.hpp"
namespace gbg {

template <typename T>
class attrib_reference {
   public:
    attrib_reference(std::vector<T>& vec) : _vec(&vec) {};
    attrib_reference() : _vec(nullptr) {};

   private:
    std::vector<T>* _vec;

   public:
    T& operator[](size_t index) { return _vec->at(index); }

    size_t size() const { return _vec->size(); }

    typename std::vector<T>::iterator begin() { return _vec->begin(); }

    typename std::vector<T>::iterator end() { return _vec->end(); }

    bool is_valid() { return _vec == nullptr; }

    void set_reference(std::vector<T>& vec) { _vec = &vec; }
};

template <typename VT>
    requires(is_variant_v<VT>)
class AttributeStorage {
   public:
    AttributeStorage() : _element_cnt(0) {}

   private:
    std::map<std::string, VT> _attribs;
    int _element_cnt;

    struct _Vis_AddElement {
        int new_size;
        template <typename T>
        void operator()(std::vector<T>& vec) {
            vec.resize(new_size);
        }
    };

   public:
    template <typename T>
        requires(variant_holds_type_v<std::vector<T>, VT>)
    attrib_reference<T> getAttribute(std::string name) {
        auto it = _attribs.find(name);
        if (it == _attribs.end()) {
            auto iit = _attribs.insert({name, std::vector<T>(_element_cnt)});
            return *std::get_if<std::vector<T>>(&(iit.first->second));
        }
        return *std::get_if<std::vector<T>>(&(it->second));
    }

    void resize(size_t new_size) {
        for (auto& el : _attribs) {
            std::visit(_Vis_AddElement{new_size}, el.second);
        }
    }
};

}  // namespace gbg
