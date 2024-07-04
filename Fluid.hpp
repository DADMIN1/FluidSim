#ifndef FLUIDSIM_FLUID_HPP_INCLUDED
#define FLUIDSIM_FLUID_HPP_INCLUDED

#include <vector>
//#include <array>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/CircleShape.hpp>
//#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>

#include "Globals.hpp"


class Fluid
{
    float gravity {0.295};
    float xgravity {0.295};
    float viscosity {0.005675};
    float fdensity {0.01975}; // controls 'force' of diffusion
    float bounceDampening {0.1915};
    float speedcap_soft{100.0};
    float speedcap_hard{200.0};
     bool isTurbulent{false};
    
    friend class Simulation;
    friend class MainGUI;
    friend struct FluidParameters; //defined in MainGUI
    
    static bool isParticleScalingPositive;
    static float gradient_thresholdLow;   // speed at which gradient begins to apply
    static float gradient_thresholdHigh;  // speed that caps out the gradient
    
    class Particle : public sf::CircleShape
    {
        const unsigned int UUID;
        unsigned int cellID{0};
        sf::Vector2f velocity {0.0, 0.0};
        
        friend class Fluid;
        friend class Simulation;
        friend class MainGUI;
        
        public:
        void UpdateColor(const bool useTransparency);
        
        Particle(const unsigned int ID, float radius=DEFAULTRADIUS, std::size_t pointcount=DEFAULTPOINTCOUNT) 
        : Particle::CircleShape(radius, pointcount), UUID{ID}
        {
            const sf::Color defaultcolor (0x0888FFFF);
            setFillColor(defaultcolor);
        }
        
        float Distance(const Particle& rh) const; // unused
        static float Distance(const Particle& lh, const Particle& rh); // unused
    };
    // calculates diffusion-force between particles within the same cell
    sf::Vector2f CalcLocalForce(const Particle& lh, const Particle& rh) const;
    // unfortunately, it can't be static because it uses 'fdensity'
    
    sf::RenderTexture particle_texture;
    std::vector<Particle> particles;
    
    void ApplySpeedcap(sf::Vector2f& velocity); // modifies passed ref
    void ApplyViscosity(sf::Vector2f& velocity) {
        velocity *= (1.0f - (viscosity * timestepRatio));
    }
    
    public:
    static bool ToggleParticleScaling() { 
        isParticleScalingPositive = !isParticleScalingPositive; 
        return isParticleScalingPositive; 
    }
    
    bool Initialize();
    void UpdatePositions();
    
    void Freeze() // sets all velocities to 0
    {
        for (Particle& particle: particles) {
            particle.velocity = {0,0};
            particle.UpdateColor(false); // even if transparency is enabled, non-moving particles should be opaque
        }
    }
    
    sf::Sprite GetSprite() { return sf::Sprite(particle_texture.getTexture()); }
    void Redraw(const bool useTransparency, const bool shouldClear) 
    {
        if(shouldClear) particle_texture.clear(sf::Color::Transparent);
        for (Particle& particle: particles) {
            particle.UpdateColor(useTransparency);
            particle_texture.draw(particle);
        }
        particle_texture.display();
    }
    
    // multithreaded version
    void UpdatePositions(const std::vector<Particle>::iterator sliceStart, const std::vector<Particle>::iterator sliceEnd, bool hasGravity, bool hasXGravity);
    void Reset();
};


void PrintSpeedcapInfo();


#endif
