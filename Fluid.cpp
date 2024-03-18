#include "Fluid.hpp"

//#include <vector>
#include <cassert>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>


bool Fluid::Initialize()
{
    assert((DEFAULTPOINTCOUNT > 0) && "Pointcount must be greater than 0");
    assert((DEFAULTRADIUS > 0.0) && "Radius must be greater than 0");
    assert((NUMCOLUMNS > 0) && (NUMROWS > 0) && "Columns and Rows must be greater than 0");
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
            particle.setPosition(c*DEFAULTRADIUS*2, r*DEFAULTRADIUS*2);
            // finding/setting initial cell
            auto& [x, y] = particle.getPosition();
            unsigned int xi = x/SPATIAL_RESOLUTION;
            unsigned int yi = y/SPATIAL_RESOLUTION;
            DiffusionField_T::Cell* cell = DiffusionFields[0].cellmatrix.at(xi).at(yi);
            particle.prevcellID = cell->UUID;
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
            particle.velocity.y += gravity * timestepRatio;
            
            sf::Vector2f nextPosition = particle.getPosition();
            nextPosition.x += particle.velocity.x * timestepRatio;
            nextPosition.y += particle.velocity.y * timestepRatio;

            //  keeping all particles within bounding box
            if (nextPosition.y >= BOXHEIGHT) {
                nextPosition.y = BOXHEIGHT-DEFAULTRADIUS;
                particle.velocity.y *= -1.0 + bounceDampening;
                assert(particle.velocity.y <= 0 && "y-velocity should be negative");
            }
            else if (nextPosition.y <= 0) {
                nextPosition.y = 0+DEFAULTRADIUS;
                particle.velocity.y *= -1.0 + bounceDampening;
                assert(particle.velocity.y >= 0 && "y-velocity should be positive");
            }

            if (nextPosition.x >= BOXWIDTH) {
                nextPosition.x = BOXWIDTH-DEFAULTRADIUS;
                particle.velocity.x *= -1.0 + bounceDampening;
                assert(particle.velocity.x <= 0 && "x-velocity should be negative");
            }
            else if (nextPosition.x <= 0) {
                nextPosition.x = 0+DEFAULTRADIUS;
                particle.velocity.x *= -1.0 + bounceDampening;
                assert(particle.velocity.x >= 0 && "x-velocity should be positive");
            }
            particle.setPosition(nextPosition);
        }
    }
}

void Fluid::UpdateDensities()
{
    for (auto& innervec: particles) {
        for (Particle& p: innervec) {
            auto& [x, y] = p.getPosition();
            unsigned int xi = x/SPATIAL_RESOLUTION;
            unsigned int yi = y/SPATIAL_RESOLUTION;
            DiffusionField_T::Cell* cell = DiffusionFields[0].cellmatrix.at(xi).at(yi);
            if (cell->UUID != p.prevcellID)
            {
                DiffusionFields[0].cells[p.prevcellID].density -= 1.0;
                p.prevcellID = cell->UUID;
                cell->density += 1.0;
            }
        }
    }
}
