
#include <memory>
template <typename T>
class SharedSet {
   private:
    class Node {
        std::shared_ptr<T> value;
        Node* right;
        Node* left;
    };

   private:
    Node* root;

   public:
    class iterator {
       private:
        Node* current;

       public:
        std::shared_ptr<T> operator*() { return current->value; }
        std::shared_ptr<T> operator->() { return current->value; }
        iterator& operator++() { current = current->right; };
    };
};
