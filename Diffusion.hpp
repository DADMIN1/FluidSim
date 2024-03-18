#ifndef FLUIDSIM_DIFFUSION_HPP_INCLUDED
#define FLUIDSIM_DIFFUSION_HPP_INCLUDED

#include <array>
#include <cassert>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/RectangleShape.hpp>

#include "Globals.hpp"


constexpr unsigned int DIFFUSION_RADIUS {1};  // number of neighboring grid-cells affected during density calculations (0 = only current cell is affected)
constexpr float DIFFUSION_SCALING {1.0/float(DIFFUSION_RADIUS+1)};  // diffusion-strength needs to decrease with distance, and must be scaled with radius

constexpr unsigned int SPATIAL_RESOLUTION {2};  // subdivisions per unit for calculating diffusion/collision
constexpr std::array<std::array<float, SPATIAL_RESOLUTION*BOXHEIGHT>, SPATIAL_RESOLUTION*BOXWIDTH> DensityGrid{};  // holds 'densities' for each cell
// TODO: move global definitions to another file


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
{ return &DensityGrid[int(positionX)*SPATIAL_RESOLUTION][int(positionY)*SPATIAL_RESOLUTION]; }

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


class DiffusionField_T
{
    sf::RenderTexture cellgrid_texture;
    public:
    //friend class Cell;
    friend class Fluid;
    // TODO: make a version with const(expr) indecies and IDs
    class Cell: public sf::RectangleShape {
        static constexpr float colorscaling {5.0}; // density required for all-white color;
        public:
        //static int counterX, counterY;
        //const int IX, IY;
        unsigned int IX, IY, UUID;  // UUID is the array index of Cell
        float density{0.0};
        
        Cell()
        : sf::RectangleShape(sf::Vector2f{1/SPATIAL_RESOLUTION, 1/SPATIAL_RESOLUTION}),
        IX{0}, IY{0}, UUID{0} 
        { }
        
        Cell(const unsigned int X, const unsigned int Y, const unsigned int UUID) 
        : sf::RectangleShape(sf::Vector2f{1/SPATIAL_RESOLUTION, 1/SPATIAL_RESOLUTION}),
        IX{X}, IY{Y}, UUID{UUID}
        {
            this->setFillColor(sf::Color::Transparent);
            this->setOutlineColor(sf::Color::White);
            this->setPosition(sf::Vector2f{float(X*SPATIAL_RESOLUTION), float(Y*SPATIAL_RESOLUTION)});
        }
        
        void UpdateColor() {
            this->setFillColor(sf::Color((density*colorscaling), 0, 0, (colorscaling*density/2.0)));
        }
    };
    
    using CellColumn = std::array<const Cell*, SPATIAL_RESOLUTION*BOXHEIGHT>;
    using CellMatrix = std::array<CellColumn, SPATIAL_RESOLUTION*BOXWIDTH>;
    // crashes
    //using CellArray = std::array<Cell, ((SPATIAL_RESOLUTION*BOXHEIGHT)*(SPATIAL_RESOLUTION*BOXWIDTH))>;
    using CellArray = std::vector<Cell>; // doesn't crash
    //CellMatrix cellmatrix;
    CellArray cells;
    
    //std::array<Cells, 2> cells;  // two buffers; a read-only 'current' state, and a working buffer
    //Cells* const state {&cells[0]};
    //Cells* state_working {&cells[1]};
    
    bool Initialize()  // returns success/fail
    {
        if (!cellgrid_texture.create(BOXWIDTH, BOXHEIGHT))
            return false;
        
        cells.reserve((SPATIAL_RESOLUTION*BOXHEIGHT)*(SPATIAL_RESOLUTION*BOXWIDTH));
        
        unsigned int ID = 0;
        for (unsigned int c{0}; c < (SPATIAL_RESOLUTION*BOXHEIGHT); ++c) {
            for (unsigned int r{0}; r < (SPATIAL_RESOLUTION*BOXWIDTH); ++r) {
                cells.emplace_back(c, r, ID);
                /* cells[ID] = Cell{c, r, ID};
                cellmatrix[c][r] = &cells[ID]; */
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
    
    void Draw(sf::RenderWindow* drawtarget) 
    {
        return;
        cellgrid_texture.clear(sf::Color::Transparent);
        for (auto& cell : cells) {
            cell.density = 2.5;
            cell.UpdateColor();
            cellgrid_texture.draw(cell);
        }
        cellgrid_texture.display();
        sf::Sprite gridsprite (cellgrid_texture.getTexture());
        drawtarget->draw(gridsprite);
    }
};

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
