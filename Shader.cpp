#include "Shader.hpp"

#include <iostream>


const Shader* Shader::current = nullptr;
std::map<sf::Keyboard::Key, Shader*> Shader::keymap{};
std::map<std::string, Shader*> Shader::NameLookup{};


Shader::Shader(std::string name, sf::Shader::Type shadertype)
: sf::Shader(), sf::RenderStates(), name{name}
{
    if (!sf::Shader::isAvailable()) { 
        std::cerr << "\nshaders unsupported!\n";
        return;
    }
    
    std::string file_extension;
    switch(shadertype)
    {
        case sf::Shader::Type::Vertex  : file_extension = ".vert"; break;
        case sf::Shader::Type::Geometry: file_extension = ".geom"; break;
        case sf::Shader::Type::Fragment: file_extension = ".frag"; break;
    }
    
    std::string filepath = "./Shaders/" + name + file_extension;
    isValid = this->loadFromFile(filepath, shadertype);
    if (!isValid) { std::cerr << "shader failed to load file: \'" << filepath << "\' \n"; }
    
    this->setUniform("texture", sf::Shader::CurrentTexture); // magic
    this->shader = this;
    
    return;
}

// TODO: needs customization point for shaders to define custom callbacks on switch
bool Shader::InvokeSwitch() const
{
    if (!isValid) { std::cerr << "could not switch to invalid shader: " << name << '\n'; }
    else if (current != this) { 
        std::cout << "\nsetting active shader: " << name << '\n'; 
        current = this;
    }
    return isValid;
}

Shader* Shader::InvokeSwitch(std::string shadername)
{
    if (!NameLookup.contains(shadername)) {
        std::cerr << "no such shader: " << shadername << '\n';
        return nullptr;
    }
    Shader* nextshader = NameLookup.at(shadername);
    if (!nextshader->InvokeSwitch()) {
        return Shader::current->GetWritePtr(); // currentptr did not change if shader was invalid
    }
    return nextshader;
}

const std::map<sf::Keyboard::Key, Shader*>& Shader::LoadAll()
{
    // TODO: write macro that sequentially assigns keybinds to shaders
    // and extend FRAGSHADER macro to be variadic
    
    // TODO: do something better than making these 'static'
    std::cout << "\nLoading Shaders...\n";
    #define XMACRO(name) Shader::NameLookup[#name] = &name;
    static FRAGSHADER(empty);
    static FRAGSHADER(brighter);
    static FRAGSHADER(darker);
    static FRAGSHADER(red);
    static FRAGSHADER(cherry_blossoms);
    static FRAGSHADER(turbulence);
    //static FRAGSHADER(SO_example);
    #undef XMACRO
    
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
    
    turbulence.uniform_vars["threshold"] = 0.5f;
    turbulence.setUniform("threshold", 0.5f);
    
    return keymap;
}

void Shader::ApplyUniform(std::string varName, float val) {
    if (!uniform_vars.contains(varName)) {
        std::cerr << "Shader: " << this->name << " does not have a variable: " << varName << '\n';
        return;
    }
    std::cout << "setting uniform: '" << varName << "' = " << val << '\n';
    uniform_vars[varName] = val;
    this->setUniform(varName, val);
    return;
}

void Shader::ApplyUniforms() {
    for (const auto& [uniform, val]: uniform_vars) {
        std::cout << "setting uniform: '" << uniform << "' = " << val << '\n';
        this->setUniform(uniform, val);
    }
}

