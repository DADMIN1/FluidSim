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
            unsigned int xi = x / SPATIAL_RESOLUTION;
            unsigned int yi = y / SPATIAL_RESOLUTION;
            // TODO: actually fix this instead of workaround
            if (xi > Cell::maxIX) xi = Cell::maxIX;
            if (yi > Cell::maxIY) yi = Cell::maxIY;
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


// note: cellmap gets eaten by the '.merge' call
void Simulation::HandleTransitions(std::map<unsigned int, CellDelta_T>&& cellmap)
{
    std::lock_guard<std::mutex> pmGuard(write_mutex);
    constexpr float turbulence_offset = { 50.f / float(NUMROWS+NUMCOLUMNS)};
    const float rng = (turbulence_offset+normalizedRNG())*(turbulence_offset+normalizedRNG());
    
    // required minimum cell-density before momentum transfers become active
    constexpr float thresholdDensityMomentumTransfer {2.f};
    
    // 'auto&&' is definitely correct here; ~100 FPS difference (300->400)
    for (auto&& [cellID, delta]: cellmap)
    {
        Cell& cell = diffusionField.cells[cellID];
        // delta.velocities has already been scaled by momentumTransfer
        // updating densities
        cell.density += delta.density;
        
        if (cell.density < thresholdDensityMomentumTransfer) { // skip the momentum-related code if cell is too empty
            for (int particleID: delta.particlesAdded) {
                fluid.particles[particleID].cellID = cellID;
            }
        } else {
            // this should probably be reverted
            // float smoothingdivisor = 1.0f + float(delta.particlesAdded.size() - delta.particlesRemoved.size());
            float smoothingdivisor = float(delta.particlesAdded.size() + delta.particlesRemoved.size());
            sf::Vector2f momentumSmoothing = { delta.velocities*momentumTransfer / smoothingdivisor };
            if (fluid.isTurbulent) cell.momentum += momentumSmoothing * rng;
            
            // transferring momentum from new particles to cell
            for (const int particleID: delta.particlesAdded)
            {
                Fluid::Particle& particle = fluid.particles.at(particleID);
                if (fluid.isTurbulent) particle.velocity += momentumSmoothing * rng;
                const sf::Vector2f momentumDelta = particle.velocity * momentumTransfer;
                particle.velocity -= momentumDelta;
                cell.momentum += momentumDelta;
                //cell.momentum -= momentumSmoothing;
                
                //Cell& oldCell = diffusionField.cells[particle.cellID];
                particle.cellID = cellID;
            }
        }
        
        /* if (fluid.isTurbulent) {
            // applying momentumSmoothing to leaving particles
            for (const int particleID: delta.particlesRemoved)
            {
                Fluid::Particle& particle = fluid.particles.at(particleID);
                particle.velocity += momentumSmoothing * rng;
                //cell.momentum -= momentumSmoothing*momentumDistribution;
                //Cell& newCell = diffusionField.cells[cellID];
            }
        } */
        
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
        
        //if (fluid.isTurbulent) continue;
        if (particleMap[cellID].empty()) {
            #ifdef PMEMPTYCOUNTER
            // turns out this happens a lot
            pmemptycounter += 1;
            #endif
            
            // disabling this prevents 'zipping' behind the mouse, and gives a minor performance improvement
            //#define PRESERVE_EMPTYCELL_MOMENTUM
            #ifndef PRESERVE_EMPTYCELL_MOMENTUM
            diffusionField.cells[cellID].momentum = {0.0, 0.0};
            particleMap.erase(cellID);
            #else
            // only erase if it's empty and has negligable momentum
            // TODO: collect stats on this to find a good minimum
            constexpr float small_enough = 0.001;
            if ((abs(cell.momentum.x) < small_enough) 
             && (abs(cell.momentum.y) < small_enough)) {
                // TODO: defer this erase or something
                diffusionField.cells[cellID].momentum = {0.0, 0.0};
                particleMap.erase(cellID);
                continue;
             }
             
             // stored momentum would otherwise not decrease for empty cells
            cell.momentum -= cell.momentum*momentumDistribution;
            // we don't erase the cell because we want to keep decreasing the momentum
            #endif
        }
    }
    
    return;
}


IDset_T Simulation::BuildAdjacentSet(const std::size_t cellID /*, const IDset_T& excluded */)
{
    std::unordered_set<unsigned int> localParticles{};
    const std::vector<Cell*> adjacentCells = diffusionField.GetCellNeighbors(cellID);
    
    for (const auto* const cellptr: adjacentCells)
    {
        // This check causes gaps/banding between sections (along the divisions between threads)
            //if (particleMap.contains(cellptr->UUID) && !excluded.contains(cellptr->UUID))
        // Inverting the second check causes pillars to appear instead
            //if (excluded.contains(cellptr->UUID))
        // Simply not checking the excluded set seems to be correct? I don't understand why
            //if (particleMap.contains(cellptr->UUID))
            //if (particleMap.contains(cellptr->UUID) || excluded.contains(cellptr->UUID)) // also seems to work, but probably incorrect
        if (particleMap.contains(cellptr->UUID))
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
            const sf::Vector2f localforce = Fluid::CalcLocalForce(particleTop, particleBottom, fluid.fdensity);
            
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
            const sf::Vector2f localforce = Fluid::CalcLocalForce(particle, adjacentParticle, fluid.fdensity);
            
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
    // TODO: figure out how to share an 'excludedIDs' set between threads
    auto segmented_particlemap = DivideContainer(particleMap);
    auto lambda = [this](auto segment) 
    {
        // need to prevent redundant combinations when building the particleset for LocalDiffusion
        //std::unordered_set<unsigned int> excludedIDs{};
        
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
            IDset_T nonlocalParticles = BuildAdjacentSet(cellID /* , excludedIDs */);
            //particleset.merge(nonlocalParticles);  // the other merge order might be more effecient?
            // VERY important that particleset isn't a reference here (if you merge); if it is, then every cell will end up-
            // - holding duplicates of all the particles from each cell in it's diffusion-radius.
            // then each cell will propagate their duplicates on every frame - segfault almost immediately.
            
            assert((particleMap[cellID].size() == originalsize) && "set in particleMap should not change size!!!!");
            LocalDiffusion(particleset);
            NonLocalDiffusion(particleset, nonlocalParticles);
            //excludedIDs.emplace(cellID);
        }
    };
    
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
    
    // TODO: figure out how to merge changes from multiple copies
    //static std::array<std::vector<Fluid::Particle>, THREAD_COUNT> particles_copy;
    
    //Fluid::CertainConstants fluidConstants (hasGravity, hasXGravity, fluid.gravity, fluid.xgravity, fluid.viscosity, fluid.bounceDampening, timestepRatio);
    // sf::Vector2f, float, float
    const auto&& [gravityForces, viscosityMultiplier, bounceDampeningFactor] = Fluid::CertainConstants (
        hasGravity, hasXGravity, fluid.gravity, fluid.xgravity, fluid.viscosity, fluid.bounceDampening, timestepRatio
    );
    
    std::array<std::future<void>, THREAD_COUNT> threads;
    for (std::size_t index{0}; auto&& slice: DivideContainer(fluid.particles)) {
        threads[index] = std::async(std::launch::async, 
        [this, gravityForces, viscosityMultiplier, bounceDampeningFactor] (auto&& sliced) { 
            //fluid.UpdatePositions(sliced.first, sliced.second, hasGravity, hasXGravity);
            Fluid::UpdatePositions(sliced.first, sliced.second, gravityForces, viscosityMultiplier, bounceDampeningFactor);
            HandleTransitions(FindCellTransitions(sliced).cellmap);
            //UpdateParticles(sliced); // TODO: rewrite this to take a slice
        }, slice);
        ++index;
    };
    
    bool isComplete{false};
    do { isComplete = true;
        for (auto& future: threads) {
            if(!future.valid()) isComplete = false;
            //future.wait_for(std::chrono::duration);
            else future.get();
        }
    } while (!isComplete);
    
    UpdateParticles();
    
    return;
}
