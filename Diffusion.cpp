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
        std::cout << "(" << x << ", " << y << ") ";
    }
    return count;
}
