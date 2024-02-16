#include <vector>
#include <cassert>

// vscode can't find /usr/include/SFML/ for some reason
#include "SFML/Graphics.hpp"


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
    public:
    sf::Vector2f velocity {0.0, 0.0};
    
    Particle(float radius=DEFAULTRADIUS, std::size_t pointcount=DEFAULTPOINTCOUNT) 
    : Particle::CircleShape(radius, pointcount)
    {
        const sf::Color defaultcolor (0x0888FFFF);
        this->setFillColor(defaultcolor);
        assert((this->getRadius() == DEFAULTRADIUS) && "radius is not default");
    }
};


class Fluid 
{
    float gravity {0.5};
    float bounceDampening {0.15};
    sf::RenderWindow* drawtarget;
    std::vector<std::vector<Particle>> particles;

    void Initialize()
    {
        assert((DEFAULTPOINTCOUNT > 0) && "Pointcount must be greater than 0");
        assert((DEFAULTRADIUS > 0.0) && "Radius must be greater than 0");
        assert((NUMCOLUMNS > 0) && (NUMROWS > 0) && "Columns and Rows must be greater than 0");
        assert((bounceDampening >= 0.0) && (bounceDampening <= 1.0) && "collision-damping must be between 0 and 1");
        // TODO: does this reserve actually work? or do I need to reserve each nested vector as well?
        particles.reserve(NUMCOLUMNS*NUMROWS);
        particles.resize(NUMCOLUMNS);
        for (int c{0}; c < NUMCOLUMNS; ++c) { 
            particles[c].resize(NUMROWS);
            for (int r{0}; r < NUMROWS; ++r) {
                Particle& currentref = particles[c][r];
                currentref = Particle();
                currentref.setPosition(c*DEFAULTRADIUS*2, r*DEFAULTRADIUS*2);
            }
        }
    }

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

    void Update()
    {
        for (int c{0}; c < NUMCOLUMNS; ++c){ 
            for (int r{0}; r < NUMROWS; ++r) {
                Particle& currentref = particles[c][r];
                currentref.velocity.y += gravity * timestepRatio;
                
                sf::Vector2f nextPosition = currentref.getPosition();
                nextPosition.x += currentref.velocity.x * timestepRatio;
                nextPosition.y += currentref.velocity.y * timestepRatio;

                //  keeping all particles within bounding box
                if (nextPosition.y > BOXHEIGHT) {
                    nextPosition.y = BOXHEIGHT;
                    currentref.velocity.y *= -1.0 + bounceDampening;
                    assert(currentref.velocity.y < 0 && "y-velocity should be negative");
                }
                else if (nextPosition.y < 0) {
                    nextPosition.y = 0;
                    currentref.velocity.y *= -1.0 + bounceDampening;
                    assert(currentref.velocity.y > 0 && "y-velocity should be positive");
                }

                if (nextPosition.x > BOXWIDTH) {
                    nextPosition.x = BOXWIDTH;
                    currentref.velocity.x *= -1.0 + bounceDampening;
                    assert(currentref.velocity.x < 0 && "x-velocity should be negative");
                }
                else if (nextPosition.x < 0) {
                    nextPosition.x = 0;
                    currentref.velocity.x *= -1.0 + bounceDampening;
                    assert(currentref.velocity.x > 0 && "x-velocity should be positive");
                }
                currentref.setPosition(nextPosition);
            }
        }
    }
};


