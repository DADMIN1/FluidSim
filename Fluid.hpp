#ifndef FLUIDSIM_FLUID_HPP_INCLUDED
#define FLUIDSIM_FLUID_HPP_INCLUDED

#include <vector>
//#include <array>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/CircleShape.hpp>
//#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>

#include "Globals.hpp"


// disable dynamic frame-delay to compensate with a hardcoded ratio instead
#define DYNAMICFRAMEDELAY true
constexpr int framerateCap{300};
#if DYNAMICFRAMEDELAY
constexpr int delaycompN{1}, delaycompD{1};
#else
constexpr int delaycompN{13}, delaycompD{15}; // the sleepdelay is multiplied by 13/15 to compensate for the calculation-time per frame
// (with framerateCap=300): 298fps actual [delay=2888us] (with compensation) vs 262fps actual [delay=3333us] (without)
#endif
const sf::Time sleepDelay {sf::microseconds((1000000*delaycompN)/(framerateCap*delaycompD))};
// main.cpp
extern float timestepRatio;  // normalizing timesteps to make physics independent of frame-rate
//TODO: refactor all the framerate-related stuff out of this file


class Fluid
{
    float gravity {0.425};
    float bounceDampening {0.25};
    float viscosity {0.005};
    float fdensity {0.0075};  // controls 'force' of diffusion
    float vcap {7.5};
    
    class Particle : public sf::CircleShape
    {
        const unsigned int UUID;
        unsigned int cellID{0};
        sf::Vector2f velocity {0.0, 0.0};
        
        public:
        friend class Fluid;
        friend class Simulation;
        void UpdateColor(const bool useTransparency);
        
        Particle(const unsigned int ID, float radius=DEFAULTRADIUS, std::size_t pointcount=DEFAULTPOINTCOUNT) 
        : Particle::CircleShape(radius, pointcount), UUID{ID}
        {
            const sf::Color defaultcolor (0x0888FFFF);
            setFillColor(defaultcolor);
        }
    };
    
    sf::RenderTexture particle_texture;
    std::vector<Particle> particles;
    
    public:
    friend class Simulation;
    
    bool Initialize();
    void UpdatePositions();
    
    void ApplyGravity() {
        for (Particle& particle : particles) {
            particle.velocity.y += gravity*timestepRatio;
        }
    }

    void Freeze() // sets all velocities to 0
    {
        for (Particle& particle: particles) {
            particle.velocity = {0,0};
            particle.UpdateColor(false); // even if transparency is enabled, non-moving particles should be opaque
        }
    }
    
    sf::Sprite Draw(const bool useTransparency)
    {
        particle_texture.clear(sf::Color::Transparent);
        for (Particle& particle: particles) {
            particle.UpdateColor(useTransparency);
            particle_texture.draw(particle);
        }
        particle_texture.display();
        return sf::Sprite(particle_texture.getTexture());
    }
};


#endif
