#include "Simulation.hpp"

#include <iostream>


bool Simulation::Initialize()
{
    std::cout << "\n\nInitializing Simulation!\n";
    if (!diffusionField.Initialize()) { std::cerr << "diffusionField initialization failed!\n"; return false; }
    if (!fluid.Initialize(diffusionField)) { std::cerr << "fluid initialization failed!\n"; return false; }
    
    return true;
}

void Simulation::Update()
{
    if (hasGravity) { fluid.ApplyGravity(); }
    fluid.UpdatePositions();
    fluid.UpdateDensities(diffusionField);
    fluid.ApplyDiffusion(diffusionField);
    
    return;
}
