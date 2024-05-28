#include "Fluid.hpp"
#include "Gradient.hpp"

//#include <vector>
//#include <numeric>
#include <cmath>
#include <cassert>
#include <array>    // required only for speedcap_counter/Stats
#include <iostream> // required only for speedcap_counter/Stats

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>


bool Fluid::isParticleScalingPositive = true;

// counts how many times the speedcaps were broken
static std::array<std::size_t, 4> speedcap_counter{0,0,0,0};
// counts the number of times two particles shared EXACTLY the same position (in CalcLocalForce)
static int exactOverlapCounter{0};

void PrintSpeedcapInfo()
{
    std::cout << '\n';
    std::cout << "speedcap_x: \t"
        << "soft: " << speedcap_counter[0] << " | "
        << "hard: " << speedcap_counter[2] << '\n';
    std::cout << "speedcap_y: \t"
        << "soft: " << speedcap_counter[1] << " | "
        << "hard: " << speedcap_counter[3] << '\n';
    std::cout << '\n';
    
    std::cout << "Exact overlaps: " << exactOverlapCounter << '\n';
    return;
}


void Fluid::ApplySpeedcap(sf::Vector2f& velocity)
{
    // TODO: figure out something better for the softcap
    if      (std::abs(velocity.x) > speedcap_hard) [[unlikely]] { velocity.x  = 0.0f; ++speedcap_counter[2]; }
    else if (std::abs(velocity.x) > speedcap_soft) [[unlikely]] { velocity.x *= 0.5f; ++speedcap_counter[0]; }
    if      (std::abs(velocity.y) > speedcap_hard) [[unlikely]] { velocity.y  = 0.0f; ++speedcap_counter[3]; }
    else if (std::abs(velocity.y) > speedcap_soft) [[unlikely]] { velocity.y *= 0.5f; ++speedcap_counter[1]; }
    // do NOT return in the if-statements (or you might be left with the other axis still invalid)
    return;
    
    // TODO: figure out why the hardcap implemented below doesn't work (sometimes)
    /* const auto startVel = velocity;
    const sf::Vector2f softcap_delta = (velocity-sf::Vector2f{speedcap_soft,speedcap_soft})/2.0f;
    const sf::Vector2f hardcap_delta = (velocity-sf::Vector2f{speedcap_hard,speedcap_hard});
    
    if      (std::abs(velocity.x) > speedcap_hard) { velocity.x -= hardcap_delta.x; assert((std::abs(velocity.x) < speedcap_hard       ) && "bad caps"); }
    else if (std::abs(velocity.x) > speedcap_soft) { velocity.x -= softcap_delta.x; assert((std::abs(velocity.x) < std::abs(startVel.x)) && "bad caps"); }
    if      (std::abs(velocity.y) > speedcap_hard) { velocity.y -= hardcap_delta.y; assert((std::abs(velocity.y) < speedcap_hard       ) && "bad caps"); }
    else if (std::abs(velocity.y) > speedcap_soft) { velocity.y -= softcap_delta.y; assert((std::abs(velocity.y) < std::abs(startVel.y)) && "bad caps"); }
    
    return; */
}


float Fluid::gradient_thresholdLow{0.75f};  // speed at which gradient begins to apply
float Fluid::gradient_thresholdHigh{40.75f};  // caps out the gradient


void Fluid::Particle::UpdateColor(const bool useTransparency)
{
    const float speed = std::abs(velocity.x) + std::abs(velocity.y);
    float inputRange = gradient_thresholdHigh-gradient_thresholdLow;
    unsigned int speedindex;
    const unsigned int baseAlpha = (useTransparency? 0xC0 : 0xFF);
    unsigned char alpha = baseAlpha;  // eventually converted to sf::Uint8 - which is unsigned char (not int)
    if (speed <= gradient_thresholdLow) { speedindex = 0; setScale({1.0f, 1.0f}); }
    else if (speed >= gradient_thresholdHigh) { speedindex = 1023; }  // size of gradient
    else {
        speedindex = (speed - gradient_thresholdLow) * (1023.f/inputRange);
        assert(speedindex <= 1023);
        if (useTransparency) {
            alpha = baseAlpha + ((speed - gradient_thresholdLow) * ((0xFF-baseAlpha)/inputRange));
            assert(alpha < 256);
        }
        float particleScaling = ((speed - gradient_thresholdLow)/inputRange);
        // faster particles grow
        float scale = 1.0f + ((Fluid::isParticleScalingPositive)? particleScaling : -particleScaling);
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


// calculates diffusion-force between particles within the same cell
sf::Vector2f Fluid::CalcLocalForce(const Fluid::Particle& lh, const Fluid::Particle& rh) const
{
    // the distance between two opposite corners of a cell (pythagorean theorem)
    constexpr float intracellDistMax{std::sqrt(SPATIAL_RESOLUTION*SPATIAL_RESOLUTION*2)};
    constexpr float maxdist = intracellDistMax*(radialdist_limit+1);
    
    const auto [diffx, diffy] = lh.getPosition() - rh.getPosition();
    const float totalDistance = std::sqrt((diffx*diffx) + (diffy*diffy));
    if (totalDistance == 0) [[unlikely]] { ++exactOverlapCounter; return {0,0}; }  // TODO: should return random direction, ideally
    
    // mapping to output range of: 0 to PI/2 (cosine hits zero at PI/2)
    const float normalized = (totalDistance/maxdist) * (M_PI/2);
    const float cosine_cubed = std::cos(normalized)*std::cos(normalized)*std::cos(normalized);
    //const float cosine_squared = std::cos(normalized)*std::cos(normalized);
    const float magnitude = cosine_cubed * fdensity;
    
    // TODO: division by zero here if diffx AND diffy both equal zero
    const float denominator = (std::abs(diffx) + std::abs(diffy));
    const sf::Vector2f directionalRatio {diffx/denominator, diffy/denominator};
    
    return directionalRatio*magnitude*timestepRatio;
}


void Fluid::UpdatePositions()
{
    for (Particle& particle: particles) 
    {
        ApplyViscosity(particle.velocity);
        ApplySpeedcap(particle.velocity);
        
        sf::Vector2f nextPosition = particle.getPosition();
        nextPosition.x += particle.velocity.x * timestepRatio;
        nextPosition.y += particle.velocity.y * timestepRatio;
        
        // TODO: refactor bounding-box checks into a seperate function
        //  keeping all particles within bounding box
        // we must not allow position == limit in this function;
        // otherwise, when we look up the related cell, it'll index past the end of the cellmatrix
        if (nextPosition.y > BOXHEIGHT) {
            nextPosition.y = BOXHEIGHT-DEFAULTRADIUS;
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        else if (nextPosition.y < 0) {
            nextPosition.y = -1*nextPosition.y;
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        
        if (nextPosition.x > BOXWIDTH) {
            nextPosition.x = BOXWIDTH-DEFAULTRADIUS;
            particle.velocity.x *= (-1.0 + bounceDampening);
        }
        else if (nextPosition.x < 0) {
            nextPosition.x = -1*nextPosition.x;
            particle.velocity.x *= (-1.0 + bounceDampening);
        }
        
        assert((nextPosition.x >= 0) && (nextPosition.y >= 0) && "negative nextPosition!");
        assert((nextPosition.x <= BOXWIDTH) && (nextPosition.y <= BOXHEIGHT) && "OOB nextPosition!");
        
        particle.setPosition(nextPosition);
    }
    
    return;
}

// multithreaded version
void Fluid::ApplyGravity(const std::vector<Particle>::iterator sliceStart, const std::vector<Particle>::iterator sliceEnd)
{
    for (std::vector<Particle>::iterator iter{sliceStart}; iter < sliceEnd; ++iter) {
        Particle& particle = *iter;
        particle.velocity.y += gravity*timestepRatio;
    }
}


// multithreaded version
void Fluid::UpdatePositions(const std::vector<Particle>::iterator sliceStart, const std::vector<Particle>::iterator sliceEnd)
{
    for (std::vector<Particle>::iterator iter{sliceStart}; iter < sliceEnd; ++iter)
    {
        Particle& particle = *iter;
        ApplyViscosity(particle.velocity);
        ApplySpeedcap(particle.velocity);
        
        sf::Vector2f nextPosition = particle.getPosition();
        nextPosition.x += particle.velocity.x * timestepRatio;
        nextPosition.y += particle.velocity.y * timestepRatio;
        
        // TODO: refactor bounding-box checks into a seperate function
        //  keeping all particles within bounding box
        // we must not allow position == limit in this function;
        // otherwise, when we look up the related cell, it'll index past the end of the cellmatrix
        if (nextPosition.y > BOXHEIGHT) {
            nextPosition.y = BOXHEIGHT-DEFAULTRADIUS;
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        else if (nextPosition.y < 0) {
            nextPosition.y = -1*nextPosition.y;
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        
        if (nextPosition.x > BOXWIDTH) {
            nextPosition.x = BOXWIDTH-DEFAULTRADIUS;
            particle.velocity.x *= (-1.0 + bounceDampening);
        }
        else if (nextPosition.x < 0) {
            nextPosition.x = -1*nextPosition.x;
            particle.velocity.x *= (-1.0 + bounceDampening);
        }
        
        assert((nextPosition.x >= 0) && (nextPosition.y >= 0) && "negative nextPosition!");
        assert((nextPosition.x <= BOXWIDTH) && (nextPosition.y <= BOXHEIGHT) && "OOB nextPosition!");
        
        particle.setPosition(nextPosition);
    }
    
    return;
}

