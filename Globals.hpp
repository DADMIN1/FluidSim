#ifndef FLUIDSIM_GLOBALS_HPP_INCLUDED
#define FLUIDSIM_GLOBALS_HPP_INCLUDED


constexpr int NUMCOLUMNS {64}, NUMROWS {64}; // layout (and number) of particles spawned during init
constexpr int BOXWIDTH {1000}, BOXHEIGHT {1000}; // internal resolution (default window resolution should match)
//constexpr float DEFAULTRADIUS {float(BOXWIDTH/NUMCOLUMNS) / 2.0f};
constexpr float DEFAULTRADIUS {10.0f}; // of particles (sf::CircleShape)
constexpr int DEFAULTPOINTCOUNT {8}; // number of points used to draw each circle (particles)
constexpr unsigned int SPATIAL_RESOLUTION {20}; // units/pixels per grid-cell for calculating diffusion/collision

static_assert((DEFAULTRADIUS > 0.0), "Radius must be greater than 0");
static_assert((DEFAULTPOINTCOUNT > 1), "Pointcount must be greater than ");
static_assert((NUMCOLUMNS > 0) && (NUMROWS > 0), "Columns and Rows must be greater than 0");

// if the field's dimensions are not evenly divisible by cell-size, we need an extra cell to cover the remainder
constexpr unsigned int maxindexAdjX = ((BOXWIDTH  % SPATIAL_RESOLUTION) == 0? 1 : 0);
constexpr unsigned int maxindexAdjY = ((BOXHEIGHT % SPATIAL_RESOLUTION) == 0? 1 : 0);
// the order is correct; it's subtracted from the max-index (because normally: maxIndex = size-1)

// calculations for initial positioning of particles
constexpr float INITIALSPACINGX {BOXWIDTH/NUMCOLUMNS};
constexpr float INITIALSPACINGY {BOXHEIGHT/NUMROWS};
constexpr float INITIALOFFSETX {(INITIALSPACINGX/2.0f) - DEFAULTRADIUS};
constexpr float INITIALOFFSETY {(INITIALSPACINGY/2.0f) - DEFAULTRADIUS};

// diffusion stuff
constexpr int radialdist_limit{5}; // highest radial_distance implemented by GetNeighbors
constexpr int DIFFUSION_RADIUS{4}; // range in orthogonal-distance (grid-cells) used for diffusion/density calculations
                                   // (radius of 0 means only current cell is considered)
static_assert((DIFFUSION_RADIUS <= radialdist_limit), "Diffusion-radius is too big");



#endif
