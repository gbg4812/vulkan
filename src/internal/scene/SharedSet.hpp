
#include <memory>
template <typename T>
class SharedSet {
   private:
    class Node {
        std::shared_ptr<T> value;
        Node* right = nullptr;
        Node* left = nullptr;
        Node* parent = nullptr;
    };

   private:
    Node* root;

   public:
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
            if (last == current->parent) {
                if (current->left != nullptr) {
                    last = current;
                    current = current->left;
                    return ++(*this);
                }
                last = current;
                return *this;

            } else if (last == current) {
                if (current->right != nullptr) {
                    current = current->left;
                }
            } else if (last == current->left) {
            } else if (last == current->right) {
            }
        }
        iterator operator++(int) {
            iterator aux = {current, last};
            ++(*this);
            return aux;
        }
    };

    iterator begin() {
        Node* current = root;
        Node* last = nullptr;
        while (current->left != nullptr) {
            last = current;
            current = current->left;
        }

        return iterator(current, last);
    }
};
