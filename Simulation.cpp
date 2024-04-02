#include "Simulation.hpp"

#include <iostream>
#include <tuple>


void NegativeMerge(std::unordered_set<unsigned int>& source, const std::unordered_set<unsigned int>& toRemove)
{
    for (unsigned int thing: toRemove) {
        source.erase(thing);
    }
}


bool Simulation::Initialize()
{
    std::cout << "\n\nInitializing Simulation!\n";
    if (!diffusionField.Initialize()) { std::cerr << "diffusionField initialization failed!\n"; return false; }
    if (!fluid.Initialize()) { std::cerr << "fluid initialization failed!\n"; return false; }
    
    // finding/setting the initial cell for each Particle
    for (Fluid::Particle& particle: fluid.particles)
    {
        const auto& [x, y] = particle.getPosition();
        const unsigned int xi = x / SPATIAL_RESOLUTION;
        const unsigned int yi = y / SPATIAL_RESOLUTION;
        assert((xi <= DiffusionField::maxIX) && (yi <= DiffusionField::maxIY) && "out-of-bounds index");
        DiffusionField::Cell* cell = diffusionField.cellmatrix.at(xi).at(yi);
        
        particleMap[cell->UUID].emplace(particle.UUID);
        particle.cellID = cell->UUID;
        cell->density += 1.0;
    }
    return true;
}

// note that the particles' cellID is NOT updated here
TransitionList Simulation::FindCellTransitions() const
{
    TransitionList transitions;
    for (const Fluid::Particle& particle: fluid.particles) {
        const auto& oldcell = diffusionField.cells.at(particle.cellID);
        if (oldcell.getGlobalBounds().contains(particle.getPosition())) continue;
        else {
            const auto& [x, y] = particle.getPosition();
            const unsigned int xi = x / SPATIAL_RESOLUTION;
            const unsigned int yi = y / SPATIAL_RESOLUTION;
            assert((xi <= DiffusionField::maxIX) && (yi <= DiffusionField::maxIY) && "out-of-bounds index");
            transitions.emplace_back(particle.UUID, particle.cellID, diffusionField.cellmatrix.at(xi).at(yi)->UUID);
        }
    }
    return transitions;
}


void Simulation::HandleTransitions(const TransitionList& transitions)
{
    DeltaMap_T deltamap{};
    for (const auto& trec: transitions)
    {
        auto& deltaN = deltamap[trec.oldCellID].negative;
        auto& deltaP = deltamap[trec.newcellID].positive;
        deltaN.particleIDs.emplace(trec.particleID);
        deltaP.particleIDs.emplace(trec.particleID);
        deltaN.density -= 1.0;
        deltaP.density += 1.0;
    }
    
    for (auto& [cellID, deltas]: deltamap)
    {
        // updating densities
        const float densityDelta = deltas.positive.density + deltas.negative.density;
        diffusionField.cells[cellID].density += densityDelta;
        
        // transferring momentum to cell and updating particle's cellID
        for (const int particleID: deltas.positive.particleIDs) 
        {
            Fluid::Particle& particle = fluid.particles.at(particleID);
            particle.cellID = cellID;
            const sf::Vector2f momentumDelta = particle.velocity * momentumTransfer;
            particle.velocity -= momentumDelta;
            diffusionField.cells[cellID].momentum += momentumDelta;
        }
        
        // updating particleMap
        NegativeMerge(particleMap[cellID], deltas.negative.particleIDs);
        particleMap[cellID].merge(deltas.positive.particleIDs);
        if (particleMap[cellID].empty()) {
            diffusionField.cells[cellID].momentum = {0.0, 0.0};
            particleMap.erase(cellID);
        }
    }
    return;
}


// assumes that density-updates were already performed on ALL cells
void Simulation::UpdateParticles()
{
    // only recalculate diffusionVec for occupied cells
    for (auto& [cellID, particleset]: particleMap) {
        DiffusionField::Cell& cell = diffusionField.cells.at(cellID);
        cell.diffusionVec = diffusionField.CalcDiffusionVec(cellID) * timestepRatio * fluid.fdensity;
        
        assert((particleset.size() > 0) && "empty particleset!");
        
        const sf::Vector2f momentumDistributed = cell.momentum * momentumDistribution * timestepRatio;
        const sf::Vector2f momentumPerParticle = momentumDistributed / float(particleset.size());
        cell.momentum -= momentumDistributed;
        
        // distributing momentum and applying diffusionVec
        for (unsigned int particleID: particleset) {
            Fluid::Particle& particle = fluid.particles.at(particleID);
            particle.velocity += cell.diffusionVec + momentumPerParticle;
        }
    }
    
    return;
}


void Simulation::Update()
{
    if (isPaused) { return; }
    if (hasGravity) { fluid.ApplyGravity(); }
    
    fluid.UpdatePositions();
    auto transitions = FindCellTransitions();
    HandleTransitions(transitions);
    UpdateParticles();
    
    return;
}
