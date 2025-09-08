#pragma once
#include <memory>

namespace gbg {
// to avoid circular dep
class Scene;

class Element {
   public:
    int getId() const { return _id; };
    std::weak_ptr<Scene> getScene() { return _scene; };
    bool operator==(const Element& other) { return this->_id == other._id; }
    bool operator>(const Element& other) { return this->_id > other._id; }
    bool operator<(const Element& other) { return this->_id < other._id; }

    Element(std::weak_ptr<Scene> scene, int id) : _scene(scene), _id(id) {};

   private:
    std::weak_ptr<Scene> _scene;
    int _id;
};

}  // namespace gbg
