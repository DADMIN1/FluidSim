#include "Diffusion.hpp"

#include <vector>
#include <iostream>



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
const auto GetNeighbors(const int radial_distance) // relative verison
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
        default: assert(false && "Neighbor-coords not defined for radial distance: " && radial_distance);
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
        else if((resultX > DiffusionField_T::maxIX) || (resultY > DiffusionField_T::maxIY)) continue;
        else {
            absoluteCoords.push_back({resultX, resultY});
        }
    }
    return absoluteCoords;
}

// the pairs within DoubleCoords are ordered: Absolute, Relative
const std::vector<DoubleCoord> DiffusionField_T::GetAdjacentPlus(const std::size_t UUID)
{
    const Cell& baseCell = cells.at(UUID);
    const int IX = baseCell.IX;
    const int IY = baseCell.IY;
    
    std::vector<DoubleCoord> coords{};
    for (const auto& [dx, dy] : GetNeighbors(1)) {
        const int resultX = IX+dx;
        const int resultY = IY+dy;
        if((resultX < 0) || (resultY < 0)) continue;
        else if((resultX > DiffusionField_T::maxIX) || (resultY > DiffusionField_T::maxIY)) continue;
        else {
            coords.push_back({{resultX, resultY}, {dx, dy}});
        }
    }
    return coords;
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
    for (int d{1}; d < radial_distance; ++d) {
        // absolute overload of GetNeighbors already does a bounds-check on results
        absoluteCoords.push_back(GetNeighbors(d, IX, IY));
    }
    return absoluteCoords;
};


using CellRef_T = std::vector<DiffusionField_T::Cell*>;

const CellRef_T DiffusionField_T::GetCellNeighbors(const std::size_t UUID)
{
    const Cell& cell = cells.at(UUID);
    const Coordlist coords = GetNeighbors(1, cell.IX, cell.IY);
    std::vector<Cell*> reflist;
    for (const auto& [ix, iy]: coords) {
        reflist.push_back(cellmatrix.at(ix).at(iy));
    }
    return reflist;
}
//const std::vector<Cell*> reflist{GetCellNeighbors(UUID)};

const sf::Vector2f DiffusionField_T::CalcDiffusionVec(const std::size_t UUID)
{
    const Cell& cell = cells.at(UUID);
    const std::vector<DoubleCoord> coordpairs = GetAdjacentPlus(UUID);
    sf::Vector2f forcevector {0.0f, 0.0f};
    /* CellRef_T reflist {};
    reflist.reserve(coordpairs.size()); */
    for (const auto& [abs, rel]: coordpairs) {
        const auto& [ix, iy] = abs;
        //reflist.push_back(cellmatrix.at(ix).at(iy));
        Cell* neighbor = cellmatrix.at(ix).at(iy);
        const float relativeDensity = cell.density - neighbor->density;
        forcevector += sf::Vector2f { 
            float(-1.0*relativeDensity*rel.first),
            float(-1.0*relativeDensity*rel.second),
        };
    }
    return forcevector;
}
