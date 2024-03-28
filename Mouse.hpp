#ifndef FLUIDSIM_MOUSE_HPP_INCLUDED
#define FLUIDSIM_MOUSE_HPP_INCLUDED

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Window.hpp>  // defines mouse and window

#include "Globals.hpp"
#include "Diffusion.hpp"


// Provides mouse-related interactions for the simulation
class Mouse_T: private sf::Mouse, public sf::CircleShape
{
    enum Mode {
        None,  // Disabled
        Push,  // Acts like a high-density particle
        Pull,  // Acts like a negative-density particle
        Drag,  // Captures particles inside radius
        Fill,  // spawn more particles
        Erase, // erase particles
    } mode {None};
    
    const sf::Window& window;
    static constexpr float defaultRadius {SPATIAL_RESOLUTION*1.5};
    float radius {defaultRadius};

    using Cell = DiffusionField_T::Cell;
    using CellMatrix = DiffusionField_T::CellMatrix;
    CellMatrix* matrixptr {nullptr}; // &fluid.DiffusionFields[0]
    Cell* hoveredCell {nullptr};
    
    //static void sf::Mouse::setPosition(const sf::Vector2i& position);
    //static void sf::Mouse::setPosition(const Vector2i& position, const Window& relativeTo);
    using sf::CircleShape::setPosition;  // disambiguates the call, preventing (unqualified) calls to sf::Mouse::setPosition()
    // is it actually necessary to even inherit from sf::Mouse?
    
    public:
    bool shouldDisplay{false};  // controls drawing of the mouse (circle and hoveredCell-outline)
    
    // Do not call the constructor for sf::Mouse (it's virtual)?
    Mouse_T(sf::Window& theWindow, CellMatrix* const mptr)
    : sf::CircleShape(defaultRadius), window{theWindow}
    {
        setOutlineThickness(1.f);
        setOutlineColor(sf::Color::White);
        setFillColor(sf::Color::Transparent);
        setOrigin(defaultRadius, defaultRadius);
        matrixptr = mptr;
    }
    
    void HandleEvent(sf::Event);
    void InvalidateHover(); // restores current hoveredCell (if any), then disables hovering
    void SwitchMode(Mode);
    
    // swaps the current mode with the saved mode; returns true unless mode is None
    bool ToggleActive() {
        static Mode prev {Push};
        if (mode == None) { SwitchMode(prev); prev = None; return true; }
        else              { prev = mode; SwitchMode(None); return false; }
    }
    
    private:
    void UpdateHovered(const int x, const int y);
    std::size_t StoreCell(Cell* const cellptr);  // saves the cell's current state, returns cellID
    void ModifyCell(const std::size_t cellID); // modifies cell's properties based on mode
    void RestoreCell(const std::size_t cellID); // restores original state and removes entry for cell
};

#endif
