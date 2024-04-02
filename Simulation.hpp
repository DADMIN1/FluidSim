#ifndef FLUIDSIM_SIMULATION_HPP_INCLUDED
#define FLUIDSIM_SIMULATION_HPP_INCLUDED

#include "Diffusion.hpp"
#include "Fluid.hpp"


class Simulation
{
    DiffusionField_T diffusionField{};
    Fluid fluid{};
    
    bool hasGravity {false};
    bool useTransparency {false};  // slow-moving particles are more transparent
    
    public:
    bool Initialize();
    void Update();
    
    // mouse needs to access this pointer to lookup cell (given an X/Y coord)
    DiffusionField_T* GetDiffusionFieldPtr() { return &diffusionField; }
    void PrintAllCells() { diffusionField.PrintAllCells(); }
    
    bool ToggleGravity(bool noArg=true) // if you pass false, it always disables gravity
    { hasGravity = (noArg? (!hasGravity) : false); return hasGravity; }
    bool ToggleTransparency() { useTransparency = !useTransparency; return useTransparency; }
    
    void Freeze() // sets all velocities to 0
    {
        fluid.Freeze();
        //hasGravity = false;
    }
    void Reset(); // TODO: implement reset
    
    sf::Sprite DrawFluid() { return fluid.Draw(useTransparency); }
    sf::Sprite DrawGrid () { return diffusionField.Draw(); }
};


#endif
