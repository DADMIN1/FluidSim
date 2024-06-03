#ifndef FLUIDSIM_DIFFUSION_HPP_INCLUDED
#define FLUIDSIM_DIFFUSION_HPP_INCLUDED

#include <array>
//#include <cassert>

#include <SFML/Graphics.hpp>  // rendertexture
//#include <SFML/Graphics/RectangleShape.hpp>

#include "Globals.hpp"
#include "Cell.hpp"


// DIFFUSIONSCALING //

// TODO: scaling function that always totals to 1.0 regardless of radius (use trig functions)
constexpr float GetNextFloat(const float x, const int orthodist) {
    //float ratio = float(orthodist) / float(1.0 + DIFFUSION_RADIUS - orthodist);
    return x - (1.0 / float(4.0f*orthodist)); // formula for count (of adjacent cells) is simply 4*N
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


class DiffusionField
{
    sf::RenderTexture cellgrid_texture;
    
    // these are added with signed-ints in GetCellNeighbors (because the result might be negative); hence the assertion.
    static_assert((Cell::maxIX < __INT_MAX__) && (Cell::maxIY < __INT_MAX__), "max-indecies will overflow");
    
    using CellMatrix = std::array<std::array<Cell*, Cell::maxSizeY>, Cell::maxSizeX>;
    //using CellArray = std::array<Cell, ((maxSizeY)*(maxSizeX))>; // crashes
    using CellArray = std::vector<Cell>; // doesn't crash
    CellArray cells; // TODO: figure out how to do this with an array without crashing
    CellMatrix cellmatrix;
    
    public:
    friend class Simulation;
    friend class Mouse_T;
    
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
        
        cells.reserve((Cell::maxSizeY)*(Cell::maxSizeX));
        
        unsigned int ID = 0;
        for (unsigned int c{0}; c < (Cell::maxSizeX); ++c) {
            for (unsigned int r{0}; r < (Cell::maxSizeY); ++r) {
                //cells[ID] = Cell{c, r, ID};
                Cell& newcell = cells.emplace_back(c, r, ID++);
                cellmatrix[c][r] = &newcell;
            }
        }
        return true;
    }
    
    void PrintAllCells() const;
    
    sf::Sprite GetSprite() { return sf::Sprite(cellgrid_texture.getTexture()); }
    void Redraw() 
    {
        cellgrid_texture.clear(sf::Color::Transparent);
        for (auto& cell : cells) {
            cell.UpdateColor();
            cellgrid_texture.draw(cell);
        }
        cellgrid_texture.display();
    }
    
    void ResetMomentum() {
        for (Cell& cell: cells) {
            cell.momentum = {0.0, 0.0};
        }
    }
};


//template <std::size_t RD, std::size_t X, std::size_t Y>  // radial_distance, X/Y index into DensityGrid
//constexpr void UpdateDiffusion(RD, X, Y);  // updates the counts for cells in diffusion delta-map (for neighbors of radial_distance RD)


#endif
