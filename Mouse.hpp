#ifndef FLUIDSIM_MOUSE_HPP_INCLUDED
#define FLUIDSIM_MOUSE_HPP_INCLUDED

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window.hpp>  // defines mouse and window

#include "Globals.hpp"
#include "Diffusion.hpp"


// Provides mouse-related interactions for the simulation
class Mouse_T: private sf::Mouse, public sf::CircleShape
{
    enum Mode {
        Disabled,
        None,  // Inactive, but enabled (waiting for clicks)
        Push,  // Acts like a high-density particle
        Pull,  // Acts like a negative-density particle
        Drag,  // Captures particles inside radius
        Fill,  // spawn more particles
        Erase, // erase particles
    } mode {Disabled};
    
    const sf::Window& window;
    static constexpr float defaultRadius {SPATIAL_RESOLUTION*2.5};
    float radius {defaultRadius};
    float strength {64.0};        // for push/pull modes
    unsigned int radialDist {3};  // distance of adjacent cells included in effect

    using Cell = DiffusionField_T::Cell;
    DiffusionField_T* fieldptr {nullptr}; // &fluid.DiffusionFields[0]
    Cell* hoveredCell {nullptr};
    
    //static void sf::Mouse::setPosition(const sf::Vector2i& position);
    //static void sf::Mouse::setPosition(const Vector2i& position, const Window& relativeTo);
    using sf::CircleShape::setPosition;  // disambiguates the call, preventing (unqualified) calls to sf::Mouse::setPosition()
    // is it actually necessary to even inherit from sf::Mouse?
    
    public:
    bool shouldDisplay{false};  // controls drawing of the mouse (circle)
    bool shouldOutline{false}; // hoveredCell-outline
    sf::RectangleShape outlined;
    
    // Do not call the constructor for sf::Mouse (it's virtual)?
    Mouse_T(sf::Window& theWindow, DiffusionField_T* const mptr)
    : sf::CircleShape(defaultRadius), window{theWindow}
    {
        setOutlineThickness(2.f);
        setOutlineColor(sf::Color::Cyan);
        setFillColor(sf::Color::Transparent);
        setOrigin(defaultRadius, defaultRadius);
        fieldptr = mptr;
        
        // default cell appearance
        sf::RectangleShape basecell {sf::Vector2f{SPATIAL_RESOLUTION, SPATIAL_RESOLUTION}};
        basecell.setFillColor(sf::Color::Transparent);
        basecell.setOutlineColor(sf::Color(0xFFFFFF80));  // half-transparent white
        basecell.setOutlineThickness(1);
        
        outlined = basecell;
        outlined.setOutlineColor(sf::Color::Cyan);
        outlined.setOutlineThickness(2.5f);
    }
    
    void HandleEvent(sf::Event);
    void InvalidateHover(); // restores current hoveredCell (if any), then disables hovering
    void SwitchMode(Mode);
    
    // swaps the current mode with the saved mode; returns true unless mode is None
    bool ToggleActive() {
        static Mode prev {None};
        if (mode == Disabled) { SwitchMode(prev); prev = Disabled; return true;  }
        else                  { prev = mode; SwitchMode(Disabled); return false; }
    }
    
    private:
    std::size_t UpdateHovered();  // returns ID of hoveredCell
    auto StoreCell(Cell* const cellptr);  // saves the cell's current state, returns an iterator
    void ModifyCell(const std::size_t cellID); // modifies cell's properties based on mode
    void RestoreCell(const std::size_t cellID); // restores original state and removes entry for cell
};

#endif
