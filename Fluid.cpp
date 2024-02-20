#include "Fluid.hpp"

//#include <vector>
#include <cassert>

// vscode can't find /usr/include/SFML/ for some reason
//#include "SFML/Graphics.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Time.hpp"


void Fluid::Initialize()
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


void Fluid::Update()
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

