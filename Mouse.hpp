#ifndef FLUIDSIM_MOUSE_HPP_INCLUDED
#define FLUIDSIM_MOUSE_HPP_INCLUDED

#include <unordered_map>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Window.hpp>  // defines mouse and window

#include "Globals.hpp"
#include "Diffusion.hpp"


static constexpr int defaultRD{2};
static constexpr float defaultRadius{(defaultRD + 0.65) * 0.65 * SPATIAL_RESOLUTION}; // radius of the circleshape, specifically
static constexpr int defaultPointCount{128};  // larger numbers don't seem to do anything

struct CellState_T;

// TODO: multi-layered, stacked overlays
// Provides mouse-related interactions for the simulation
class Mouse_T: private sf::Mouse, private sf::CircleShape
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
    
    static const auto ModeToString(const Mode mode) {
        #define CASE(name) case Mode::name: return #name;
        switch(mode)
        {
            CASE(Disabled) CASE(None) CASE(Push) CASE(Pull) CASE(Drag) CASE(Fill) CASE(Erase)
            default: return "unreachable";
        }
        #undef CASE
    }
    // non-static version using current mode
    const auto ModeToString() const { return ModeToString(this->mode); }
    
    
    sf::RenderWindow& window; // TODO: refactor this out
    float strength {64.0};  // for push/pull modes
    int radialDist {defaultRD};  // orthogonal distance of adjacent cells included in effect
    
    DiffusionField* fieldptr; // &fluid.DiffusionField
    Cell* hoveredCell {nullptr};
    sf::RenderTexture cellOverlay;    // outlines of the cells around the cursor in painting-mode
    sf::RenderTexture outlineOverlay; // for color-coding the active/locked cells in painting mode
    static std::unordered_map<std::size_t, CellState_T> savedState;      // unlocked painted cells
    static std::unordered_map<std::size_t, CellState_T> preservedOverlays; // locked painted cells
    
    //static void sf::Mouse::setPosition(const sf::Vector2i& position);
    //static void sf::Mouse::setPosition(const Vector2i& position, const Window& relativeTo);
    using sf::CircleShape::setPosition;  // disambiguates the call, preventing (unqualified) calls to sf::Mouse::setPosition()
    // is it actually necessary to even inherit from sf::Mouse?
    
    public:
    friend int main(int, char**);
    friend class  MainGUI;
    friend struct MouseParameters;
    bool shouldDisplay{false}; // controls drawing of the mouse (circle)
    bool shouldOutline{false}; // hoveredCell-outline
    bool isPaintingMode{false}; // mouse-interactions stay painted over traveled areas
    static bool isPaintingDebug; // color-codes all cells in the overlay (always) by which container they're in
    // TODO: Grid-display should be auto-enabled when this is toggled!!!
    
    void RedrawOutlines();  // for painting mode, outlines every cell around mouse
    void RedrawOverlay(); // also for painting mode; overlay of 
    void ClearPreservedOverlays()  const; // for painting-mode
    void RecalculateModDensities() const; // updates preservedOverlays' mod.densities (when strength changes)
    
    void HandleEvent(const sf::Event&);
    
    // Do not call the constructor for sf::Mouse (it's virtual)?
    Mouse_T(sf::RenderWindow& theWindow, DiffusionField* const mptr)
        : sf::CircleShape(defaultRadius, defaultPointCount), window{theWindow}, fieldptr{mptr}
    {
        setOutlineThickness(((defaultRD == 0) ? 1.0 : defaultRD));
        setOrigin(getRadius(), getRadius());
        setOutlineColor(sf::Color::Cyan);
        setFillColor(sf::Color::Transparent);
        auto [w, h] = theWindow.getSize();
        cellOverlay.create(w, h);
        outlineOverlay.create(w, h);
    }
    
    auto GetOverlaySprites() { 
        return std::pair<sf::Sprite,sf::Sprite> (
            sf::Sprite{    cellOverlay.getTexture() },
            sf::Sprite{ outlineOverlay.getTexture() }
        );
    }
    
    private:
    void InvalidateHover(bool preserve=false); // restores current hoveredCell and all modified cells
    void SwitchMode(const Mode);
    
    // checks if mouse is enabled, optionally checks if any effect is being applied (hasEffect=true)
    bool isActive(const bool hasEffect=false) const { return (mode != Disabled) && (hasEffect? (mode != None) : true); }
    // swaps the current mode with the saved mode; returns true unless the new mode is 'Disabled'
    bool ToggleActive() {
        static Mode prev {None};
        if (mode == Disabled) { SwitchMode(prev); prev = Disabled; return true;  }
        else                  { prev = mode; SwitchMode(Disabled); return false; }
    }
    
    bool isInsideWindow() const
    {
        const auto [winsizeX, winsizeY] = window.getSize();
        const auto [mouseX, mouseY] = sf::Mouse::getPosition(window);
        const bool insideWindow {
            (mouseX >= 0) && (mouseY >= 0) && 
            (u_int(mouseX) <= winsizeX) && (u_int(mouseY) <= winsizeY)
        };
        return insideWindow;
    }
    
    void ChangeRadius(const bool increase) {
        shouldDisplay = true; // before early-returns; otherwise it doesn't show at limits
        radialDist += (increase? 1 : -1);
        if (radialDist > int(radialdist_limit)) { radialDist = radialdist_limit; return; }
        else if (radialDist < 0)                { radialDist = 0;                return; }
        // since adjustments are made incrementally, the previous value of any illegal change must be the limit
        // therefore, the state will not change; hence the early returns.

        setOutlineThickness(((radialDist == 0) ? 1.0 : radialDist));
        setRadius((radialDist+0.65) * 0.65 * SPATIAL_RESOLUTION);
        setOrigin(getRadius(), getRadius());
        // window.draw() ? need to display the new size even if 'shouldDisplay' is false
        // implement some kind of timer to temporarily reveal the radius
    }
    
    // TODO: should these even be member functions?
    bool UpdateHovered();  // returns true if hoveredCell was found
    auto StoreCell(Cell* const cellptr);  // saves the cell's current state, returns an iterator
    void ModifyCell(const Cell* const cellptr); // modifies cell's properties based on mode
    void RestoreCell(const std::size_t cellID); // restores original state and removes entry for cell
    void Reset();
};


// dynamic thickness/transparency
/* setOutlineThickness(-1 * (radialDist * radialDist));
sf::Color asdf{getOutlineColor()};
asdf.a = 0xFF - (0x1A * radialDist);
setOutlineColor(asdf); */


#endif
