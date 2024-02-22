#ifndef FLUIDSIM_FLUID_HPP_INCLUDED
#define FLUIDSIM_FLUID_HPP_INCLUDED

#include <vector>

//#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>


#define DYNAMICFRAMEDELAY false
// if false, tries to compensate with a hardcoded ratio instead

constexpr int NUMCOLUMNS {50}, NUMROWS {25};
constexpr int BOXWIDTH {1000}, BOXHEIGHT {1000};
constexpr float DEFAULTRADIUS {float(BOXWIDTH/NUMCOLUMNS)/2.0};
constexpr int DEFAULTPOINTCOUNT {30};


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
    sf::RenderWindow* drawtarget;
    std::vector<std::vector<Particle>> particles;

    void Initialize();

    public:
    void Draw()
    {    
        for (int c{0}; c < NUMCOLUMNS; ++c){ 
            for (int r{0}; r < NUMROWS; ++r) {
                drawtarget->draw(particles[c][r]);
            }
        }
    }

    Fluid(sf::RenderWindow* window) : drawtarget{window}
    {
        Initialize();
    }

    void Update();
};


#endif