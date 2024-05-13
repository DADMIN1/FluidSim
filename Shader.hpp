#ifndef FLUIDSYM_SHADER
#define FLUIDSYM_SHADER

#include <SFML/Graphics.hpp>
//https://www.sfml-dev.org/tutorials/2.6/graphics-shader.php


class Shader: sf::Shader, public sf::RenderStates {
    std::string name;
    bool isValid {false};
    
    public:
    // filename does not include extension
    // sf::Shader::Type::Fragment || Geometry || Vertex
    Shader(std::string filename, sf::Shader::Type);
    inline bool IsValid() const {return isValid;}
    bool InvokeSwitch() const;
};

// constructor aliases (requires variable name to match filename)
#define VERTSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Vertex  )
#define GEOMSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Geometry)
#define FRAGSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Fragment)


#endif
