#include "Fluid.hpp"
#include "Gradient.hpp"

//#include <vector>
#include <numeric>
#include <cassert>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>



void Particle::UpdateColor(bool useTransparency)
{
    float speed = std::abs(velocity.x) + std::abs(velocity.y);
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
    
    for (auto& F: DiffusionFields) {
        if (!F.Initialize())
            return false;
    }
    
    // TODO: does this reserve actually work? or do I need to reserve each nested vector as well?
    particles.reserve(NUMCOLUMNS*NUMROWS);
    particles.resize(NUMCOLUMNS);
    for (int c{0}; c < NUMCOLUMNS; ++c) { 
        particles[c].resize(NUMROWS);
        for (int r{0}; r < NUMROWS; ++r) {
            Particle& particle = particles[c][r];
            particle.setPosition((c*INITIALSPACINGX)+INITIALOFFSETX, (r*INITIALSPACINGY)+INITIALOFFSETY);
            
            // finding/setting initial cell
            const auto& [x, y] = particle.getPosition();
            unsigned int xi = x/SPATIAL_RESOLUTION;
            unsigned int yi = y/SPATIAL_RESOLUTION;
            assert((xi <= DiffusionField_T::maxIX) && (yi <= DiffusionField_T::maxIY) && "out-of-bounds index");
            DiffusionField_T::Cell* cell = DiffusionFields[0].cellmatrix.at(xi).at(yi);
            particle.cellID = cell->UUID;
            particle.prevCellID = cell->UUID;
            cell->density += 1.0;
        }
    }
    
    return true;
}

// we must not allow position == limit in this function; 
// otherwise, when we look up the related cell, it'll index past the end of the cellmatrix
void Fluid::Update()
{
    for (int c{0}; c < NUMCOLUMNS; ++c) { 
        for (int r{0}; r < NUMROWS; ++r) {
            Particle& particle = particles[c][r];
            
            if (hasGravity)
                particle.velocity.y += gravity;
            
            /* particle.velocity -= { 
                (particle.velocity.x>0.0? 1.0f : -1.0f) * viscosity, 
                (particle.velocity.y>0.0? 1.0f : -1.0f) * viscosity,
            }; */
            
            if (particle.velocity.x > vcap) {
                particle.velocity.x -= viscosity;
            }
            else if (particle.velocity.x < -vcap) {
                particle.velocity.x += viscosity;
            }
            if (particle.velocity.y > vcap) {
                particle.velocity.y -= viscosity;
            }
            else if (particle.velocity.y < -vcap) {
                particle.velocity.y += viscosity;
            }
            
            sf::Vector2f nextPosition = particle.getPosition();
            nextPosition.x += particle.velocity.x * timestepRatio;
            nextPosition.y += particle.velocity.y * timestepRatio;

            //  keeping all particles within bounding box
            if (nextPosition.y >= BOXHEIGHT) {
                nextPosition.y = BOXHEIGHT-DEFAULTRADIUS;
                particle.velocity.y *= (-1.0 + bounceDampening);
                particle.velocity.y -= 0.05f;  // forcing particles with 0 velocity away from the edge
                assert(particle.velocity.y <= 0 && "y-velocity should be negative");
            }
            else if (nextPosition.y <= 0) {
                nextPosition.y = -1*nextPosition.y;
                particle.velocity.y *= (-1.0 + bounceDampening);
                particle.velocity.y += 0.05f;  // forcing particles with 0 velocity away from the edge
                assert(particle.velocity.y >= 0 && "y-velocity should be positive");
            }

            if (nextPosition.x >= BOXWIDTH) {
                nextPosition.x = BOXWIDTH-DEFAULTRADIUS;
                particle.velocity.x *= (-1.0 + bounceDampening);
                particle.velocity.x -= 0.05f;   // forcing particles with 0 velocity away from the edge
                assert(particle.velocity.x <= 0 && "x-velocity should be negative");
            }
            else if (nextPosition.x <= 0) {
                nextPosition.x = -1*nextPosition.x;
                particle.velocity.x *= (-1.0 + bounceDampening);
                particle.velocity.x += 0.05f;   // forcing particles with 0 velocity away from the edge
                assert(particle.velocity.x >= 0 && "x-velocity should be positive");
            }
            
            particle.setPosition(nextPosition);
            particle.UpdateColor(useTransparency);
        }
    }
}

void Fluid::UpdateDensities()
{
    for (auto& innervec: particles) {
        for (Particle& particle: innervec) {
            const auto& [x, y] = particle.getPosition();
            unsigned int xi = x/SPATIAL_RESOLUTION;
            unsigned int yi = y/SPATIAL_RESOLUTION;
            DiffusionField_T::Cell* cell = DiffusionFields[0].cellmatrix.at(xi).at(yi);
            if (cell->UUID != particle.cellID)
            {
                DiffusionField_T::Cell& oldcell = DiffusionFields[0].cells[particle.cellID];
                /* float densityRatio = (oldcell.density+1.0) / (cell->density+1.0);
                particle.velocity *= densityRatio; */
                // can't do that calculation here because not every cell has it's density updated yet (it blows up)
                oldcell.density -= 1.0;
                cell->density += 1.0;
                particle.prevCellID = particle.cellID;
                particle.cellID = cell->UUID;
            }
            
            // finding position relative to center of the cell
            sf::Vector2f halfway = cell->getPosition() + sf::Vector2f{float(SPATIAL_RESOLUTION/2), float(SPATIAL_RESOLUTION/2)};
            particle.reldirections[0] = (x > halfway.x);
            particle.reldirections[1] = (y > halfway.y);
        }
    }
}

// expects that UpdateDensities has already been called for the frame
void Fluid::ApplyDiffusion()
{
    for (auto& innervec: particles) {
        for (Particle& particle: innervec) {
            const DiffusionField_T::Cell& cell = DiffusionFields[0].cells[particle.cellID];
            // TODO: implement this better
            if (particle.prevCellID != particle.cellID) // taking care of velocity scaling after cell transition
            {
                DiffusionField_T::Cell& oldcell = DiffusionFields[0].cells[particle.prevCellID];
                float densityRatio = (cell.density+1.0)/(oldcell.density+1.0); // accounting for the density added/subtracted by this particle
                // particle.velocity += particle.velocity*fdensity*(1.0f-densityRatio);
                particle.velocity -= particle.velocity*fdensity*densityRatio;
                particle.prevCellID = particle.cellID;  // only perform this calculation once
            }
            particle.velocity += DiffusionFields[0].CalcDiffusionVec(particle.cellID) * timestepRatio * fdensity;
            // Applying current cell's diffusion
            const float diffusionForce = (cell.density-1.0)*fdensity;
            const sf::Vector2f diffvec { diffusionForce*(particle.reldirections[0] ? 1.0f : -1.0f ), 
                                         diffusionForce*(particle.reldirections[1] ? 1.0f : -1.0f ), };
            particle.velocity += diffvec*timestepRatio;
            
            /* if (particle.velocity.x < 1.0) {
                particle.velocity.x += diffusionForce*(particle.reldirections[0] ? float(-1.0) : float(1.0));
            } */
        }
    }
}

