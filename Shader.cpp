#include "Shader.hpp"

#include <iostream>


const Shader* Shader::current = nullptr;
std::map<sf::Keyboard::Key, Shader*> Shader::keymap{};

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
    if (!isValid) { std::cerr << "shader failed to load file: " << filepath; }
    
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
    current = this;
    return isValid;
}

const std::map<sf::Keyboard::Key, Shader*>& Shader::LoadAll()
{
    static std::size_t index{0};
    
    // TODO: do something better than making these 'static'
    std::cout << "loading shaders\n";
    static FRAGSHADER(empty);
    static FRAGSHADER(brighter);
    static FRAGSHADER(darker);
    static FRAGSHADER(red);
    static FRAGSHADER(turbulence);
    static FRAGSHADER(cherry_blossoms);
    
    keymap = {
        { sf::Keyboard::Num0, &empty },
        { sf::Keyboard::Num1, &brighter },
        { sf::Keyboard::Num2, &darker },
        { sf::Keyboard::Num3, &red },
        { sf::Keyboard::Num4, &cherry_blossoms },
        { sf::Keyboard::Num5, &turbulence },
    };
    
    //empty.InvokeSwitch();
    /* if (!empty.InvokeSwitch()) {
        std::cerr << "ragequitting because empty shader didn't load.\n";
        //return 3;
    } */
    
    return keymap;
}

