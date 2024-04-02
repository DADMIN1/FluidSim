#ifndef FLUIDSIM_SIMULATION_HPP_INCLUDED
#define FLUIDSIM_SIMULATION_HPP_INCLUDED

#include "Diffusion.hpp"
#include "Fluid.hpp"

#include <unordered_set>
#include <map>

// holds info about a particle that has crossed into a new cell
struct Transition_T { const unsigned int particleID, oldCellID, newcellID; };
using TransitionList = std::vector<Transition_T>;

using UUID_Map_T = std::map<unsigned int, std::unordered_set<unsigned int>>;

struct CellDelta_T 
{
    std::unordered_set<unsigned int> particleIDs{};
    float density {0.0};
    //sf::Vector2f momentum{0.0, 0.0};
};
struct CellDelta_Dual_T { CellDelta_T positive, negative; };
using DeltaMap_T = std::map<unsigned int, CellDelta_Dual_T>; // maps cellID -> celldeltas


class Simulation
{
    DiffusionField diffusionField{};
    Fluid fluid{};
    UUID_Map_T particleMap{}; // mapping cellIDs to particleIDs
    
    bool hasGravity {false};
    bool useTransparency {false};  // slow-moving particles are more transparent
    bool isPaused{false};
    
    float momentumTransfer{0.20}; // percentage of velocity transferred to cell (and lost) by particle
    float momentumDistribution{0.75}; // percentage of cell's total momentum distributed to local particles per timestep
    
    // identifies Particles that have crossed a cell-boundary
    // does NOT update the Particles' cellID or the cells' density
    TransitionList FindCellTransitions() const;
    void HandleTransitions(const TransitionList& transitions);
    void UpdateParticles();
    
    public:
    bool Initialize();
    void Update();
    
    // mouse needs to access this pointer to lookup cell (given an X/Y coord)
    DiffusionField* GetDiffusionFieldPtr() { return &diffusionField; }  //TODO: get rid of this
    void PrintAllCells() { diffusionField.PrintAllCells(); }
    
    bool TogglePause() { isPaused = !isPaused; return isPaused; }
    bool ToggleGravity(bool noArg=true) // if you pass false, it always disables gravity
    { hasGravity = (noArg? (!hasGravity) : false); return hasGravity; }
    bool ToggleTransparency() { useTransparency = !useTransparency; return useTransparency; }
    
    void Freeze() // sets all velocities to 0
    {
        if(!isPaused) { TogglePause(); }
        fluid.Freeze();
        diffusionField.ResetMomentum();
        //hasGravity = false;
    }
    void Reset(); // TODO: implement reset
    
    sf::Sprite DrawFluid() { return fluid.Draw(useTransparency); }
    sf::Sprite DrawGrid () { return diffusionField.Draw(); }
};


// TODO: implement class RenderState to manage all visuals (including mouse)
// holds textures/sprites for each member
/* class RenderState
{
    sf::RenderTarget* target; // &mainwindow
    std::array<sf::RenderTexture, 5> texture;
    public:
    std::array<sf::Sprite, 5> sprites;
}; */


#endif
