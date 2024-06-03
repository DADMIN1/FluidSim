#include "Cell.hpp"

#include <iostream>
#include <cassert>



void AdjacentCellsTest()
{
    constexpr int Rdist = 16;
    constexpr auto coords = LocalCells<Rdist>::BaseRelativeCoords();
    std::cout << "\nradial_dist@" << Rdist << " = " << (coords.size()*2)-2 << '\n';
    // x2 for each pair of coords, minus two duplicates at start and end
    for (const auto& [firstcoord, secondcoord]: coords) {
        for (const auto& [x, y]: {firstcoord, secondcoord}) {
            std::cout << "\t{ " << x << " , " << y << " }\t";
            if (y == 0) break;  // skip duplicates on either end
        }
        std::cout << "\n";
    }
    std::cout << "\n\n";
    return;
}


// prints the relative coords of neighbors at radial_distance, and returns total count
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
CoordlistRel GetNeighbors(const int radial_distance) // relative verison
{
    switch (radial_distance) {
        case 0: return CoordlistRel {};
        case 1: return CoordlistRel { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, };
        case 2: return CoordlistRel {
            {2, 0}, {-2, 0}, { 0, 2}, { 0, -2}, 
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1},
        };
        case 3: return CoordlistRel {
            {3, 0}, {-3, 0}, { 0, 3}, { 0, -3}, 
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, 
            {1, 2}, {1, -2}, {-1, 2}, {-1, -2},
        };
        case 4: return CoordlistRel {
            {4, 0}, {-4, 0}, { 0, 4}, { 0, -4},
            {3, 1}, {3, -1}, {-3, 1}, {-3, -1}, 
            {2, 2}, {2, -2}, {-2, 2}, {-2, -2}, 
            {1, 3}, {1, -3}, {-1, 3}, {-1, -3},
        };
        case 5: return CoordlistRel {
            {5, 0}, {-5, 0}, { 0, 5}, { 0, -5},
            {4, 1}, {4, -1}, {-4, 1}, {-4, -1}, 
            {3, 2}, {3, -2}, {-3, 2}, {-3, -2}, 
            {2, 3}, {2, -3}, {-2, 3}, {-2, -3}, 
            {1, 4}, {1, -4}, {-1, 4}, {-1, -4},
        };
        default: 
            assert(false && "Neighbor-coords not defined for radial distance: " && radial_distance);
            return CoordlistRel {};
    }
};


// X/Y parameters are indecies into DensityGrid
// absolute version; can be used as indecies into DensityGrid
CoordlistAbs GetNeighbors(const int radial_distance, const int IX, const int IY)
{
    CoordlistAbs absoluteCoords{};
    for (const auto& [dx, dy] : GetNeighbors(radial_distance)) {
        const int resultX = IX+dx;
        const int resultY = IY+dy;
        // TODO: return something to handle the edges (out-of-bounds)
        if((resultX < 0) || (resultY < 0)) continue;
        else if((resultX > int(Cell::maxIX)) || (resultY > int(Cell::maxIY))) continue;
        else {
            absoluteCoords.push_back({resultX, resultY});
        }
    }
    return absoluteCoords;
}


// TODO: write constexpr (template) version of this
std::vector<CoordlistRel> GetNeighborsAll(const int radial_distance) // relative verison
{
    std::vector<CoordlistRel> coords{};
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
std::vector<CoordlistAbs> GetNeighborsAll(const int radial_distance, const int IX, const int IY)
{
    std::vector<CoordlistAbs> absoluteCoords{};
    absoluteCoords.reserve(radial_distance-1);
    for (int d{1}; d <= radial_distance; ++d) {
        // absolute overload of GetNeighbors already does a bounds-check on results
        absoluteCoords.push_back(GetNeighbors(d, IX, IY));
    }
    return absoluteCoords;
};

