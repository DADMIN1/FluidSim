#include "Diffusion.hpp"

#include <vector>
#include <iostream>


void DiffusionField::PrintAllCells() const
{
    std::cout << "maxIX, maxIY = " << Cell::maxIX << ", " << Cell::maxIY << '\n';
    for (const Cell& cell: cells) {
        std::cout << "UUID: " << cell.UUID << " ";
        std::cout << "ix: " << cell.IX << " ";
        std::cout << "iy: " << cell.IY << " ";
        const auto& [x, y] = cell.getPosition();
        std::cout << "\n\tposition: " << x << ", " << y << '\n';
    }
}


// result is for only a single distance
std::vector<Cell*> DiffusionField::GetCellNeighbors(const std::size_t UUID, const unsigned int radialdist) const
{
    const Cell& cell = cells.at(UUID);
    std::vector<Cell*> reflist{};
    const CoordlistAbs coords = GetNeighbors(radialdist, cell.IX, cell.IY);
    for (const auto& [ix, iy]: coords) {
        reflist.push_back(cellmatrix.at(ix).at(iy));
    }
    return reflist;
}

// result is for every distance up to (and including) current DIFFUSION_RADIUS
std::vector<Cell*> DiffusionField::GetCellNeighbors(const std::size_t UUID) const
{
    const Cell& cell = cells.at(UUID);
    std::vector<Cell*> reflist;
    for (unsigned int radius{1}; radius<=DIFFUSION_RADIUS; ++radius) {
        const CoordlistAbs coords = GetNeighbors(radius, cell.IX, cell.IY);
        for (const auto& [ix, iy]: coords) {
            reflist.push_back(cellmatrix.at(ix).at(iy));
        }
    }
    return reflist;
}
//const std::vector<Cell*> reflist{GetCellNeighbors(UUID)};

// the pairs within DoubleCoords are ordered: Absolute, Relative
std::vector<DoubleCoord> DiffusionField::GetAdjacentPlus(const std::size_t UUID) const
{
    const Cell& baseCell = cells.at(UUID);
    const int IX = baseCell.IX;
    const int IY = baseCell.IY;
    
    std::vector<DoubleCoord> coords{};
    for (unsigned int radius{DIFFUSION_RADIUS}; radius>0; --radius) {
        for (const auto& [dx, dy] : GetNeighbors(radius)) {
            const int resultX = IX+dx;
            const int resultY = IY+dy;
            if ((resultX < 0) || (resultY < 0)) continue;
            else if ((resultX > int(Cell::maxIX)) || (resultY > int(Cell::maxIY))) continue;
            else {
                coords.push_back({{resultX, resultY}, {dx, dy}});
            }
        }
    }
    return coords;
}

// TODO: repurpose this function to smooth out momentum between cells, instead of creating a diffusionforce
// TODO: store the constexpr parts seperately
// coordpairs, cellmatrix-indecies, orthodist, etc. will all be static for a given UUID
sf::Vector2f DiffusionField::CalcDiffusionVec(const std::size_t UUID) const
{
    const Cell& cell = cells.at(UUID);
    const std::vector<DoubleCoord> coordpairs = GetAdjacentPlus(UUID);
    sf::Vector2f forcevector {0.0f, 0.0f};
    
    for (const auto& [abs, rel]: coordpairs) {
        const auto& [ix, iy] = abs;
        const Cell* const neighbor = cellmatrix.at(ix).at(iy);
        
        // scaling neighbor's density by distance
        // taking max of either axis so that diagonals are considered a distance of 1, instead of 2
        const int  diagdist = std::max(std::abs(rel.first), std::abs(rel.second));
        //const int orthodist = std::abs(rel.first) + std::abs(rel.second);
        const float magnitude = (cell.density - neighbor->density) * DIFFUSIONSCALING[diagdist];
        
        // we need to find the directional components of the vector (angle);
        const int orthodist_sum {std::abs(rel.first) + std::abs(rel.second)};
        const std::pair<float, float> angleComponents { 
            float(rel.first /orthodist_sum), 
            float(rel.second/orthodist_sum), 
        };
        
        forcevector += sf::Vector2f { 
            float(magnitude*angleComponents.first),
            float(magnitude*angleComponents.second),
        };
    }
    
    return forcevector;
}
