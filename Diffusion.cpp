#include "Diffusion.hpp"

#include <vector>
#include <iostream>

// Doesn't this fail at the max height/width? Need to modify Fluid.Update
/* const float* CellLookup(const float positionX, const float positionY)
{
    return &DensityGrid[int(positionX)*SPATIAL_RESOLUTION][int(positionY)*SPATIAL_RESOLUTION];
} */

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

using Coordlist = std::vector<std::pair<int, int>>;

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
const auto GetNeighbors(const int radial_distance, const int IX, const int IY)
{
    Coordlist absoluteCoords{};
    for (const auto& [x, y] : GetNeighbors(radial_distance)) {
        if ((IX+x >= 0) && (IY+y >= 0))
        {
            //absoluteCoords.push_back(std::make_pair(IX+x, IY+y));
            absoluteCoords.push_back({IX+x, IY+y});
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
    for (int d{1}; d < radial_distance; ++d) {
        // absolute overload of GetNeighbors already does a bounds-check on results
        absoluteCoords.push_back(GetNeighbors(d, IX, IY));
    }
    return absoluteCoords;
};


/* void DiffusionField_T::Initialize() 
{
    int ID = 0;
    for (int c{0}; c < SPATIAL_RESOLUTION*BOXHEIGHT; ++c) {
        for (int r{0}; r < SPATIAL_RESOLUTION*BOXWIDTH; ++r) {
            cells[ID] = Cell{c, r, ID};
            cellmatrix[c][r] = &cells[ID];
            ++ID;
        }
    }
} */

/* void DiffusionField_T::Draw() 
{
    cellgrid_texture.clear(sf::Color::Transparent);
    for (auto& cell : cells) {
        cell.density = 2.5;
        cell.UpdateColor();
        cellgrid_texture.draw(cell);
    }
    cellgrid_texture.display();
} */
