#ifndef FLUIDSIM_DIFFUSION_HPP_INCLUDED
#define FLUIDSIM_DIFFUSION_HPP_INCLUDED

#include <array>
#include <cassert>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/RectangleShape.hpp>

#include "Globals.hpp"


// std::size_t so they can be used for array sizing
constexpr std::size_t NeighborCountsAtDist[] = {0, 4, 8, 12, 16, 20};  // number of neighbors at a given radial distance

template <int RD>  // radial_distance
constexpr int BaseNeighborCount() { return NeighborCountsAtDist[RD] + BaseNeighborCount<RD-1> (); }
// returns number of neighbor-cells; important for sizing arrays (in GetNeighborCells)
// normally the result is independent of position, but if either coordinate is within radial-distance of the edge, we need to prevent out-of-bounds lookups

template<> constexpr int BaseNeighborCount<0>() { return 0; }

static_assert(BaseNeighborCount<1>() == 4);
static_assert(BaseNeighborCount<2>() == 12);
static_assert(BaseNeighborCount<3>() == 24);

constexpr int BaseNeighborCounts[] = {0, 4, 12, 24, 40, 60};  // number of neighbors (cumulative) for a given radial distance

// X/Y indexes are parameters because we need to specialize for cells within DIFFUSION_RADIUS of the edge
/* template<int RD, int X, int Y> // radial_distance, X/Y index into CellMatrix
constexpr int ComplexNeighborCount() { return BaseNeighborCount<RD>(); } */

int CalcBaseNCount(int radial_distance);  // returns the relative coords of neighbors at radial_distance


template <int RD, int X, int Y>  // radial_distance, X/Y index into DensityGrid
class NeighborCells {
    static constexpr int Ncount = BaseNeighborCount<RD, X, Y>();
    constexpr std::array<float*, Ncount> GetNeighborCells();
};

// DIFFUSIONSCALING //

// TODO: scaling function that always totals to 1.0 regardless of radius (use trig functions)
constexpr float GetNextFloat(const float x, const int orthodist)
{
    //float ratio = float(orthodist) / float(1.0 + DIFFUSION_RADIUS - orthodist);
    return x - (1.0 / float(NeighborCountsAtDist[orthodist]));
    //return x * (float(DIFFUSION_RADIUS - orthodist) / float(DIFFUSION_RADIUS));
}

consteval std::array<float, radialdist_limit+1> CreateDiffusionScale()
{
    float x = 1.0f;
    int orthodist = 0;
    
    std::array<float, radialdist_limit+1> diffusionScaling{
        (1.0f),
        (x = GetNextFloat(x, ++orthodist), x),
        (x = GetNextFloat(x, ++orthodist), x),
        (x = GetNextFloat(x, ++orthodist), x),
        (x = GetNextFloat(x, ++orthodist), x),
        (x = GetNextFloat(x, ++orthodist), x),
    };
    return diffusionScaling;
}

constexpr auto DIFFUSIONSCALING = CreateDiffusionScale();

static_assert(DIFFUSIONSCALING[0] == 1.0);
static_assert((DIFFUSIONSCALING[1] > 0.0) && (DIFFUSIONSCALING[1] < 1.0));
static_assert((DIFFUSIONSCALING[2] > 0.0) && (DIFFUSIONSCALING[2] < 1.0));


// END DIFFUSIONSCALING //


using CoordlistRel = std::vector<std::pair<int, int>>;
using CoordlistAbs = std::vector<std::pair<int, int>>;
using DoubleCoord = std::pair<std::pair<int, int>, std::pair<int, int>>;


class DiffusionField
{
    sf::RenderTexture cellgrid_texture;
    
    public:
    friend class Mouse_T;
    friend class Simulation;
    
    // TODO: make a version with const(expr) indecies and IDs
    class Cell: public sf::RectangleShape {
        static constexpr float colorscaling {64.0}; // density required for all-white color;
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
            setOutlineColor(sf::Color(0xFFFFFF40));  // mostly-transparent white
            setOutlineThickness(0.5);
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
    std::vector<Cell*> GetCellNeighbors(const std::size_t UUID) const;
    // finds cells at every distance up to (and including) current DIFFUSION_RADIUS
    std::vector<Cell*> GetCellNeighbors(const std::size_t UUID, const unsigned int radialdist) const;
    std::vector<DoubleCoord> GetAdjacentPlus(const std::size_t UUID) const; // returns pairs of absolute and relative coords
    sf::Vector2f CalcDiffusionVec(std::size_t UUID) const;
    
    
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
    
    // TODO: rewrite draw calls, seperate out the sprite-creation
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

using CellPtrArray = std::vector<DiffusionField::Cell*>;

//template <std::size_t RD, std::size_t X, std::size_t Y>  // radial_distance, X/Y index into DensityGrid
//constexpr void UpdateDiffusion(RD, X, Y);  // updates the counts for cells in diffusion delta-map (for neighbors of radial_distance RD)


#endif
