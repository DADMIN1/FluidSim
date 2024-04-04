#include "Fluid.hpp"
#include "Gradient.hpp"

//#include <vector>
//#include <numeric>
#include <cmath>
#include <cassert>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>


void Fluid::Particle::UpdateColor(const bool useTransparency)
{
    const float speed = std::abs(velocity.x) + std::abs(velocity.y);
    constexpr float thresholdLow{0.5f};  // speed at which gradient begins to apply
    constexpr float thresholdHigh{32.5f};  // caps out the gradient
    constexpr float inputRange = thresholdHigh-thresholdLow;
    unsigned int speedindex;
    const unsigned int baseAlpha = (useTransparency? 0xC0 : 0xFF);
    unsigned char alpha = baseAlpha;  // eventually converted to sf::Uint8 - which is unsigned char (not int)
    if (speed <= thresholdLow) { speedindex = 0; setScale({1.1f, 1.1f}); }
    else if (speed >= thresholdHigh) { speedindex = 1023; }  // size of gradient
    else {
        speedindex = (speed - thresholdLow) * (1023.f/inputRange);
        assert(speedindex <= 1023);
        if (useTransparency) {
            alpha = baseAlpha + ((speed - thresholdLow) * ((0xFF-baseAlpha)/inputRange));
            assert(alpha < 256);
        }
        // faster particles shrink
        float scale = 1.1f - ((speed - thresholdLow)/inputRange);
        setScale({scale, scale});
    }
    auto&&[r, g, b, a] = Gradient_T::Lookup(speedindex);
    setFillColor({r,g,b,alpha});
    return;
}


bool Fluid::Initialize()
{
    assert((bounceDampening >= 0.0) && (bounceDampening <= 1.0) && "collision-damping must be between 0 and 1");
    
    if (!particle_texture.create(BOXWIDTH, BOXHEIGHT))
        return false;
    
    unsigned int nextID{0};
    particles.reserve(NUMCOLUMNS*NUMROWS);
    for (int c{0}; c < NUMCOLUMNS; ++c) { 
        for (int r{0}; r < NUMROWS; ++r) {
            Particle& particle = particles.emplace_back(nextID++);
            particle.setPosition((c*INITIALSPACINGX)+INITIALOFFSETX, (r*INITIALSPACINGY)+INITIALOFFSETY);
            // the particles still need their Cell-related variables set
            // and the cells need to have their density increased
        }
    }
    
    return true;
}


float Fluid::Particle::Distance(const Particle& rh) const
{
    auto [diffx, diffy] = this->getPosition() - rh.getPosition();
    return std::sqrt((diffx*diffx) + (diffy*diffy)); // pythagorean theorem
}

float Fluid::Particle::Distance(const Particle& lh, const Particle& rh)
{
    auto [diffx, diffy] = lh.getPosition() - rh.getPosition();
    return std::sqrt((diffx*diffx) + (diffy*diffy));
}

// the distance between two opposite corners of a cell (pythagorean theorem)
static constexpr float intracellDistMax{std::sqrt(SPATIAL_RESOLUTION*SPATIAL_RESOLUTION*2)};

int exactOverlapCounter{0};

// calculates diffusion-force between particles within the same cell
sf::Vector2f Fluid::CalcLocalForce(const Fluid::Particle& lh, const Fluid::Particle& rh) const
{
    const auto [diffx, diffy] = lh.getPosition() - rh.getPosition();
    const float totalDistance = std::sqrt((diffx*diffx) + (diffy*diffy));
    if (totalDistance == 0) { ++exactOverlapCounter; return {0,0}; }  // TODO: should return random direction, ideally
    
    // mapping to output range of: 0 to PI/4 (cosine hits zero at PI/4)
    const float normalized = (totalDistance/intracellDistMax) * (M_PI/4);
    const float cosine_cubed = std::cos(normalized)*std::cos(normalized)*std::cos(normalized);
    const float magnitude = cosine_cubed * fdensity * timestepRatio;
    
    // TODO: division by zero here if diffx AND diffy both equal zero
    const float denominator = (std::abs(diffx) + std::abs(diffy));
    const sf::Vector2f directionalRatio {diffx/denominator, diffy/denominator};
    
    return directionalRatio*magnitude;
}

void Fluid::UpdatePositions()
{
    for (Particle& particle: particles) 
    {
        /* particle.velocity -= { 
            (particle.velocity.x>0.0? 1.0f : -1.0f) * viscosity, 
            (particle.velocity.y>0.0? 1.0f : -1.0f) * viscosity,
        }; */
        
        if (particle.velocity.x > vcap) {
            particle.velocity.x -= viscosity*timestepRatio;
        }
        else if (particle.velocity.x < -vcap) {
            particle.velocity.x += viscosity*timestepRatio;
        }
        if (particle.velocity.y > vcap) {
            particle.velocity.y -= viscosity*timestepRatio;
        }
        else if (particle.velocity.y < -vcap) {
            particle.velocity.y += viscosity*timestepRatio;
        }
        
        sf::Vector2f nextPosition = particle.getPosition();
        nextPosition.x += particle.velocity.x * timestepRatio;
        nextPosition.y += particle.velocity.y * timestepRatio;
        
        //  keeping all particles within bounding box
        // we must not allow position == limit in this function;
        // otherwise, when we look up the related cell, it'll index past the end of the cellmatrix
        if (nextPosition.y >= BOXHEIGHT) {
            nextPosition.y = BOXHEIGHT-DEFAULTRADIUS;
            particle.velocity.y *= (-1.0 + bounceDampening);
            particle.velocity.y -= 0.001f;  // forcing particles with 0 velocity away from the edge
            assert(particle.velocity.y <= 0 && "y-velocity should be negative");
        }
        else if (nextPosition.y <= 0) {
            nextPosition.y = -1*nextPosition.y;
            particle.velocity.y *= (-1.0 + bounceDampening);
            particle.velocity.y += 0.001f;  // forcing particles with 0 velocity away from the edge
            assert(particle.velocity.y >= 0 && "y-velocity should be positive");
        }
        
        if (nextPosition.x >= BOXWIDTH) {
            nextPosition.x = BOXWIDTH-DEFAULTRADIUS;
            particle.velocity.x *= (-1.0 + bounceDampening);
            particle.velocity.x -= 0.001f;   // forcing particles with 0 velocity away from the edge
            assert(particle.velocity.x <= 0 && "x-velocity should be negative");
        }
        else if (nextPosition.x <= 0) {
            nextPosition.x = -1*nextPosition.x;
            particle.velocity.x *= (-1.0 + bounceDampening);
            particle.velocity.x += 0.001f;   // forcing particles with 0 velocity away from the edge
            assert(particle.velocity.x >= 0 && "x-velocity should be positive");
        }
        
        particle.setPosition(nextPosition);
    }
}

