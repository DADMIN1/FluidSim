#include "Simulation.hpp"

#include <iostream>


bool Simulation::Initialize()
{
    std::cout << "\n\nInitializing Simulation!\n";
    if (!diffusionField.Initialize()) { std::cerr << "diffusionField initialization failed!\n"; return false; }
    if (!fluid.Initialize()) { std::cerr << "fluid initialization failed!\n"; return false; }
    
    // finding/setting the initial cell for each Particle
    for (Fluid::Particle& particle: fluid.particles)
    {
        const auto& [x, y] = particle.getPosition();
        unsigned int xi = x / SPATIAL_RESOLUTION;
        unsigned int yi = y / SPATIAL_RESOLUTION;
        assert((xi <= DiffusionField_T::maxIX) && (yi <= DiffusionField_T::maxIY) && "out-of-bounds index");
        DiffusionField_T::Cell* cell = diffusionField.cellmatrix.at(xi).at(yi);
        particle.cellID = cell->UUID;
        particle.prevCellID = cell->UUID;
        cell->density += 1.0;
    }
    return true;
}


void Simulation::Update()
{
    if (isPaused) { return; }
    if (hasGravity) { fluid.ApplyGravity(); }
    fluid.UpdatePositions();
    fluid.UpdateDensities(diffusionField);
    fluid.ApplyDiffusion(diffusionField);
    
    return;
}
