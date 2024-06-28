#ifndef FLUIDSIM_CELL_INCLUDED
#define FLUIDSIM_CELL_INCLUDED

#include <vector>
#include <tuple>  //std::pair
#include <ranges> //LocalCells

#include <SFML/Graphics/RectangleShape.hpp>

#include "Globals.hpp"


using DoubleCoord = std::pair<std::pair<int, int>, std::pair<int, int>>;


template <int RD> // radial_distance
struct LocalCells
{
    static constexpr int RadialDistance{RD};
    static constexpr int Basecount   {4*RD};
    
    // returns total (sum) count of cells WITHIN the radial distance
    static constexpr int BasecountTotal() {
        if constexpr (RD <= 0) { return 0; }
        return Basecount + LocalCells<RD-1>::BasecountTotal();
    }
    
    // 'Base-' variables/operations don't account for the actual cell's location (which may be an edge)
    static constexpr auto BaseRelativeCoords()
    {
        // iota_view with negative numbers is not symmetric (endpoint is not included)
        // it's correct to add one anyway, otherwise the range of values would be RD-1        
        constexpr auto xaxis = std::ranges::iota_view{ -RD, RD+1 };
        constexpr auto inverseRD = std::views::transform(xaxis, [](const int i) {
            int insanemod {(RD-(i%RD))%-RD}; // don't ask
            return 
                (i==0)?  RD : // sets midpoint to RD instead of zero
                ((i<0)? (RD-insanemod)%RD : insanemod);
                //     the '%RD' here: ^ just sets the first number to 0 (instead of RD)
        });
        
        static_assert(xaxis.size()  == inverseRD.size());
        
        // pairs of coords inverted (negative) on one axis (and matching along the other)
        // inverting x instead of y is functionally equivalent, and 
        constexpr auto MakeCoordPair = [](const int x, const int y){ return DoubleCoord{ std::pair{x,y}, std::pair{x,-y} }; };
        constexpr auto coords = std::ranges::zip_transform_view(MakeCoordPair, xaxis, inverseRD);
        return coords;
    }
};


template<>
struct LocalCells<0> {
    static constexpr int  RadialDistance{0};
    static constexpr int  Basecount     {0};
    static constexpr int  BasecountTotal()     { return 0; }
    static constexpr auto BaseRelativeCoords() { return DoubleCoord{{0, 0}, {0, 0}}; }
};


// TODO: make a version with const(expr) indecies and IDs
class Cell: public sf::RectangleShape {
    static constexpr float colorscaling {64.0}; // density required for all-white color;
    unsigned int IX, IY, UUID; // UUID is the array index of Cell
    sf::Vector2f diffusionVec{0.0, 0.0}; // force calculated from the density of nearby cells
    sf::Vector2f momentum{0.0, 0.0}; // stored velocity imparted by particles, distributed to local particles
    float density{0.0};
    
    void Reset() {
        diffusionVec = {0.f, 0.f};
        momentum     = {0.f, 0.f};
        density      =  0.f;
    }
    // IX, IY, UUID will be handled by Simulation::Reset()
    
    public:
    friend class DiffusionField;
    friend class Simulation;
    friend class Mouse_T;
    
    // maybe better in Coord class??
    // if the field's dimensions are not evenly divisible by cell-size, we need an extra cell to cover the remainder
    static constexpr unsigned int maxIX = (BOXWIDTH /SPATIAL_RESOLUTION + ((BOXWIDTH %SPATIAL_RESOLUTION)? 1:0));
    static constexpr unsigned int maxIY = (BOXHEIGHT/SPATIAL_RESOLUTION + ((BOXHEIGHT%SPATIAL_RESOLUTION)? 1:0));
    static constexpr unsigned int arraySizeX = maxIX+1;
    static constexpr unsigned int arraySizeY = maxIY+1;
    
    
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

CoordlistRel GetNeighbors(const int radial_distance); // relative verison
// absolute version; can be used as indecies into DiffusionField's CellMatrix
CoordlistAbs GetNeighbors(const int radial_distance, const int IX, const int IY);
// X/Y parameters are indecies into DiffusionField's CellMatrix

std::vector<CoordlistRel> GetNeighborsAll(const int radial_distance); // relative verison
// absolute version; can be used as indecies into DiffusionField's CellMatrix
std::vector<CoordlistAbs> GetNeighborsAll(const int radial_distance, const int IX, const int IY);
// X/Y parameters are indecies into DiffusionField's CellMatrix




#endif
