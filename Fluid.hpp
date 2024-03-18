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
    unsigned int prevcellID {0};
    public:
    friend class Fluid;
    
    Particle(float radius=DEFAULTRADIUS, std::size_t pointcount=DEFAULTPOINTCOUNT) 
    : Particle::CircleShape(radius, pointcount)
    {
        const sf::Color defaultcolor (0x0888FFFF);
        this->setFillColor(defaultcolor);
    }
};


class Fluid 
{
    float gravity {0.5};
    float bounceDampening {0.15};
    float viscosity {1.0};
    float density {1.0};  // controls 'force' of diffusion
    sf::RenderTexture particle_texture;
    std::vector<std::vector<Particle>> particles;

    std::array<DiffusionField_T, 2> DiffusionFields;
    //bool buffer_index{0}; // which diffusion field is being used as the 'current' (not working) buffer
    // why even bother swapping them? A copy to the other buffer will need to be made regardless
    //inline void SwapStateBuffers() { buffer_index = !buffer_index; }

    public:
    //const DiffusionField_T* state;
    bool Initialize();
    void Update();
    void UpdateDensities();
    //void ModifyVelocities();  // TODO: implement this
    
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
        return DiffusionFields[0].Draw();
    }
};


#endif
