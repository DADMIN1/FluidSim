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
    fluid.Update(hasGravity, useTransparency);
    fluid.UpdateDensities(diffusionField);
    fluid.ApplyDiffusion(diffusionField);
}
