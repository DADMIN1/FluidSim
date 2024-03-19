#ifndef FLUIDSIM_DIFFUSION_HPP_INCLUDED
#define FLUIDSIM_DIFFUSION_HPP_INCLUDED

#include <array>
#include <cassert>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/RectangleShape.hpp>

#include "Globals.hpp"


constexpr unsigned int DIFFUSION_RADIUS {3};  // number of neighboring grid-cells affected during density calculations (0 = only current cell is affected)
constexpr float DIFFUSION_SCALING {1.0/float(DIFFUSION_RADIUS+1)};  // diffusion-strength needs to decrease with distance, and must be scaled with radius

constexpr std::array<std::array<float, BOXHEIGHT/SPATIAL_RESOLUTION>, BOXWIDTH/SPATIAL_RESOLUTION> DensityGrid{};  // holds 'densities' for each cell
// TODO: move global definitions to another file
// TODO: replace DensityGrid with DiffusionField_T

consteval float DiffusionStrength(const unsigned int radial_distance) // radial-distance is number of cells away from particle
{
    // can't use static_assert (until std23?) because radial_distance doesn't count as constexpr, apparently.
    assert((radial_distance <= DIFFUSION_RADIUS) && "DiffusionStrength called for cell beyond radius of effect");
    // TODO: generate static lookup-arrays for diffusion strength based on radius and resolution
    // TODO: scaling function that always totals to 1.0 regardless of radius
    return 1.0 - DIFFUSION_SCALING*(radial_distance);
}

// TODO: implement as a template function (wouldn't that result in a million instantiations?)
//const float* CellLookup(const float positionX, const float positionY);  // pointer into DensityGrid at the correct cell for a given (particle) position
inline const float* CellLookup(const float positionX, const float positionY)
{ return &DensityGrid[int(positionX/SPATIAL_RESOLUTION)][int(positionY/SPATIAL_RESOLUTION)]; }
// Doesn't this fail at the max height/width? Need to modify Fluid.Update

/* consteval int CalcBaseNCount(int radial_distance) {
    return radial_distance;
} */
// std::size_t so they can be used for array sizing
constexpr std::size_t NeighborCountsAtDist[] = {0, 4, 8, 16, 20};  // number of neighbors at a given radial distance
constexpr int BaseNeighborCounts[] = {0, 4, 12, 28, 48};  // number of neighbors (cumulative) for a given radial distance
int CalcBaseNCount(int radial_distance);  // returns the relative coords of neighbors at radial_distance

// X/Y indexes are parameters because we need to specialize for cells within DIFFUSION_RADIUS of the edge
template <int RD, int X, int Y>  // radial_distance, X/Y index into DensityGrid
constexpr int GetNeighborCount() { return BaseNeighborCounts[RD]; }  // returns number of neighbor-cells; important for sizing arrays (in GetNeighborCells)
// normally the result is independent of position, but if either coordinate is within radial-distance of the edge, we need to prevent out-of-bounds lookups

template <int RD, int X, int Y>  // radial_distance, X/Y index into DensityGrid
class NeighborCells {
    static constexpr int Ncount = GetNeighborCount<RD, X, Y>();
    constexpr std::array<float*, Ncount> GetNeighborCells();
};


using Coordlist = std::vector<std::pair<int, int>>;
using DoubleCoord = std::pair<std::pair<int, int>, std::pair<int, int>>;


class DiffusionField_T
{
    sf::RenderTexture cellgrid_texture;
    public:
    //friend class Cell;
    friend class Fluid;
    friend struct Mouse_T;
    // TODO: make a version with const(expr) indecies and IDs
    class Cell: public sf::RectangleShape {
        static constexpr float colorscaling {16.0}; // density required for all-white color;
        public:
        //static int counterX, counterY;
        //const int IX, IY;
        unsigned int IX, IY, UUID;  // UUID is the array index of Cell
        float density{0.0};
        
        // TODO: Cells should be larger than 1 pixel??
        Cell()
        : sf::RectangleShape(sf::Vector2f{SPATIAL_RESOLUTION, SPATIAL_RESOLUTION}),
        IX{0}, IY{0}, UUID{0} 
        { }
        
        Cell(const unsigned int X, const unsigned int Y, const unsigned int UUID) 
        : sf::RectangleShape(sf::Vector2f{SPATIAL_RESOLUTION, SPATIAL_RESOLUTION}),
        IX{X}, IY{Y}, UUID{UUID}
        {
            this->setFillColor(sf::Color::Transparent);
            this->setOutlineColor(sf::Color(0xFFFFFF80));  // half-transparent white
            this->setOutlineThickness(1);
            this->setPosition(sf::Vector2f{float(X*SPATIAL_RESOLUTION), float(Y*SPATIAL_RESOLUTION)});
        }
        
        void UpdateColor() {
            sf::Uint8 alpha = (density >= 127/colorscaling ? 255 : colorscaling*density + 127); // avoiding overflows
            sf::Uint8 red = (density >= 255/colorscaling ? 255 : colorscaling*density); // avoiding overflows
            this->setFillColor(sf::Color(red, 0, 0, alpha));
        }
    };
    static constexpr unsigned int maxIX = (BOXWIDTH/SPATIAL_RESOLUTION - 1);
    static constexpr unsigned int maxIY = (BOXHEIGHT/SPATIAL_RESOLUTION - 1);
    // these are added with signed-ints in GetCellNeighbors (because the result might be negative); hence the assertion.
    static_assert((maxIX < __INT_MAX__) && (maxIY < __INT_MAX__) && "max-indecies will overflow");
    
    using CellMatrix = std::array<std::array<Cell*, BOXHEIGHT/SPATIAL_RESOLUTION>, BOXWIDTH/SPATIAL_RESOLUTION>;
    //using CellArray = std::array<Cell, ((BOXHEIGHT/SPATIAL_RESOLUTION)*(BOXWIDTH/SPATIAL_RESOLUTION))>; // crashes
    using CellArray = std::vector<Cell>; // doesn't crash
    CellArray cells; // TODO: figure out how to do this with an array without crashing
    CellMatrix cellmatrix;
    
    //std::array<Cells, 2> cells;  // two buffers; a read-only 'current' state, and a working buffer
    //Cells* const state {&cells[0]};
    //Cells* state_working {&cells[1]};
    
    const sf::Vector2f CalcDiffusionVec(std::size_t UUID);
    const std::vector<Cell*> GetCellNeighbors(const std::size_t UUID);
    const std::vector<DoubleCoord> GetAdjacentPlus(const std::size_t UUID); // returns pairs of absolute and relative coords
    
    bool Initialize()  // returns success/fail
    {
        if (!cellgrid_texture.create(BOXWIDTH, BOXHEIGHT))
            return false;
        
        cells.reserve((BOXHEIGHT/SPATIAL_RESOLUTION)*(BOXWIDTH/SPATIAL_RESOLUTION));
        
        unsigned int ID = 0;
        for (unsigned int c{0}; c < (BOXHEIGHT/SPATIAL_RESOLUTION); ++c) {
            for (unsigned int r{0}; r < (BOXWIDTH/SPATIAL_RESOLUTION); ++r) {
                //cells[ID] = Cell{c, r, ID};
                Cell& newcell = cells.emplace_back(c, r, ID);
                cellmatrix[c][r] = &newcell;
                ++ID;
            }
        }
        return true;
    }
    
    /* DiffusionField_T()
    {
        Initialize();
        cellgrid_texture.create(BOXWIDTH, BOXHEIGHT);
    } */
    
    sf::Sprite Draw() 
    {
        cellgrid_texture.clear(sf::Color::Transparent);
        for (auto& cell : cells) {
            cell.UpdateColor();
            cellgrid_texture.draw(cell);
        }
        cellgrid_texture.display();
        return sf::Sprite(cellgrid_texture.getTexture());
    }
};

using CellRef_T = std::vector<DiffusionField_T::Cell*>;

//template <std::size_t RD, std::size_t X, std::size_t Y>  // radial_distance, X/Y index into DensityGrid
//constexpr void UpdateDiffusion(RD, X, Y);  // updates the counts for cells in diffusion delta-map (for neighbors of radial_distance RD)

// alternatively, we could just return nullptr for 

// these should be member functions
//void UpdateDensities();
//void ModifyVelocities();

/* 
// Density updates should be done in the following steps:
// 1. Update the 'CurrentCell' for each particle. If it's unchanged, break
// 2. Record the change (+newcell, -oldcell) into an intermediate table; this should allow you to skip net-zero calculations
//  (if particle A moves to a new-cell, and particle B moves into A's prev-cell, the densities for A's prev-cell AND it's (diffusion) neighbors do not need to be recalculated)
//  (the subtraction from B's prev-cell and addition to A's new-cell still need to be calculated)
// The intermediate-table will contain the net-difference in number of particles per grid-cell since last frame
// 3. From this table, you can create another intermediate-table for the delta to neighbor-cells' density
// Getting the neighbor-cells should be consteval. It's probably not worth doing cancellation/masking like we did in the first step?
//  (actually, we can combine the subtractions from prev-cell and additions to new-cell by combining them;
//   the delta should be the sum of: -1*DiffusionStrength(dist_from_oldcell) + DiffusionStrength(dist_from_newcell) 
//   the formula requires DiffusionStrength to return 0 if the distance is greater than the max diffusion radius
//   this might only be efficient when particles move within the diffusion radius per frame? (maybe not, we'd need to do the subtractions/additions anyway)
//   we might even be able to precalculate the masks/deltas for each movement, but that would probably require an absurd number of template instantiations (and limit velocities to 1 cell/frame)
//   perhaps you could create a formula that generates the mask/deltas from a single calculation on the distance between oldcell and newcell??)
// I think we might want seperate intermediate subtraction/addition maps for Diffusion-deltas?
// The Diffusion neighbor-cell MASK (per-particle) holds the delta in prev/next radial-distances;
//   if a cell goes from a radial distance of 3->1, the density of that cell will change by (2*Diffusion_Scaling); equivalent to simply adding DiffusionStrength at distance=2
//   another example; previous-cell's DiffusionStrength will drop from 1.0 to DiffusionStrength(dist_to_prevcell); literally equivalent to 1.0 - DIFFUSION_SCALING*(dist_to_prevcell)
//   algebraically, the prev-density of a cell equals: 1.0-DIFFUSION_SCALING*(prev_radial_distance), and the new density equals: 1.0-DIFFUSION_SCALING*(radial_distance)
//   therefore, the difference equals: 
//      (1.0 - DIFFUSION_SCALING*(prev_radial_distance)) - (1.0 - DIFFUSION_SCALING*(radial_distance))
//      (-DIFFUSION_SCALING*(prev_radial_distance)) - (-DIFFUSION_SCALING*(radial_distance))
//      DIFFUSION_SCALING*(radial_distance) - DIFFUSION_SCALING*(prev_radial_distance)
//      DIFFUSION_SCALING * (radial_distance - prev_radial_distance)  // should be negative, actually?
// The intermediate delta-map for Diffusion holds the combined deltas for each radial-distance from all particles;
//  With linear DIFFUSION_SCALING / DiffusionStrength, I think you could just sum all the deltas, then multiply by DIFFUSION_SCALING?
//  But nonlinear equations will require you to track the counts for each radial-distance seperately, at which point you may as well just directly calculate / modify DiffusionStrength directly
// Maybe you could create an intermediate map for each radial-distance? That might actually work well if NeighborLookup is called for each radial-distance
 */






#endif
