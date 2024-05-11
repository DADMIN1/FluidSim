#include "Simulation.hpp"

#include "Threading.hpp"

#include <iostream>
#include <tuple>
#include <cassert>


#ifdef PMEMPTYCOUNTER
long long pmemptycounter{0};  // counts how many times particleMap was empty
#endif

// helper function because sets don't have an inverse-merge
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
        assert((xi <= Cell::maxIX) && (yi <= Cell::maxIY) && "out-of-bounds index");
        Cell* cell = diffusionField.cellmatrix.at(xi).at(yi);
        
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
            assert((xi <= Cell::maxIX) && (yi <= Cell::maxIY) && "out-of-bounds index");
            transitions.emplace_back(particle.UUID, particle.cellID, diffusionField.cellmatrix.at(xi).at(yi)->UUID);
        }
    }
    return transitions;
}

// multithreaded version
DeltaMap Simulation::FindCellTransitions(const auto& particles_slice) const
{
    DeltaMap dmap;
    
    for (auto iter{particles_slice.first}; iter < particles_slice.second; ++iter) {
        const Fluid::Particle& particle = *iter;
        const Cell& oldcell = diffusionField.cells.at(particle.cellID);
        if (oldcell.getGlobalBounds().contains(particle.getPosition())) continue;
        else {
            const auto& [x, y] = particle.getPosition();
            const unsigned int xi = x / SPATIAL_RESOLUTION;
            const unsigned int yi = y / SPATIAL_RESOLUTION;
            assert((xi <= Cell::maxIX) && (yi <= Cell::maxIY) && "out-of-bounds index");
            const Cell& newcell = *diffusionField.cellmatrix.at(xi).at(yi);
            
            dmap.transitionlist.emplace_back(particle.UUID, oldcell.UUID, newcell.UUID);
            CellDelta_T& oldcell_delta = dmap.cellmap[oldcell.UUID];
            CellDelta_T& newcell_delta = dmap.cellmap[newcell.UUID];
            oldcell_delta.particlesRemoved.emplace(particle.UUID);
            newcell_delta.particlesAdded.emplace(particle.UUID);
            oldcell_delta.density -= 1.0f;
            newcell_delta.density += 1.0f;
            //oldcell_delta.velocities -= 1.0f;
            newcell_delta.velocities += particle.velocity * momentumTransfer;
        }
    }
    return dmap;
}


// note: deltamap gets eaten by the '.merge' call
void Simulation::HandleTransitions(DeltaMap&& deltamap)
{
    std::lock_guard<std::mutex> pmGuard(write_mutex);
    
    /* for (const Transition_T& transition : deltamap.transitionlist)
    {
        
    } */
    
    for (auto&& [cellID, delta]: deltamap.cellmap)
    {
        Cell& cell = diffusionField.cells[cellID];
        // delta.velocities has already been scaled by momentumTransfer
        
        // updating densities
        diffusionField.cells[cellID].density += delta.density;
        
        // this should probably be reverted
        // float smoothingdivisor = 1.0f + float(delta.particlesAdded.size() - delta.particlesRemoved.size());
        float smoothingdivisor = float(delta.particlesAdded.size() + delta.particlesRemoved.size());
        const sf::Vector2f momentumSmoothing = { delta.velocities*momentumTransfer / smoothingdivisor };
        if (fluid.isTurbulent) cell.momentum += momentumSmoothing;
        
        // transferring momentum from new particles to cell
        for (const int particleID: delta.particlesAdded)
        {
            Fluid::Particle& particle = fluid.particles.at(particleID);
            if (fluid.isTurbulent) particle.velocity += momentumSmoothing;
            const sf::Vector2f momentumDelta = particle.velocity * momentumTransfer;
            particle.velocity -= momentumDelta;
            cell.momentum += momentumDelta;
            //cell.momentum -= momentumSmoothing;
            
            //Cell& oldCell = diffusionField.cells[particle.cellID];
            particle.cellID = cellID;
        }
        
        if (fluid.isTurbulent) {
            // applying momentumSmoothing to leaving particles
            for (const int particleID: delta.particlesRemoved)
            {
                Fluid::Particle& particle = fluid.particles.at(particleID);
                particle.velocity += momentumSmoothing;
                //cell.momentum -= momentumSmoothing;
                //Cell& newCell = diffusionField.cells[cellID];
            }
        }
        
        // transferring momentum to cell and updating particle's cellID
        /* for (const int particleID: delta.particlesAdded) 
        {
            Fluid::Particle& particle = fluid.particles.at(particleID);
            Cell& oldCell = diffusionField.cells[particle.cellID];
            particle.cellID = cellID;
            const sf::Vector2f momentumDelta = particle.velocity * momentumTransfer;
            particle.velocity -= momentumDelta;
            const sf::Vector2f momentumSmoothing = (oldCell.momentum-diffusionField.cells[cellID].momentum)*(momentumTransfer/2.0f);
            oldCell.momentum -= momentumSmoothing;
            diffusionField.cells[cellID].momentum += momentumDelta + momentumSmoothing;
            // TODO: rewrite momentum calculations to account for density and viscosity
            // and momentum should be applied/calculated every frame?
        } */
        
        // updating particleMap
        NegativeMerge(particleMap[cellID], delta.particlesRemoved);
        particleMap[cellID].merge(delta.particlesAdded);
        
        // turns out this happens a lot
        // TODO: defer this erase or something
        if (particleMap[cellID].empty()) {
            #ifdef PMEMPTYCOUNTER
            pmemptycounter += 1;
            #endif
            diffusionField.cells[cellID].momentum = {0.0, 0.0};
            particleMap.erase(cellID);
        }
    }
    
    return;
}


IDset_T Simulation::BuildAdjacentSet(const std::size_t cellID, const IDset_T& excluded)
{
    std::unordered_set<unsigned int> localParticles{};
    const std::vector<Cell*> adjacentCells = diffusionField.GetCellNeighbors(cellID);
    
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
    auto segmented_particlemap = DivideContainer(particleMap);
    auto lambda = [this](auto segment) 
    {
        // need to prevent redundant combinations when building the particleset for LocalDiffusion
        std::unordered_set<unsigned int> excludedIDs{};
        
        // only recalculate diffusionVec for occupied cells
        for (auto iter{segment.first}; iter != segment.second; ++iter)
        {
            auto& [cellID, particleset] = *iter;
            // VERY IMPORTANT: particleset should NOT be a reference if you merge with it
            //assert((particleset.size() > 0) && "empty particleset!");
            if (particleset.empty()) { continue; }
            
            Cell& cell = diffusionField.cells.at(cellID);
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
    };
    
    // TODO: fix the particle-banding caused by multithreading (excludedIDs is thread-local)
    
    std::array<std::future<void>, THREAD_COUNT> threads;
    assert((segmented_particlemap.size() == threads.size()) && "mismatched sizes between particlemap and threads");
    for (std::size_t index{0}; index < threads.size(); ++index) {
        threads[index] = std::async(std::launch::async, lambda, segmented_particlemap[index]);
    }
    for (auto& handle: threads) { handle.wait(); }
    
    return;
}


void Simulation::Update()
{
    if (isPaused) { return; }
    
    /* if (hasGravity) { fluid.ApplyGravity(); }
    fluid.UpdatePositions();  */
    
    std::array<std::future<DeltaMap>, THREAD_COUNT> threads;
    auto particles_slices = DivideContainer(fluid.particles);
    for (std::size_t index{0}; index < threads.size(); ++index) {
        auto slice = particles_slices[index];
        auto lambda = [this, slice](){ 
            if (hasGravity) 
            { fluid.ApplyGravity (slice.first, slice.second); }
            fluid.UpdatePositions(slice.first, slice.second);
            return FindCellTransitions(slice);
        };
        threads[index] = std::async(std::launch::async, lambda);
    };
    
    DeltaMap transitions{};
    for (auto& handle: threads) {
        transitions.Combine(handle.get()); // extracts/moves elements with new keys
    }
    
    HandleTransitions(std::move(transitions));
    
    // single-threaded versions
    //auto transitions = FindCellTransitions();
    //HandleTransitions(transitions);
    
    // TODO: rewrite HandleTransitions to handle multithreading better
    
    // multithreaded (seems to be slower)
    /* std::array<std::future<void>, THREAD_COUNT> threads;
    auto particles_slices = DivideContainer(fluid.particles);
    for (std::size_t index{0}; index < threads.size(); ++index) {
        auto lambda = [this](auto slice) { 
            auto transitions = FindCellTransitions(slice);
            HandleTransitions(transitions);
        };
        threads[index] = std::async(std::launch::async, lambda, particles_slices[index]);
    };
    for (auto& handle: threads) { handle.wait(); } */
    
    UpdateParticles();
    //fluid.UpdatePositions();  // seems like doing this last might give slightly better performance?
    
    return;
}
