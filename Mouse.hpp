#ifndef FLUIDSIM_MOUSE_HPP_INCLUDED
#define FLUIDSIM_MOUSE_HPP_INCLUDED

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Window.hpp>  // defines mouse and window

#include "Globals.hpp"
#include "Diffusion.hpp"


// Provides mouse-related interactions for the simulation
class Mouse_T: public sf::Mouse, public sf::CircleShape
{
    enum Mode {
        None,
        Push, // Acts like a high-density particle
        Pull, // Acts like a negative-density particle
        Drag, // Captures particles inside radius
        Fill, // spawn more particles
        Erase, // erase particles
    } mode {None};
    
    const sf::Window& window;
    static constexpr float defaultRadius {SPATIAL_RESOLUTION*1.5};
    float radius {defaultRadius};
    DiffusionField_T::CellMatrix* matrixptr {nullptr}; // &fluid.DiffusionFields[0]
    DiffusionField_T::Cell* hoveredCell {nullptr};
    DiffusionField_T::Cell savedState {}; // needed to restore the cell's density () after moving or releasing-button
    
    //static void sf::Mouse::setPosition(const sf::Vector2i& position);
    //static void sf::Mouse::setPosition(const Vector2i& position, const Window& relativeTo);
    using sf::CircleShape::setPosition;  // disambiguates the call, preventing (unqualified) calls to sf::Mouse::setPosition()
    // is it actually necessary to even inherit from sf::Mouse?
    
    
    public:
    bool insideWindow{false};
    
    
    // Do not call the constructor for sf::Mouse (it's virtual)?
    Mouse_T(sf::Window& theWindow, DiffusionField_T::CellMatrix* const mptr)
    : sf::CircleShape(defaultRadius), window{theWindow}
    {
        setOutlineThickness(1.f);
        setOutlineColor(sf::Color::White);
        setFillColor(sf::Color::Transparent);
        setOrigin(defaultRadius, defaultRadius);
        matrixptr = mptr;
    }
    
    void UpdateHovered(const int x, const int y);
    void HandleEvent(sf::Event);
    
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
