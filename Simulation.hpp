#ifndef FLUIDSIM_SIMULATION_HPP_INCLUDED
#define FLUIDSIM_SIMULATION_HPP_INCLUDED

#include "Diffusion.hpp"
#include "Fluid.hpp"

#include <unordered_set>
#include <map>
#include <thread> // std::mutex
#include <random>

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
            default       : return false;
        }
    }
};
using TransitionList = std::vector<Transition_T>;

using UUID_Map_T = std::map<unsigned int, std::unordered_set<unsigned int>>;
using IDset_T = std::unordered_set<unsigned int>;

struct CellDelta_T 
{
    //const unsigned int cellUUID;
    IDset_T particlesAdded{};
    IDset_T particlesRemoved{};
    float density {0.0};
    sf::Vector2f velocities {0.0, 0.0};
};


// TODO: replace DeltaMap_T with DeltaMap
struct DeltaMap
{
    std::map<unsigned int, CellDelta_T> cellmap; // key is cellUUID
    TransitionList transitionlist;
    /* auto begin() { return cellmap.begin(); }
    auto end()   { return cellmap.end();   } */
    // loses info about transitionlist, but that's fine because it's unused
    
    DeltaMap() {}
    /* DeltaMap(decltype(cellmap.begin()) B, decltype(cellmap.end()) E): cellmap(B, E) {} */
    DeltaMap(DeltaMap&& other): cellmap{std::move(other.cellmap)}, transitionlist{other.transitionlist} {}
    DeltaMap(TransitionList&& list) : transitionlist{list}
    {
        for (const auto& [particleID, oldCellID, newCellID]: list)
        {
            cellmap[newCellID].particlesAdded.emplace(particleID);
            cellmap[oldCellID].particlesRemoved.emplace(particleID);
        }
        for (auto& [cellID, celldelta]: cellmap) {
            celldelta.density = celldelta.particlesAdded.size() - celldelta.particlesRemoved.size();
        }
        // TODO: calculate and store momentumDelta here instead of HandleTransitions()
    }
    
    void Combine(DeltaMap&& other)
    {
        //this->transitionlist.insert(transitionlist.end(), other.transitionlist.begin(), other.transitionlist.end());
        for (auto&& x : std::move(other.transitionlist)) {
            transitionlist.emplace_back(x);
        }
        
        // merge extracts/moves elements with new keys
        this->cellmap.merge(other.cellmap);
        // combine the remaining keys
        for (auto&& [key, other_delta]: std::move(other.cellmap))
        {
            auto& entry = this->cellmap[key];
            entry.particlesAdded.merge(other_delta.particlesAdded);
            entry.particlesRemoved.merge(other_delta.particlesRemoved);
            entry.density += other_delta.density;
            entry.velocities += other_delta.velocities;
        }
    }
};


class Simulation
{
    DiffusionField diffusionField{};
    Fluid fluid{};
    UUID_Map_T particleMap{}; // mapping cellIDs to particleIDs
    std::mutex write_mutex;
    std::random_device RNG; // TODO: savestates
    float normalizedRNG() {
        return RNG() / RNG.max();
    }
    
    bool hasGravity {false};
    bool hasXGravity {false};
    bool useTransparency {false};  // slow-moving particles are more transparent
    bool isPaused{false};
    friend int main(int argc, char** argv); // only so that the turbulence render block can check 'isPaused'
    
    // TODO: scale these based on density
    float momentumTransfer{0.165}; // percentage of velocity transferred to cell (and lost) by particle
    float momentumDistribution{0.25}; // percentage of cell's total momentum distributed to local particles per timestep
    
    // identifies Particles that have crossed a cell-boundary
    // does NOT update the Particles' cellID or the cells' density
    TransitionList FindCellTransitions() const;
    DeltaMap FindCellTransitions(const auto& particles_slice) const; // multithreaded version
    void HandleTransitions(std::map<unsigned int, CellDelta_T>&& cellmap); // cellmap-parameter gets eaten by this function (invalidated)
    void UpdateParticles();
    void LocalDiffusion(const IDset_T& particleset); // diffusion within a single cell
    void NonLocalDiffusion(const IDset_T& originset, const IDset_T& adjacentset); // diffusion across cells
    IDset_T BuildAdjacentSet(const std::size_t cellID /* , const IDset_T& excluded */);
    
    friend class MainGUI;
    friend struct SimulParameters; // MainGUI
    
    public:
    bool Initialize();
    void Update();
    void Step(); // TODO: implement this
    
    // mouse needs to access this pointer to lookup cell (given an X/Y coord)
    DiffusionField* GetDiffusionFieldPtr() { return &diffusionField; }  //TODO: get rid of this
    void PrintAllCells() { diffusionField.PrintAllCells(); }
    
    bool TogglePause() { isPaused = !isPaused; return isPaused; }
    bool SetPause(bool newState) { bool oldState = isPaused; isPaused = newState; return oldState; }
    bool ToggleGravity(bool xGrav, bool noArg=true) { // if you pass false, it always disables gravity
        bool& grav = (xGrav? hasXGravity: hasGravity);
        grav = (noArg? (!grav) : false); return grav;
    }
    bool ToggleTransparency() { useTransparency = !useTransparency; return useTransparency; }
    bool ToggleTurbulence() { fluid.isTurbulent = !fluid.isTurbulent; return fluid.isTurbulent; }
    
    void Freeze() // sets all velocities to 0
    {
        if(!isPaused) { TogglePause(); }
        fluid.Freeze();
        diffusionField.ResetMomentum();
        //hasGravity = false;
    }
    void Reset(); // TODO: implement reset
    
    void RedrawGrid()  { diffusionField.Redraw(); }
    void RedrawFluid() { fluid.Redraw(useTransparency); }
    auto GetSprites() { 
        struct Sprites{sf::Sprite gridSprite; sf::Sprite fluidSprite;};
        return Sprites{diffusionField.GetSprite(), fluid.GetSprite()};
    }
};

//#define PMEMPTYCOUNTER
#ifdef PMEMPTYCOUNTER
extern long long pmemptycounter;
#endif

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
