#ifndef FLUIDSIM_DIFFUSION_HPP_INCLUDED
#define FLUIDSIM_DIFFUSION_HPP_INCLUDED

#include <array>
#include <cassert>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/RectangleShape.hpp>

#include "Globals.hpp"


constexpr static std::array<std::array<float, BOXHEIGHT/SPATIAL_RESOLUTION>, BOXWIDTH/SPATIAL_RESOLUTION> DensityGrid{};  // holds 'densities' for each cell
// TODO: move global definitions to another file
// TODO: replace DensityGrid with DiffusionField

consteval float DiffusionStrength(const unsigned int radial_distance) // radial-distance is number of cells away from particle
{
    // can't use static_assert (until std23?) because radial_distance doesn't count as constexpr, apparently.
    assert((radial_distance <= DIFFUSION_RADIUS) && "DiffusionStrength called for cell beyond radius of effect");
    // TODO: generate static lookup-arrays for diffusion strength based on radius and resolution
    // TODO: scaling function that always totals to 1.0 regardless of radius (use trig functions)
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
constexpr static std::size_t NeighborCountsAtDist[] = {0, 4, 8, 16, 20};  // number of neighbors at a given radial distance
constexpr static int BaseNeighborCounts[] = {0, 4, 12, 28, 48};  // number of neighbors (cumulative) for a given radial distance
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


class DiffusionField
{
    sf::RenderTexture cellgrid_texture;
    
    public:
    friend class Mouse_T;
    friend class Simulation;
    
    // TODO: make a version with const(expr) indecies and IDs
    class Cell: public sf::RectangleShape {
        static constexpr float colorscaling {24.0}; // density required for all-white color;
        unsigned int IX, IY, UUID; // UUID is the array index of Cell
        sf::Vector2f diffusionVec{0.0, 0.0}; // force calculated from the density of nearby cells
        sf::Vector2f momentum{0.0, 0.0}; // stored velocity imparted by particles, distributed to local particles
        float density{0.0};
        
        public:
        friend class DiffusionField;
        friend class Simulation;
        friend class Mouse_T;
        
        Cell()
        : sf::RectangleShape(sf::Vector2f{SPATIAL_RESOLUTION, SPATIAL_RESOLUTION}),
        IX{0}, IY{0}, UUID{0} 
        { }
        
        Cell(const unsigned int X, const unsigned int Y, const unsigned int UUID) 
        : sf::RectangleShape(sf::Vector2f{SPATIAL_RESOLUTION, SPATIAL_RESOLUTION}),
        IX{X}, IY{Y}, UUID{UUID}
        {
            setFillColor(sf::Color::Transparent);
            setOutlineColor(sf::Color(0xFFFFFF80));  // half-transparent white
            setOutlineThickness(1);
            setPosition(sf::Vector2f{float(X*SPATIAL_RESOLUTION), float(Y*SPATIAL_RESOLUTION)});
        }
        
        void UpdateColor() {
            if (density < 0) {  // painting negative-density areas red/magenta
                sf::Uint8 alpha = (std::abs(density) >= 127/colorscaling ? 255 : colorscaling*std::abs(density) + 127);
                sf::Uint8 colorchannel = (std::abs(density) >= colorscaling ? 255 : (127/colorscaling)*std::abs(density) + 127);
                setFillColor(sf::Color(colorchannel, 0, colorchannel/2, alpha/1.5));
                return; 
            }
            sf::Uint8 alpha = (density >= 127/colorscaling ? 255 : colorscaling*density + 127); // avoiding overflows
            sf::Uint8 colorchannel = (density >= colorscaling ? 255 : (255/colorscaling)*density); // avoiding overflows
            setFillColor(sf::Color(colorchannel, colorchannel, colorchannel, alpha));  // white
        }
    };
    
    static constexpr unsigned int maxIX = (BOXWIDTH/SPATIAL_RESOLUTION - maxindexAdjX);
    static constexpr unsigned int maxIY = (BOXHEIGHT/SPATIAL_RESOLUTION - maxindexAdjY);
    static constexpr unsigned int maxSizeX = maxIX+1;
    static constexpr unsigned int maxSizeY = maxIY+1;

    // these are added with signed-ints in GetCellNeighbors (because the result might be negative); hence the assertion.
    static_assert((maxIX < __INT_MAX__) && (maxIY < __INT_MAX__), "max-indecies will overflow");
    
    using CellMatrix = std::array<std::array<Cell*, maxSizeY>, maxSizeX>;
    //using CellArray = std::array<Cell, ((maxSizeY)*(maxSizeX))>; // crashes
    using CellArray = std::vector<Cell>; // doesn't crash
    CellArray cells; // TODO: figure out how to do this with an array without crashing
    CellMatrix cellmatrix;
    
    // finds cells at a single distance
    const std::vector<Cell*> GetCellNeighbors(const std::size_t UUID) const;
    // finds cells at every distance up to (and including) current DIFFUSION_RADIUS
    const std::vector<Cell*> GetCellNeighbors(const std::size_t UUID, const unsigned int radialdist) const;
    const std::vector<DoubleCoord> GetAdjacentPlus(const std::size_t UUID) const; // returns pairs of absolute and relative coords
    const sf::Vector2f CalcDiffusionVec(std::size_t UUID) const;
    
    bool Initialize()  // returns success/fail
    {
        if (!cellgrid_texture.create(BOXWIDTH, BOXHEIGHT))
            return false;
        
        cells.reserve((maxSizeY)*(maxSizeX));
        
        unsigned int ID = 0;
        for (unsigned int c{0}; c < (maxSizeX); ++c) {
            for (unsigned int r{0}; r < (maxSizeY); ++r) {
                //cells[ID] = Cell{c, r, ID};
                Cell& newcell = cells.emplace_back(c, r, ID++);
                cellmatrix[c][r] = &newcell;
            }
        }
        return true;
    }
    
    void PrintAllCells() const;
    
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
    
    void ResetMomentum() {
        for (Cell& cell: cells) {
            cell.momentum = {0.0, 0.0};
        }
    }
};

using CellRef_T = std::vector<DiffusionField::Cell*>;

//template <std::size_t RD, std::size_t X, std::size_t Y>  // radial_distance, X/Y index into DensityGrid
//constexpr void UpdateDiffusion(RD, X, Y);  // updates the counts for cells in diffusion delta-map (for neighbors of radial_distance RD)


#endif
