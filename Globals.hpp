#ifndef FLUIDSIM_GLOBALS_HPP_INCLUDED
#define FLUIDSIM_GLOBALS_HPP_INCLUDED

constexpr int NUMCOLUMNS {50}, NUMROWS {50};
constexpr int BOXWIDTH {1000}, BOXHEIGHT {1000};
//constexpr float DEFAULTRADIUS {float(BOXWIDTH/NUMCOLUMNS) / 2.0f};
constexpr float DEFAULTRADIUS {8.0f};
constexpr int DEFAULTPOINTCOUNT {20}; // number of points used to draw each circle
constexpr unsigned int SPATIAL_RESOLUTION {30};  // units/pixels per grid-cell for calculating diffusion/collision

// if the field's dimensions are not evenly divisible by cell-size, we need an extra cell to cover the remainder
constexpr unsigned int maxindexAdjX = ((BOXWIDTH  % SPATIAL_RESOLUTION) == 0? 1 : 0);
constexpr unsigned int maxindexAdjY = ((BOXHEIGHT % SPATIAL_RESOLUTION) == 0? 1 : 0);
// the order is correct; it's subtracted from the max-index (because normally: maxIndex = size-1)

constexpr float INITIALSPACINGX {BOXWIDTH/NUMCOLUMNS};
constexpr float INITIALSPACINGY {BOXHEIGHT/NUMROWS};
constexpr float INITIALOFFSETX {(INITIALSPACINGX/2.0f) - DEFAULTRADIUS};
constexpr float INITIALOFFSETY {(INITIALSPACINGY/2.0f) - DEFAULTRADIUS};


static_assert((DEFAULTRADIUS > 0.0) && "Radius must be greater than 0");
static_assert((DEFAULTPOINTCOUNT > 1) && "Pointcount must be greater than ");
static_assert((NUMCOLUMNS > 0) && (NUMROWS > 0) && "Columns and Rows must be greater than 0");
//static_assert(SPATIAL_RESOLUTION > 0)

#endif
