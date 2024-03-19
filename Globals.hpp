#ifndef FLUIDSIM_GLOBALS_HPP_INCLUDED
#define FLUIDSIM_GLOBALS_HPP_INCLUDED

constexpr int NUMCOLUMNS {50}, NUMROWS {50};
constexpr int BOXWIDTH {1000}, BOXHEIGHT {1000};
//constexpr float DEFAULTRADIUS {float(BOXWIDTH/NUMCOLUMNS) / 2.0f};
constexpr float DEFAULTRADIUS {4.f};
constexpr int DEFAULTPOINTCOUNT {10}; // number of points used to draw each circle
constexpr unsigned int SPATIAL_RESOLUTION {25};  // units/pixels per grid-cell for calculating diffusion/collision

constexpr float INITIALSPACINGX {BOXWIDTH/NUMCOLUMNS};
constexpr float INITIALSPACINGY {BOXHEIGHT/NUMROWS};
constexpr float INITIALOFFSETX {(INITIALSPACINGX/2.0f) - DEFAULTRADIUS};
constexpr float INITIALOFFSETY {(INITIALSPACINGY/2.0f) - DEFAULTRADIUS};


static_assert((DEFAULTRADIUS > 0.0) && "Radius must be greater than 0");
static_assert((DEFAULTPOINTCOUNT > 1) && "Pointcount must be greater than ");
static_assert((NUMCOLUMNS > 0) && (NUMROWS > 0) && "Columns and Rows must be greater than 0");
//static_assert(SPATIAL_RESOLUTION > 0)

#endif
