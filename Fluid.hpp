#ifndef FLUIDSIM_FLUID_HPP_INCLUDED
#define FLUIDSIM_FLUID_HPP_INCLUDED

#include <vector>
//#include <array>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/CircleShape.hpp>
//#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>

#include "Globals.hpp"
#include "Diffusion.hpp"


// disable dynamic frame-delay to compensate with a hardcoded ratio instead
#define DYNAMICFRAMEDELAY false
constexpr int framerateCap{300};
#if DYNAMICFRAMEDELAY
constexpr int delaycompN{1}, delaycompD{1};
#else
constexpr int delaycompN{13}, delaycompD{15}; // the sleepdelay is multiplied by 13/15 to compensate for the calculation-time per frame
// (with framerateCap=300): 298fps actual [delay=2888us] (with compensation) vs 262fps actual [delay=3333us] (without)
#endif
const sf::Time sleepDelay {sf::microseconds((1000000*delaycompN)/(framerateCap*delaycompD))};
constexpr float timestepRatio {1.0/float(framerateCap/60)};  // normalizing timesteps to make physics independent of frame-rate


class Particle : public sf::CircleShape
{
    sf::Vector2f velocity {0.0, 0.0};
    unsigned int cellID {0}, prevCellID {0};
    bool reldirections[2] {true, true};  // tracks position relative to cell's center (X, Y axes)
    // 'true' indicates a positive direction on that axis
    public:
    friend class Fluid;
    void UpdateColor(bool useTransparency);
    
    Particle(float radius=DEFAULTRADIUS, std::size_t pointcount=DEFAULTPOINTCOUNT) 
    : Particle::CircleShape(radius, pointcount)
    {
        const sf::Color defaultcolor (0x0888FFFF);
        this->setFillColor(defaultcolor);
    }
};


class Fluid 
{
    bool hasGravity {false};
    float gravity {0.175};
    float bounceDampening {0.15};
    float viscosity {0.005};
    float fdensity {0.0125};  // controls 'force' of diffusion
    float vcap {7.5};
    bool useTransparency {false};  // slow-moving particles are more transparent
    sf::RenderTexture particle_texture;
    std::vector<std::vector<Particle>> particles;

    // std::array<DiffusionField_T, 2> DiffusionFields;
    DiffusionField_T DiffusionField;
    //bool buffer_index{0}; // which diffusion field is being used as the 'current' (not working) buffer
    // why even bother swapping them? A copy to the other buffer will need to be made regardless
    //inline void SwapStateBuffers() { buffer_index = !buffer_index; }

    public:
    // mouse needs to access this pointer to lookup cell (given an X/Y coord)
    DiffusionField_T* GetDiffusionFieldPtr() { return &DiffusionField; }
    void PrintAllCells() {DiffusionField.PrintAllCells();}
    
    bool ToggleGravity(bool noArg=true) // if you pass false, it always disables
    { 
        if (noArg) hasGravity = !hasGravity;
        else hasGravity = false;
        return hasGravity; 
    }
    bool ToggleTransparency() { useTransparency = !useTransparency; return useTransparency; }
    //const DiffusionField_T* state;
    bool Initialize();
    void Update();
    void UpdateDensities();
    void ApplyDiffusion();
    void Freeze() // sets all velocities to 0
    {
        for (auto& column: particles) {
            for (Particle& particle: column) {
                particle.velocity = {0, 0};
                particle.UpdateColor(useTransparency);
            }
        }
        hasGravity = false;
    }
    void Reset(); // TODO: implement reset
    
    sf::Sprite Draw()
    {
        particle_texture.clear(sf::Color::Transparent);
        for (int c{0}; c < NUMCOLUMNS; ++c){ 
            for (int r{0}; r < NUMROWS; ++r) {
                //particles[c][r].UpdateColor();
                particle_texture.draw(particles[c][r]);
            }
        }
        particle_texture.display();
        return sf::Sprite(particle_texture.getTexture());
    }
    
    sf::Sprite DrawGrid()
    {
        // DiffusionFields[0]  // update densities here
        return DiffusionField.Draw();
    }
};


#endif
