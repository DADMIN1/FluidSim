#ifndef FLUIDSIM_MOUSE_HPP_INCLUDED
#define FLUIDSIM_MOUSE_HPP_INCLUDED

#include <SFML/Graphics/CircleShape.hpp>

#include "Globals.hpp"
#include "Diffusion.hpp"


// Provides mouse-related interactions for the simulation
class Mouse_T: public sf::CircleShape 
{
    enum Mode {
        None,
        Push, // Acts like a high-density particle
        Pull, // Acts like a negative-density particle
        Drag, // Captures particles inside radius
        Fill, // spawn more particles
        Erase, // erase particles
    } mode {None};
    
    static constexpr float defaultRadius {SPATIAL_RESOLUTION*2};
    float radius {defaultRadius};
    DiffusionField_T::CellMatrix* matrixptr {nullptr}; // &fluid.DiffusionFields[0]
    DiffusionField_T::Cell* hoveredCell {nullptr};
    float originalDensity{0.0f};  // needs to restore the cell's density after moving or releasing-button
    //CellRef_T adjacent
    
    
    public:
    bool insideWindow{false};
    
    Mouse_T(DiffusionField_T::CellMatrix* const mptr): sf::CircleShape(defaultRadius)
    {
        setOutlineThickness(3.f);
        setOutlineColor(sf::Color::White);
        setFillColor(sf::Color::Transparent);
        setOrigin(defaultRadius, defaultRadius);
        matrixptr = mptr;
    }
    
    void UpdatePosition(const int x, const int y);
    
    // modifies the properties like color/outline, and density?
    /* void ModifyCell(DiffusionField_T::Cell* cell) {
        return;
    }
    
    // restores original state (undoes the changes made by ModifyCell)
    void RestoreCell(DiffusionField_T::Cell* cell) {
        return;
    } */
    
};

#endif
