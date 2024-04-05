#include "Simulation.hpp"

#include <iostream>
#include <tuple>


std::size_t NegativeMerge(std::unordered_set<unsigned int>& source, const std::unordered_set<unsigned int>& toRemove)
{
    std::size_t numErased{0};
    for (unsigned int thing: toRemove) {
        source.erase(thing);
        // if (source.erase(thing)) { ++numErased; }
    }
    return numErased;
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
            // TODO: rewrite momentum calculations to account for density and viscosity
            // and momentum should be applied/calculated every frame?
        }
        
        // fluid.ApplySpeedcap(diffusionField.cells[cellID].momentum);
        // fluid.ApplyViscosity(diffusionField.cells[cellID].momentum);
        
        // updating particleMap
        NegativeMerge(particleMap[cellID], deltas.negative.particleIDs);
        particleMap[cellID].merge(deltas.positive.particleIDs);
        
        // TODO: I don't think the maps should end up with empty sets ever? figure out why
        if (particleMap[cellID].empty()) {
            diffusionField.cells[cellID].momentum = {0.0, 0.0};
            particleMap.erase(cellID);
        }
    }
    return;
}


IDset_T Simulation::BuildAdjacentSet(const std::size_t cellID, const IDset_T& excluded)
{
    std::unordered_set<unsigned int> localParticles{};
    const CellPtrArray adjacentCells = diffusionField.GetCellNeighbors(cellID);
    
    for (const auto* const cellptr: adjacentCells)
    {
        if (particleMap.contains(cellptr->UUID) && !excluded.contains(cellptr->UUID))
        {
            std::unordered_set<unsigned int> pmapCopy{particleMap[cellptr->UUID]};
            
            #define BUILDADJACENTSET_SIZECHECKS false
            #if BUILDADJACENTSET_SIZECHECKS
            if (particleMap[cellptr->UUID].size() == 0) { std::cerr << "empty set in particleMap!\n"; continue; }
            std::size_t originalSize = pmapCopy.size();
            std::size_t originalSizeLP = localParticles.size();
            #endif
            
            //localParticles.merge(std::move(pmapCopy)); //doesn't seem to make a difference
            localParticles.merge(pmapCopy);
            
            #if BUILDADJACENTSET_SIZECHECKS
            std::size_t postmergeSize = pmapCopy.size();
            std::size_t postmergeSizeLP = localParticles.size();
            std::size_t diff = originalSize - postmergeSize;
            std::size_t diffLP = postmergeSizeLP - originalSizeLP;
            if (diff != diffLP) { 
                std::cerr << "mismatched merge sizes!\n";
                std::cerr << "pmapCopy: "       << originalSize   << " -> " << postmergeSize   << " = " << diff   << '\n';
                std::cerr << "localParticles: " << originalSizeLP << " -> " << postmergeSizeLP << " = " << diffLP << '\n';
            }
            
            assert((particleMap[cellptr->UUID].size() == originalSize) && "pmap did not actually copy");
            #endif
        }
    }
    
    return localParticles;
}


// Diffusion between particles within a single cell (restricted because only the origin will excluded) 
// otherwise, there will be many duplicate calculations between other cells, and everything will explode.
void Simulation::LocalDiffusion(const IDset_T& particleset)
{
    std::vector<sf::Vector2f> localForces;
    localForces.resize(particleset.size(), {0,0});
    
    // calculating localForce between all particles in the cell
    // nested loops avoid recalculating the force between every particle twice (by storing the negative)
    std::vector<sf::Vector2f>::iterator iterVecTop{localForces.begin()};
    for (auto iterTop{particleset.begin()}; iterTop != particleset.end(); ++iterTop)
    {
        Fluid::Particle& particleTop = fluid.particles.at(*iterTop);
        std::vector<sf::Vector2f>::iterator iterVecBottom{iterVecTop};
        ++iterVecBottom;
        
        // notice the iteration in the loop condition (it can't be initialized with 'iterTop+1')
        for (auto iterBottom{iterTop}; ++iterBottom != particleset.end();) {
            const Fluid::Particle& particleBottom = fluid.particles.at(*iterBottom);
            const sf::Vector2f localforce = fluid.CalcLocalForce(particleTop, particleBottom);
            
            *iterVecTop    += localforce;
            *iterVecBottom -= localforce; // the other particle experiences forces in the opposite direction
            
            ++iterVecBottom;
        }
        
        particleTop.velocity += *iterVecTop;
        ++iterVecTop;
    }
    
    return;
}

// applies diffusion across cells. 
void Simulation::NonLocalDiffusion(const IDset_T& originset, const IDset_T& adjacentset)
{
    if (adjacentset.empty()) { return; }
    for (const auto UUID : originset)
    {
        Fluid::Particle& particle = fluid.particles.at(UUID);
        for (const auto adjacentUUID: adjacentset)
        {
            Fluid::Particle& adjacentParticle = fluid.particles.at(adjacentUUID);
            const sf::Vector2f localforce = fluid.CalcLocalForce(particle, adjacentParticle);
            
            particle.velocity += localforce;
            adjacentParticle.velocity -= localforce; // the other particle experiences forces in the opposite direction
            // not redundant because the originset will be excluded from the adjacentset later.
        }
    }
    
    return;
}


// assumes that density-updates were already performed on ALL cells (and momentum-calculations)
void Simulation::UpdateParticles()
{
    // need to prevent redundant combinations when building the particleset for LocalDiffusion
    std::unordered_set<unsigned int> excludedIDs{};
    
    // only recalculate diffusionVec for occupied cells
    for (auto& [cellID, particleset]: particleMap)
    { // VERY IMPORTANT: particleset should NOT be a reference if you merge with it
        assert((particleset.size() > 0) && "empty particleset!");
        
        DiffusionField::Cell& cell = diffusionField.cells.at(cellID);
        cell.diffusionVec = diffusionField.CalcDiffusionVec(cellID) * timestepRatio * fluid.fdensity;
        // TODO: repurpose diffusionVec to redistribute/smooth momentum between cells
        
        // fluid.ApplySpeedcap(cell.momentum);
        const sf::Vector2f momentumDistributed = cell.momentum * momentumDistribution * timestepRatio;
        const sf::Vector2f momentumPerParticle = momentumDistributed / float(particleset.size());
        cell.momentum -= momentumDistributed;
        
        // distributing momentum and applying diffusionVec
        for (unsigned int particleID: particleset) {
            Fluid::Particle& particle = fluid.particles.at(particleID);
            particle.velocity += cell.diffusionVec + momentumPerParticle;
        }
        // unfortunately, we have to handle diffusionVec and momentum in a seperate loop;
        // because they're only meant to apply to the current cell's particles.
        // that also means it must be done BEFORE merging the particlesets
        
        // TODO: split the cell-related updates into a seperate function?
        
        const std::size_t originalsize = particleset.size();
        IDset_T nonlocalParticles = BuildAdjacentSet(cellID, excludedIDs);
        //particleset.merge(nonlocalParticles);  // the other merge order might be more effecient?
        // VERY important that particleset isn't a reference here (if you merge); if it is, then every cell will end up-
        // - holding duplicates of all the particles from each cell in it's diffusion-radius.
        // then each cell will propagate their duplicates on every frame - segfault almost immediately.
        
        assert((particleMap[cellID].size() == originalsize) && "set in particleMap should not change size!!!!");
        LocalDiffusion(particleset);
        NonLocalDiffusion(particleset, nonlocalParticles);
        excludedIDs.emplace(cellID);
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
    //fluid.UpdatePositions();  // seems like doing this last might give slightly better performance?
    
    return;
}
