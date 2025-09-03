#include <iostream>
#include <memory>

#include "Logger.hpp"
#include "Model.hpp"
#include "Shader.hpp"
#include "resourceLoader.hpp"

int main(int argc, char* argv[]) {
    LOG("Creating Mesh: ")
    auto mesh1 = std::make_shared<gbg::Mesh>();
    LOG("Adding vertex {0,1,0} and {0,2,0}")
    mesh1->addVertex({0, 1, 0});
    mesh1->addVertex({0, 2, 0});
    LOG("Creating attribute 'pscale' of type Float")
    auto pscale = mesh1->createAttribute<gbg::AttributeType::Float>("pscale");
    LOG("Geting vertex position attribute")
    auto pos = mesh1->getPositions();

    LOG_VAR(*pscale)
    LOG_VAR(*pos)

    LOG("Adding vertex {1,1,1}")
    mesh1->addVertex({1, 1, 1});
    mesh1->addVertex({2, 2, 2});
    mesh1->addVertex({3, 3, 3});
    mesh1->addVertex({4, 4, 4});

    auto attr = mesh1->getAttribute<gbg::AttributeType::Float>("pscale");
    LOG(attr->at(3));

    LOG_VAR(*pscale)
    LOG_VAR(*pos)

    LOG("Creating Shader: ")
    auto shader1 =
        std::make_shared<gbg::Shader>("./shader.frag", "./shader.vert");

    LOG("Adding inputs to the shader")
    shader1->addInput("mvp", gbg::InputType::Mat4x4);
    shader1->addInput("roughness", gbg::InputType::Float);

    LOG("Adding attributes to the shader")
    shader1->addInputAttribute("position", gbg::AttributeType::Vector3);
    shader1->addInputAttribute("uv", gbg::AttributeType::Vector2);

    LOG("Creating Material")
    auto mat1 = std::make_shared<gbg::Material>(shader1);
    mat1->setResource<gbg::InputType::Mat4x4>("mvp", glm::mat4(1.0f));

    LOG("Creating Object")
    auto obj1 = std::make_shared<gbg::Model>(mat1, mesh1);

    LOG("Loading Mesh: ")
    auto lmesh = gbg::loadMesh("../../models/EasyModels/square.obj");

    LOG_VAR(*lmesh->getPositions())
    LOG_VAR(*lmesh->getFaces())

    return 0;
}
