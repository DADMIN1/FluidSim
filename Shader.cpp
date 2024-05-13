#include "Shader.hpp"

#include <iostream>


Shader::Shader(std::string name, sf::Shader::Type shadertype)
: sf::Shader(), sf::RenderStates(), name{name}
{
    if (!sf::Shader::isAvailable()) { 
        std::cerr << "shaders unsupported!\n";
        return;
    }
    
    std::string file_extension;
    switch(shadertype)
    {
        case sf::Shader::Type::Vertex  : file_extension = ".vert"; break;
        case sf::Shader::Type::Geometry: file_extension = ".geom"; break;
        case sf::Shader::Type::Fragment: file_extension = ".frag"; break;
    }
    
    std::string filepath = "Shaders/" + name + file_extension;
    isValid = this->loadFromFile(filepath, shadertype);
    if (!isValid) { std::cerr << "shader failed to load file: " << file_extension; }
    
    this->setUniform("texture", sf::Shader::CurrentTexture); // magic
    this->shader = this;
    
    return;
}

bool Shader::InvokeSwitch() const 
{
    std::cout << "setting active shader: " << name << '\n';
    if (!isValid) {
        std::cerr << "could not load shader: " << name << '\n';
    }
    return isValid;
}
