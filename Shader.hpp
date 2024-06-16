#ifndef FLUIDSYM_SHADER_HPP_INCLUDED
#define FLUIDSYM_SHADER_HPP_INCLUDED

#include <map>
#include <SFML/Graphics.hpp>
//https://www.sfml-dev.org/tutorials/2.6/graphics-shader.php

//#include "Keybinds.hpp" //TODO: create this header
// TODO: each shader should store it's own source-code
// eventually, you should construct a system for combining/sequencing shaders

class Shader: public sf::Shader, public sf::RenderStates {
    bool isValid {false};
    static std::map<sf::Keyboard::Key, Shader*> keymap;
    static std::map<std::string, Shader*> NameLookup; // maps name->shader
    public: // this really shouldn't be public, but whatever
    std::map<std::string, float> uniform_vars;  // variables specific to the shader
    //Keybind keybind  //TODO: store keybind as a member variable
    
    //friend int main(int argc, char** argv);
    friend class MainGUI;
    
    public:
    const std::string name;
    static const Shader* current;
    // filename does not include extension
    // sf::Shader::Type::Fragment || Geometry || Vertex
    Shader(std::string filename, sf::Shader::Type);
    bool InvokeSwitch() const;  // sets as current shader
    static Shader* InvokeSwitch(std::string shadername); // switches to specified shader (and returns it)
    
    //bool InvokeSwitch(sf::Keyboard::Key) const;
    static const std::map<sf::Keyboard::Key, Shader*>& LoadAll();  // returns keymap ref
    void ApplyUniform(std::string, float);
    void ApplyUniforms(); //applies all stored in map
    
    Shader* GetWritePtr() const { return NameLookup[this->name]; } // it's really hard to setUniform() through a const pointer
};

// constructor aliases (requires variable name to match filename)
#define VERTSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Vertex  ); XMACRO(name)
#define GEOMSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Geometry); XMACRO(name)
#define FRAGSHADER(name) Shader name = Shader(std::string(#name), sf::Shader::Type::Fragment); XMACRO(name)
// The 'XMACRO' is a customization point; give it an empty definition if you don't use it
// ALWAYS undefine 'XMACRO' after using it: 
/*  "If the identifier is already defined as any type of macro, 
    the program is ill-formed unless the definitions are identical." */

#endif
