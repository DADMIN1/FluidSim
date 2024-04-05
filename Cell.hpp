#ifndef FLUIDSIM_CELL_INCLUDED
#define FLUIDSIM_CELL_INCLUDED

#include <vector>
#include <tuple> //std::pair

#include <SFML/Graphics/RectangleShape.hpp>

#include "Globals.hpp"


// prints the relative coords of neighbors at radial_distance, and returns total count
int CalcBaseNCount(int radial_distance);


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

// TODO: move all the 'finding Neighbor'-related stuff to ANOTHER file??
template <int RD, int X, int Y>  // radial_distance, X/Y index into DensityGrid
class NeighborCells {
    static constexpr int Ncount = BaseNeighborCount<RD, X, Y>();
    constexpr std::array<float*, Ncount> GetNeighborCells();
};


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
    
    // maybe better in Coord class??
    static constexpr unsigned int maxIX = (BOXWIDTH/SPATIAL_RESOLUTION - maxindexAdjX);
    static constexpr unsigned int maxIY = (BOXHEIGHT/SPATIAL_RESOLUTION - maxindexAdjY);
    static constexpr unsigned int maxSizeX = maxIX+1;
    static constexpr unsigned int maxSizeY = maxIY+1;
    
    
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


// TODO: use these to replace 'Coordlist-Rel/Abs'
struct CoordBase_T
{
    enum Tag {
        Invalid  = 0,
        Edge     = 1, // out-of-bounds, but within the diffusionRadius of a real cell
        Relative = 2,
        Absolute = 4,
    } tag;
    
    int x, y;
    
    CoordBase_T(Tag t): tag{t}
    {}
};

struct CoordAbs: virtual CoordBase_T
{
    std::size_t UUID;
    bool hasUUID{false}; // sometimes it's unknown and not important?
    CoordAbs(): CoordBase_T(Absolute)
    {}
};

struct CoordRel: virtual CoordBase_T 
{
    CoordAbs origin;
    CoordRel(): CoordBase_T(Relative)
    {}
};


using CoordlistRel = std::vector<std::pair<int, int>>;
using CoordlistAbs = std::vector<std::pair<int, int>>;
using DoubleCoord = std::pair<std::pair<int, int>, std::pair<int, int>>;

CoordlistRel GetNeighbors(const int radial_distance); // relative verison
// absolute version; can be used as indecies into DiffusionField's CellMatrix
CoordlistAbs GetNeighbors(const int radial_distance, const int IX, const int IY);
// X/Y parameters are indecies into DiffusionField's CellMatrix

std::vector<CoordlistRel> GetNeighborsAll(const int radial_distance); // relative verison
// absolute version; can be used as indecies into DiffusionField's CellMatrix
std::vector<CoordlistAbs> GetNeighborsAll(const int radial_distance, const int IX, const int IY);
// X/Y parameters are indecies into DiffusionField's CellMatrix




#endif
