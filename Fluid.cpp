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


void Fluid::Particle::ApplySpeedcap()
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


float Fluid::gradient_thresholdLow{0.00f};   // speed at which gradient begins to apply
float Fluid::gradient_thresholdHigh{32.00f}; // caps out the gradient
Gradient_T* Fluid::activeGradient{nullptr};


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
    const auto[r, g, b, a] = activeGradient->Lookup(speedindex);
    setFillColor({r,g,b,alpha});
    return;
}


// calculations for initial positioning of particles
static constexpr float INITIALSPACINGX {BOXWIDTH/NUMCOLUMNS};
static constexpr float INITIALSPACINGY {BOXHEIGHT/NUMROWS};
static constexpr float INITIALOFFSETX {(INITIALSPACINGX/2.0f) - DEFAULTRADIUS};
static constexpr float INITIALOFFSETY {(INITIALSPACINGY/2.0f) - DEFAULTRADIUS};

bool Fluid::Initialize()
{
    assert((bounceDampening >= 0.0) && (bounceDampening <= 1.0) && "collision-damping must be between 0 and 1");
    assert((activeGradient != nullptr) && "Fluid's gradient was never set");
    
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

void Fluid::Reset()
{
    int c{0}; int r{0};
    for (Particle& particle: particles) {
        particle.cellID = -1;
        particle.velocity = {0,0};
        if (++c >= NUMCOLUMNS) { c=0; ++r; }
        particle.setPosition((c*INITIALSPACINGX)+INITIALOFFSETX, (r*INITIALSPACINGY)+INITIALOFFSETY);
        // the particles still need their Cell-related variables set, and the cells need to have their density increased
    }
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


// MSVC really doesn't believe that cmath functions( std::sqrt/cos) are constexpr (they are since C++23)
#define PLZ_STOPCOMPLAINING_MSCPP
#ifdef PLZ_STOPCOMPLAINING_MSCPP
  #define CONSTEXPR const
#else
  #define CONSTEXPR constexpr
#endif

// calculates diffusion-force between particles within the same cell
sf::Vector2f Fluid::CalcLocalForce(const Fluid::Particle& lh, const Fluid::Particle& rh, float fdensity)
{
    // the distance between two opposite corners of a cell (pythagorean theorem)
    CONSTEXPR float intracellDistMax{std::sqrt(SPATIAL_RESOLUTION*SPATIAL_RESOLUTION*2)};
    CONSTEXPR float maxdist = intracellDistMax*(radialdist_limit+1);
    
    const auto [diffx, diffy] = lh.getPosition() - rh.getPosition();
    const float totalDistance = std::sqrt((diffx*diffx) + (diffy*diffy));
    if(totalDistance == 0.f) [[unlikely]] { ++exactOverlapCounter; return -lh.velocity*timestepRatio*fdensity; }  // TODO: should return random direction, ideally
    
    // mapping to output range of: 0 to PI/2 (cosine hits zero at PI/2)
    const float normalized = (totalDistance/maxdist) * (M_PI/2.f);
    const float cosine_cubed = std::cos(normalized)*std::cos(normalized)*std::cos(normalized);
    const float magnitude = cosine_cubed * fdensity;
    
    // TODO: division by zero here if diffx AND diffy both equal zero
    const float denominator = (std::abs(diffx) + std::abs(diffy));
    const sf::Vector2f directionalRatio {diffx/denominator, diffy/denominator};
    
    return directionalRatio*magnitude*timestepRatio;
}

#undef CONSTEXPR


// particle positions still use top-left corner, so the non-zero boundary needs adjustment
#define ADJ_BOXHEIGHT (BOXHEIGHT-DEFAULTRADIUS)
#define ADJ_BOXWIDTH  ( BOXWIDTH-DEFAULTRADIUS)


void Fluid::UpdatePositions()
{
    for (Particle& particle: particles) 
    {
        particle.ApplyViscosity(viscosity);
        particle.ApplySpeedcap();
        
        sf::Vector2f nextPosition = particle.getPosition();
        nextPosition.x += particle.velocity.x * timestepRatio;
        nextPosition.y += particle.velocity.y * timestepRatio;
        
        // TODO: refactor bounding-box checks into a seperate function
        //  keeping all particles within bounding box
        // we must not allow position == limit in this function;
        // otherwise, when we look up the related cell, it'll index past the end of the cellmatrix
        if (nextPosition.y > ADJ_BOXHEIGHT) {
            nextPosition.y = BOXHEIGHT -(DEFAULTRADIUS*2.f);
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        else if (nextPosition.y < 0) {
            nextPosition.y = -1*nextPosition.y;
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        
        if (nextPosition.x > ADJ_BOXWIDTH) {
            nextPosition.x = BOXWIDTH - (DEFAULTRADIUS*2.f);
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
void Fluid::UpdatePositions(const std::vector<Particle>::iterator sliceStart, const std::vector<Particle>::iterator sliceEnd, bool hasGravity, bool hasXGravity)
{
    const float tgravity  = (hasGravity ?  gravity : 0.f);
    const float txgravity = (hasXGravity? xgravity : 0.f);
    
    for (std::vector<Particle>::iterator iter{sliceStart}; iter < sliceEnd; ++iter)
    {
        Particle& particle = *iter;
        particle.ApplyViscosity(viscosity);
        particle.velocity.y +=  tgravity*timestepRatio; // inlining gravity calc here
        particle.velocity.x += txgravity*timestepRatio;
        particle.ApplySpeedcap();
        
        sf::Vector2f nextPosition = particle.getPosition();
        nextPosition.x += particle.velocity.x * timestepRatio;
        nextPosition.y += particle.velocity.y * timestepRatio;
        
        // TODO: refactor bounding-box checks into a seperate function
        //  keeping all particles within bounding box
        // we must not allow position == limit in this function;
        // otherwise, when we look up the related cell, it'll index past the end of the cellmatrix
        if (nextPosition.y > ADJ_BOXHEIGHT) {
            nextPosition.y = BOXHEIGHT -(DEFAULTRADIUS*2.f);
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        else if (nextPosition.y < 0) {
            nextPosition.y = -1*nextPosition.y;
            particle.velocity.y *= (-1.0 + bounceDampening);
        }
        
        if (nextPosition.x > ADJ_BOXWIDTH) {
            nextPosition.x = BOXWIDTH - (DEFAULTRADIUS*2.f);
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


// static version
void Fluid::UpdatePositions(
    std::vector<Particle>::iterator sliceStart, std::vector<Particle>::iterator sliceEnd,
    sf::Vector2f gravityForces, float viscosityMultiplier, float bounceDampening)
{
    for (std::vector<Particle>::iterator iter{sliceStart}; iter!=sliceEnd; ++iter)
    {
        Particle& particle = *iter;
        sf::Vector2f nextPosition { particle.getPosition() + (particle.velocity+=gravityForces) };
        
        // handling reflections
        particle.velocity = sf::Vector2f {
          ((nextPosition.x < 0.f) || (nextPosition.x > ADJ_BOXWIDTH ))?
          -(particle.velocity.x*bounceDampening) : particle.velocity.x,
          ((nextPosition.y < 0.f) || (nextPosition.y > ADJ_BOXHEIGHT))?
          -(particle.velocity.y*bounceDampening) : particle.velocity.y,
        };
        
        nextPosition = sf::Vector2f {
          {(nextPosition.x < 0.f)? -nextPosition.x :
          ((nextPosition.x > ADJ_BOXWIDTH)? 
          (BOXWIDTH - (DEFAULTRADIUS*2.f)): nextPosition.x)},
          {(nextPosition.y < 0.f)? -nextPosition.y :
          ((nextPosition.y > ADJ_BOXHEIGHT)? 
          (BOXHEIGHT -(DEFAULTRADIUS*2.f)): nextPosition.y)},
        };
        
        particle.velocity *= viscosityMultiplier; particle.ApplySpeedcap();
        particle.setPosition(nextPosition);
    }
    
    return;
}
