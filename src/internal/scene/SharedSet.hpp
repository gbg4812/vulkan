// GOD TRY
#pragma once
#include <memory>
#include <stdexcept>

#include "Logger.hpp"
template <typename T>
class SharedSet {
   private:
    class Node {
       public:
        Node(std::shared_ptr<T> value) : value(value) {}
        std::shared_ptr<T> value;
        Node* right = nullptr;
        Node* left = nullptr;
        Node* parent = nullptr;
    };

   private:
    Node* root = nullptr;

    void add_rec(std::shared_ptr<T> value, Node* current) {
        if (*value < *(current->value)) {
            if (current->left == nullptr) {
                current->left = new Node(value);
                current->left->parent = current;
                return;
            }
            add_rec(value, current->left);
        } else if (*value > *(current->value)) {
            if (current->right == nullptr) {
                current->right = new Node(value);
                current->right->parent = current;
                return;
            }
            add_rec(value, current->right);
        } else {
            throw std::runtime_error("Element already exists in the set");
        }
    }

   public:
    void add(std::shared_ptr<T> value) {
        if (root == nullptr) {
            root = new Node(value);
            return;
        }

        add_rec(value, root);
    }
    class iterator {
       public:
        iterator(Node* current, Node* last) : current(current), last(last) {}

       private:
        Node* current;
        Node* last = nullptr;

       public:
        std::shared_ptr<T> operator*() { return current->value; }
        std::shared_ptr<T> operator->() { return current->value; }
        iterator& operator++() {
            if (current == nullptr) {
                // end iterator
                last = nullptr;
                return *this;

            } else if (last == current->parent) {
                // first visit of the node
                if (current->left != nullptr) {
                    last = current;
                    current = current->left;
                    return ++(*this);
                }
                last = current;
                return *this;

            } else if (last == current) {
                // after self visiting
                if (current->right != nullptr) {
                    current = current->right;
                    return ++(*this);
                }

                current = current->parent;
                return ++(*this);

            } else if (last == current->left) {
                last = current;
                return *this;

            } else if (last == current->right) {
                last = current;

                current = current->parent;
                return ++(*this);
            } else {
                throw std::runtime_error("Bad iterator");
            }
        }
        iterator operator++(int) {
            iterator aux = {current, last};
            ++(*this);
            return aux;
        }
        bool operator==(const iterator& other) {
            if (other.current == this->current and other.last == this->last) {
                return true;
            }
        }

        bool operator!=(const iterator& other) { return not(*this == other); }
    };

    iterator begin() {
        Node* current = root;
        while (current->left != nullptr) {
            current = current->left;
        }

        return iterator(current, current);
    }

    iterator end() { return iterator(nullptr, nullptr); }
};
