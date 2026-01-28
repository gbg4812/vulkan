#pragma once

#include <stdexcept>

#include "SceneTree.hpp"

namespace gbg {
template <typename T, typename... Args>
class srPool {
   public:
    srPool(size_t capacity, uint8_t type) : _type(type) {
        _data.reserve(capacity);
    }

    DepDataHandle alloc(Args... args) {
        uint32_t index;
        if (_free.empty()) {
            _data.push_back(T(args...));
            _rdis.push_back(_nextid);
            index = _data.size() - 1;
        } else {
            index = _free.back();
            _free.pop_back();
            _data[index] = T(args...);
            _rdis[index] = _nextid;
        }
        return DepDataHandle{_data.size() - 1, _nextid++, _type, 0};
    }

    void free(DepDataHandle h) {
        _rdis[h.index] = 0;
        _free.push_back(h.index);
    }

    T& get(DepDataHandle h) {
        if (h.type != _type || _rdis[h.index] != h.rid) {
            throw std::runtime_error("invalid Pool access: wrong type or rid");
        }

        return _data[h.index];
    }

   private:
    std::list<size_t> _free;
    std::vector<T> _data;
    std::vector<uint16_t> _rdis;
    uint8_t _type;
    uint16_t _nextid = 1;

   public:
    class iterator {
       public:
        iterator(size_t index, std::vector<T>& data,
                 std::vector<uint16_t>& rids)
            : _index(index), _data(data), _rids(rids) {}

        iterator& operator++() {
            while (_rids[++_index] == 0) {
            }
            return *this;
        }

        iterator operator++(int) {
            iterator aux = *this;
            ++(*this);
            return aux;
        }

        T& operator*() { return _data[_index]; }

        bool operator==(iterator& other) {
            return other._index == this->_index;
        }

       private:
        size_t _index;
        std::vector<T>& _data;
        std::vector<uint16_t>& _rids;
    };

    iterator begin() { return iterator(0, _data, _rdis); }
    iterator end() { return iterator(_data.size(), _data, _rdis); }
};
}  // namespace gbg
