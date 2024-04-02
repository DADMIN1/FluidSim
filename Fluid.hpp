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
//TODO: timestepRatio needs to adjust when framerate is below cap
//TODO: refactor all the framerate-related stuff out of this file


class Particle : public sf::CircleShape
{
    sf::Vector2f velocity {0.0, 0.0};
    unsigned int cellID {0}, prevCellID {0};
    bool reldirections[2] {true, true};  // tracks position relative to cell's center (X, Y axes)
    // 'true' indicates a positive direction on that axis
    public:
    friend class Fluid;
    friend class Simulation;
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
    float gravity {0.175};
    float bounceDampening {0.15};
    float viscosity {0.005};
    float fdensity {0.0125};  // controls 'force' of diffusion
    float vcap {7.5};
    
    sf::RenderTexture particle_texture;
    std::vector<std::vector<Particle>> particles;

    public:
    friend class Simulation;
    
    bool Initialize(DiffusionField_T& dfref);
    void Update(const bool applyGravity, const bool useTransparency);
    void UpdateDensities(DiffusionField_T& dfref);
    void ApplyDiffusion(const DiffusionField_T& dfref);
    
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
};


#endif
