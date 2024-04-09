#ifndef FLUIDSIM_SIMULATION_HPP_INCLUDED
#define FLUIDSIM_SIMULATION_HPP_INCLUDED

#include "Diffusion.hpp"
#include "Fluid.hpp"

#include <unordered_set>
#include <map>


// holds info about a particle that has crossed into a new cell
struct Transition_T {
    const unsigned int particleID, oldCellID, newCellID; 
    /*bool operator==(const Transition_T& other) const {
        return particleID == other.particleID;
    };*/
    enum Tag { particle, oldCell, newCell, };
    bool operator==(const std::pair<unsigned int, Tag>&& comp) const {
        const auto& [ID, tag] = comp;
        switch (tag) {
            case particle : return particleID == ID;
            case oldCell  : return oldCellID  == ID;
            case newCell  : return newCellID  == ID;
        }
    }
};
using TransitionList = std::vector<Transition_T>;

using UUID_Map_T = std::map<unsigned int, std::unordered_set<unsigned int>>;
using IDset_T = std::unordered_set<unsigned int>;

struct CellDelta_T 
{
    IDset_T particleIDs{};
    float density {0.0};
    //sf::Vector2f momentum{0.0, 0.0};
};
struct CellDelta_Dual_T { CellDelta_T positive, negative; };
using DeltaMap_T = std::map<unsigned int, CellDelta_Dual_T>; // maps cellID -> celldeltas
// TODO: replace DeltaMap_T with DeltaMap
struct DeltaMap
{
    std::map<unsigned int, CellDelta_T> positive;
    std::map<unsigned int, CellDelta_T> negative;
    DeltaMap(TransitionList&& list)
    {
        for (const auto& [particleID, oldCellID, newCellID]: list)
        {
            //positive.emplace(newCellID, particleID, 0.0);
            //negative.emplace(oldCellID, particleID, 0.0);
            positive[newCellID].particleIDs.emplace(particleID);
            negative[oldCellID].particleIDs.emplace(particleID);
            
        }
        for (auto& D: positive) { D.second.density = D.second.particleIDs.size(); }
        for (auto& D: negative) { D.second.density = -D.second.particleIDs.size(); }
    }
};


class Simulation
{
    DiffusionField diffusionField{};
    Fluid fluid{};
    UUID_Map_T particleMap{}; // mapping cellIDs to particleIDs
    
    bool hasGravity {false};
    bool useTransparency {false};  // slow-moving particles are more transparent
    bool isPaused{false};
    
    // TODO: scale these based on density
    float momentumTransfer{0.20}; // percentage of velocity transferred to cell (and lost) by particle
    float momentumDistribution{0.75}; // percentage of cell's total momentum distributed to local particles per timestep
    
    // identifies Particles that have crossed a cell-boundary
    // does NOT update the Particles' cellID or the cells' density
    TransitionList FindCellTransitions() const;
    void HandleTransitions(const TransitionList& transitions);
    void UpdateParticles();
    void LocalDiffusion(const IDset_T& particleset); // diffusion within a single cell
    void NonLocalDiffusion(const IDset_T& originset, const IDset_T& adjacentset); // diffusion across cells
    IDset_T BuildAdjacentSet(const std::size_t cellID, const IDset_T& excluded);
    
    public:
    bool Initialize();
    void Update();
    void Step(); // TODO: implement this
    
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
