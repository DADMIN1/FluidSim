#ifndef FLUIDSYM_SHADER_HPP_INCLUDED
#define FLUIDSYM_SHADER_HPP_INCLUDED

#include <map>
#include <SFML/Graphics.hpp>
//https://www.sfml-dev.org/tutorials/2.6/graphics-shader.php


class Shader: public sf::Shader, public sf::RenderStates {
    std::string name;
    bool isValid {false};
    static std::map<sf::Keyboard::Key, Shader*> keymap;
    
    public:
    static const Shader* current;
    // filename does not include extension
    // sf::Shader::Type::Fragment || Geometry || Vertex
    Shader(std::string filename, sf::Shader::Type);
    bool InvokeSwitch() const;  // sets as current shader
    //bool InvokeSwitch(sf::Keyboard::Key) const;
    static const std::map<sf::Keyboard::Key, Shader*>& LoadAll();  // returns keymap ref
};

// constructor aliases (requires variable name to match filename)
#define VERTSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Vertex  )
#define GEOMSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Geometry)
#define FRAGSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Fragment)


#endif
