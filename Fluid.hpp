#include <vector>
#include <cassert>

// vscode can't find /usr/include/SFML/ for some reason
#include "SFML/Graphics.hpp"

constexpr int NUMROWS {10}, NUMCOLUMNS {10};
constexpr int BOXWIDTH {1000}, BOXHEIGHT {1000};
constexpr float DEFAULTRADIUS {(BOXWIDTH/NUMROWS)/2};
constexpr int DEFAULTPOINTCOUNT {30};


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
    sf::RenderWindow* drawtarget;
    std::vector<std::vector<Particle>> particles;

    void Initialize()
    {
        particles.reserve(NUMROWS*NUMCOLUMNS);
        particles.resize(NUMROWS);
        for (int r{0}; r < NUMROWS; ++r) { 
            particles[r].resize(NUMCOLUMNS);
            for (int c{0}; c < NUMCOLUMNS; ++c) {
                Particle& currentref = particles[r][c];
                currentref = Particle();
                currentref.setPosition(r*DEFAULTRADIUS*2, c*DEFAULTRADIUS*2);
            }
        }
    }

    public:
    void Draw()
    {    
        for (int r{0}; r < NUMROWS; ++r){ 
            for (int c{0}; c < NUMCOLUMNS; ++c) {
                drawtarget->draw(particles[r][c]);
            }
        }
    }

    Fluid(sf::RenderWindow* window) : drawtarget{window}
    {
        Initialize();
    }

    void Update()
    {
        for (int r{0}; r < NUMROWS; ++r){ 
            for (int c{0}; c < NUMCOLUMNS; ++c) {
                Particle& currentref = particles[r][c];
                currentref.velocity.y += gravity;
                
                sf::Vector2f nextPosition = currentref.getPosition();
                nextPosition.x += currentref.velocity.x;
                nextPosition.y += currentref.velocity.y;

                //  keeping all particles within bounding box
                if (nextPosition.y > BOXHEIGHT) {
                    nextPosition.y = BOXHEIGHT;
                    currentref.velocity.y *= -1.0;
                    assert(currentref.velocity.y < 0 && "y-velocity should be negative");
                }
                else if (nextPosition.y < 0) {
                    nextPosition.y = 0;
                    currentref.velocity.y *= -1.0;
                    assert(currentref.velocity.y > 0 && "y-velocity should be positive");
                }

                if (nextPosition.x > BOXWIDTH) {
                    nextPosition.x = BOXWIDTH;
                    currentref.velocity.x *= -1.0;
                    assert(currentref.velocity.x < 0 && "x-velocity should be negative");
                }
                else if (nextPosition.x < 0) {
                    nextPosition.x = 0;
                    currentref.velocity.x *= -1.0;
                    assert(currentref.velocity.x > 0 && "x-velocity should be positive");
                }
                currentref.setPosition(nextPosition);
            }
        }
    }
};


