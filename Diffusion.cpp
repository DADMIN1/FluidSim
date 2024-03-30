#include "Diffusion.hpp"

#include <vector>
#include <iostream>
#include <numeric>


void DiffusionField_T::PrintAllCells() const
{
    std::cout << "maxIX, maxIY = " << maxIX << ", " << maxIY << '\n';
    for (const Cell& cell: cells) {
        std::cout << "UUID: " << cell.UUID << " ";
        std::cout << "ix: " << cell.IX << " ";
        std::cout << "iy: " << cell.IY << " ";
        const auto& [x, y] = cell.getPosition();
        std::cout << "\n\tposition: " << x << ", " << y << '\n';
    }
}

// returns the relative coords of neighbors at radial_distance
int CalcBaseNCount(int radial_distance) {
    if (radial_distance == 0) { return 0; }
    std::vector<std::pair<int, int>> coords{};
    int count {0};
    for (int x{radial_distance}, y{radial_distance-x}; x>=0; --x, ++y) {
        //int y = radial_distance-x;
        coords.push_back({x, y});
        count += 1;
        if (y > 0)
        {
            coords.push_back({x, -y});
            count += 1;
        }
        if (x > 0) {
            coords.push_back({-x, y});
            count += 1;
            if (y > 0)
            {
                coords.push_back({-x, -y});
                count += 1;
            }
        }
    }
    
    for (auto& [x, y]: coords) {
        std::cout << "(" << x << ", " << y << "), ";
    }
    return count;
}


// TODO: write constexpr (template) version of this
const Coordlist GetNeighbors(const int radial_distance) // relative verison
{
    switch (radial_distance) {
        case 0: return Coordlist {};
        case 1: return Coordlist { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, };
        case 2: return Coordlist {
            {2, 0}, {-2, 0}, { 0, 2}, { 0, -2}, 
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1},
        };
        case 3: return Coordlist {
            {3, 0}, {-3, 0}, { 0, 3}, { 0, -3}, 
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, 
            {1, 2}, {1, -2}, {-1, 2}, {-1, -2},
        };
        case 4: return Coordlist {
            {4, 0}, {-4, 0}, { 0, 4}, { 0, -4},
            {3, 1}, {3, -1}, {-3, 1}, {-3, -1}, 
            {2, 2}, {2, -2}, {-2, 2}, {-2, -2}, 
            {1, 3}, {1, -3}, {-1, 3}, {-1, -3},
        };
        case 5: return Coordlist {
            {5, 0}, {-5, 0}, { 0, 5}, { 0, -5},
            {4, 1}, {4, -1}, {-4, 1}, {-4, -1}, 
            {3, 2}, {3, -2}, {-3, 2}, {-3, -2}, 
            {2, 3}, {2, -3}, {-2, 3}, {-2, -3}, 
            {1, 4}, {1, -4}, {-1, 4}, {-1, -4},
        };
        default: 
            assert(false && "Neighbor-coords not defined for radial distance: " && radial_distance);
            return Coordlist {};
    }
};

// X/Y parameters are indecies into DensityGrid
// absolute version; can be used as indecies into DensityGrid
const Coordlist GetNeighbors(const int radial_distance, const int IX, const int IY)
{
    Coordlist absoluteCoords{};
    for (const auto& [dx, dy] : GetNeighbors(radial_distance)) {
        const int resultX = IX+dx;
        const int resultY = IY+dy;
        if((resultX < 0) || (resultY < 0)) continue;
        else if((resultX > int(DiffusionField_T::maxIX)) || (resultY > int(DiffusionField_T::maxIY))) continue;
        else {
            absoluteCoords.push_back({resultX, resultY});
        }
    }
    return absoluteCoords;
}


// TODO: write constexpr (template) version of this
const auto GetNeighborsAll(const int radial_distance) // relative verison
{
    std::vector<Coordlist> coords{};
    coords.reserve(radial_distance-1);
    for (int d{1}; d < radial_distance; ++d) {
        // TODO: use a flat array (probably)
        /* const auto N = GetNeighborCoordsRel(d);
        coords.insert(coords.end(), N.begin(), N.end()); */
        coords.push_back(GetNeighbors(d));
    }
    return coords;
};


// X/Y parameters are indecies into DensityGrid
// absolute version; can be used as indecies into DensityGrid
const auto GetNeighborsAll(const int radial_distance, const int IX, const int IY)
{
    std::vector<Coordlist> absoluteCoords{};
    absoluteCoords.reserve(radial_distance-1);
    for (int d{1}; d <= radial_distance; ++d) {
        // absolute overload of GetNeighbors already does a bounds-check on results
        absoluteCoords.push_back(GetNeighbors(d, IX, IY));
    }
    return absoluteCoords;
};

// result is for only a single distance
const CellRef_T DiffusionField_T::GetCellNeighbors(const std::size_t UUID, const unsigned int radialdist) const
{
    assert((radialdist <= radialdist_limit) && "radialdist too large");
    const Cell& cell = cells.at(UUID);
    std::vector<Cell*> reflist{};
    const Coordlist coords = GetNeighbors(radialdist, cell.IX, cell.IY);
    for (const auto& [ix, iy]: coords) {
        reflist.push_back(cellmatrix.at(ix).at(iy));
    }
    return reflist;
}

// result is for every distance up to (and including) current DIFFUSION_RADIUS
const CellRef_T DiffusionField_T::GetCellNeighbors(const std::size_t UUID) const
{
    const Cell& cell = cells.at(UUID);
    std::vector<Cell*> reflist;
    for (unsigned int radius{1}; radius<=DIFFUSION_RADIUS; ++radius) {
        const Coordlist coords = GetNeighbors(radius, cell.IX, cell.IY);
        for (const auto& [ix, iy]: coords) {
            reflist.push_back(cellmatrix.at(ix).at(iy));
        }
    }
    return reflist;
}
//const std::vector<Cell*> reflist{GetCellNeighbors(UUID)};

// the pairs within DoubleCoords are ordered: Absolute, Relative
const std::vector<DoubleCoord> DiffusionField_T::GetAdjacentPlus(const std::size_t UUID) const
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
            else if ((resultX > int(DiffusionField_T::maxIX)) || (resultY > int(DiffusionField_T::maxIY))) continue;
            else {
                coords.push_back({{resultX, resultY}, {dx, dy}});
            }
        }
    }
    return coords;
}

const sf::Vector2f DiffusionField_T::CalcDiffusionVec(const std::size_t UUID) const
{
    const Cell& cell = cells.at(UUID);
    const std::vector<DoubleCoord> coordpairs = GetAdjacentPlus(UUID);
    sf::Vector2f forcevector {0.0f, 0.0f};
    /* CellRef_T reflist {};
    reflist.reserve(coordpairs.size()); */
    for (const auto& [abs, rel]: coordpairs) {
        const auto& [ix, iy] = abs;
        //reflist.push_back(cellmatrix.at(ix).at(iy));
        const Cell* const neighbor = cellmatrix.at(ix).at(iy);
        // scaling neighbor's density by distance
        const int orthodist = std::max(std::abs(rel.first), std::abs(rel.second));
        // taking max of either axis so that diagonals are considered a distance of 1, instead of 2
        const float magnitude = (cell.density - neighbor->density) * (1.0f - float(orthodist*DIFFUSION_SCALING));
        
        // we need to find the directional components of the vector (angle);
        int orthodist_sum {std::abs(rel.first) + std::abs(rel.second)};
        std::pair<float, float> angleComponents { 
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
