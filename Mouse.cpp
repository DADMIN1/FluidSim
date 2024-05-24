#include "Mouse.hpp"

#include <iostream>
#include <unordered_map>
#include <cassert>

#include <SFML/Graphics/RectangleShape.hpp>
// TODO: refactor this out of Mouse.cpp/hpp completely; it can go in main.
sf::RectangleShape hoverOutline{sf::Vector2f{SPATIAL_RESOLUTION, SPATIAL_RESOLUTION}};

static std::vector<sf::Vector2f> outlined{};
void Mouse_T::DrawOutlines() const
{
    sf::RectangleShape outline{sf::Vector2f{SPATIAL_RESOLUTION, SPATIAL_RESOLUTION}};
    // outline.setFillColor(sf::Color::Transparent);
    outline.setFillColor({0x00, 0x00, 0x00, 0x42});
    outline.setOutlineColor({0x00, 0x99, 0x9A, 0xCD});
    outline.setOutlineThickness(SPATIAL_RESOLUTION/-5.0);

    for (sf::Vector2f& position: outlined) {
        outline.setPosition(position);
        window.draw(outline);
    }
}

// TODO: refactor this elsewhere
bool shouldDrawGrid {false};
bool ToggleGridDisplay() { shouldDrawGrid = !shouldDrawGrid; return shouldDrawGrid; }


struct CellState_T
{
    Cell* const cellptr;
    const Cell originalState;  // needed to restore the cell after moving or releasing-button
    // saving the whole cell is overkill; only the modified/necessary components should be copied
    
    struct Mod_T // stores modifications applied by the mouse-mode
    {
        float density {0.0};
        // sf::Color outlineColor;
        // sf::Color fillColor;  // problem: Cell.UpdateColor will overwrite fillColor every frame
        // sf::RectangleShape overlay;
    } mod;
    
    CellState_T(Cell* const ptr): cellptr{ptr}, originalState{*ptr}
    {
        // TODO: initialize mod here or in ModifyCell?
        // example of pointer-to-member
        /* float Cell::* mcptr = &Cell::density;
        const float& epicmem = cellptr->*mcptr; */
    }
};

// TODO: reserve based on neighbor counts?
static std::unordered_map<std::size_t, CellState_T> savedState {};


auto Mouse_T::StoreCell(Cell* const cellptr)
{
    const std::size_t ID = cellptr->UUID;
    //savedState[cellptr->UUID] = CellState_T(cellptr); // can't do because some members are const
    if (savedState.contains(ID))
    {
        std::cerr << "savedState already contains ID: " << ID;
    }
    const auto result = savedState.emplace(ID, cellptr);
    assert((result.first->first == ID) && "inserted IDs don't match!!");
    assert(result.second && "Cell-state was not saved! (entry already exists)");
    // std::cout << "stored: " << ID << '\n';
    return result.first;
}


// modifies properties based on mouse's current mode
void Mouse_T::ModifyCell(const Cell* const cellptr)
{
    switch(mode)
    { 
        case None:
            if (!hoveredCell) { return; }
            hoverOutline.setPosition(hoveredCell->getPosition());
            shouldOutline = true;
        break;
        
        case Pull:
        case Push:
        {
            CellState_T& state = savedState.at(cellptr->UUID);
            state.mod.density = ((mode==Push)? strength : -strength);
            state.cellptr->density += state.mod.density;
            hoverOutline.setPosition(hoveredCell->getPosition());
            if (isPaintingMode)
            {
                hoverOutline.setFillColor({0x1A, 0xFF, 0x1A, 0x82});
                outlined.clear();
                outlined.push_back(hoveredCell->getPosition());
            }
            for (int dist{1}; dist <= radialDist; ++dist)
            {
                const float adjStrength = {((mode==Push)? strength : -strength)*DIFFUSIONSCALING[dist]};
                const std::vector<Cell*> adjacent {fieldptr->GetCellNeighbors(cellptr->UUID, dist)};
                for (Cell* const cellptr: adjacent)
                {
                    if (savedState.contains(cellptr->UUID)) {
                        RestoreCell(cellptr->UUID);
                    }
                    auto entry = StoreCell(cellptr);
                    entry->second.mod.density = adjStrength;
                    cellptr->density += adjStrength;
                    outlined.push_back(cellptr->getPosition());
                }
            }
        }
        break;
        
        case Drag: case Fill: case Erase: 
        case Disabled:
        default:
            assert(false && "modifycell called with unexpected mode");
            break;
    }
    return;
}

// restores original state and removes entry from map
void Mouse_T::RestoreCell(const std::size_t cellID)
{
    // CellState_T& state = shouldRemove? savedState.extract(cellID).mapped() : savedState.at(cellID);
    const CellState_T state = savedState.extract(cellID).mapped();
    //state.cellptr->density -= state.mod.density;  // will be overwitten by originalState anyway
    // colors/outlines are restored by the read from originalState
    
    // accounting for any external changes made to density (not from Mouse) since it was stored
    const float densityAdjustment = (state.cellptr->density - state.mod.density) - state.originalState.density;
    *state.cellptr = state.originalState;
    state.cellptr->density += densityAdjustment;
    // std::cout << "Restored: " << cellID << '\n';
    return;
}

void Mouse_T::InvalidateHover()
{
    std::vector<std::size_t> ids{};
    ids.reserve(savedState.size());
    for (auto iter{savedState.cbegin()}; iter != savedState.cend(); ++iter)
    {
        // these iterators get invalidated from extraction, it seems.
        //const auto node = savedState.extract(iter);
        //const CellState_T& state = node.mapped();
        ids.push_back(iter->first);
    }
    for (const auto id: ids)
    {
        const CellState_T state = savedState.extract(id).mapped();
        const float densityAdjustment = (state.cellptr->density - state.mod.density) - state.originalState.density;
        *state.cellptr = state.originalState;
        state.cellptr->density += densityAdjustment;
    }
    
    shouldDisplay = false;
    shouldOutline = false;
    hoveredCell = nullptr;
    outlined.clear();
    return;
}


void Mouse_T::SwitchMode(const Mode nextmode)
{
    if (mode == nextmode) { return; }
    // handling transition from current mode
    // InvalidateHover();
    if ((mode == None) || (mode == Disabled)) {
        // these modes shouldn't store any mods, so no need to restore hoveredCell
        shouldDisplay = false;
    } else {
        // we need to restore hoveredCell, otherwise it won't revert on mouse-release
        // because ModifyCell doesn't undo existing mods, and None-mode doesn't handle mods at all
        // also we'll lose information about which mode the mods were set with (the current one)
        InvalidateHover();
    }
    
    mode = nextmode;
    switch(nextmode)
    {
        case None:
        {
            hoverOutline.setFillColor(sf::Color::Transparent);
            // just drawing hoveredCell's outline
            if (UpdateHovered()) {
                hoverOutline.setPosition(hoveredCell->getPosition());
                shouldOutline = true;
                shouldDisplay = false;
            }
        }
        break;
        
        case Push:
        case Pull:
        {
            if(!UpdateHovered()) { return; }  // invalidate hover?
            ModifyCell(hoveredCell);
            shouldDisplay = true;
            shouldOutline = false;
            if (isPaintingMode) { shouldOutline = true; }  // required for outlined
        }
        break;
        
        /* case Drag: case Fill: case Erase:
        break; */
        
        case Disabled:
        default:
            InvalidateHover();
            shouldOutline = false;
        break;
    }
    return;
}


bool Mouse_T::UpdateHovered()
{
    if (!isInsideWindow()) { return false; }
    const auto [x, y] = sf::Mouse::getPosition(window);
    setPosition(x, y); // moving the sf::CircleShape
    if (hoveredCell) // checking if we're still hovering the same cell
    {
        if (hoveredCell->getGlobalBounds().contains(x, y))
        { // still inside oldcell
            if (!savedState.contains(hoveredCell->UUID))
                StoreCell(hoveredCell);
            return true;
        }
        else 
        { // restore original state to previous cell (before hoveredCell is updated)
            if (isPaintingMode) {
                /* if (savedState.contains(hoveredCell->UUID))
                    RestoreCell(hoveredCell->UUID);  // removes from map */
                // don't remove it from the map; it'll get restored and re-inserted during ModifyCell
                // if you remove it now, you'll get holes (and it won't work at all if radialDist is 0)
                hoveredCell = nullptr;
            }
            else {
                const bool sd {shouldDisplay};
                const bool so {shouldOutline};
                InvalidateHover();
                shouldDisplay = sd;
                shouldOutline = so;
            }
        }
    }
    
    // finding indecies for new hoveredCell  // TODO: do this better
    unsigned int xi = x/SPATIAL_RESOLUTION;
    unsigned int yi = y/SPATIAL_RESOLUTION;
    if ((xi > Cell::maxIX) || (yi > Cell::maxIY)) {
        std::cerr << "bad indecies calculated: ";
        std::cerr << xi << ", " << yi << '\n';
        return false;
    }
    assert((xi <= Cell::maxIX) && (yi <= Cell::maxIY) && "index of hovered-cell out of range");
    
    hoveredCell = fieldptr->cellmatrix.at(xi).at(yi);
    if (savedState.contains(hoveredCell->UUID))
        RestoreCell(hoveredCell->UUID);
    StoreCell(hoveredCell);
    return true;
}


void Mouse_T::HandleEvent(const sf::Event& event)
{
    if (mode == Disabled) { return; }
    
    switch(event.type)
    {
        // not currently passed by mainwindow
        case sf::Event::MouseLeft:  // mouse left the window
            std::cerr << "mouseleft!\n";
            InvalidateHover();
        break;
        
        case sf::Event::MouseWheelScrolled:
        {
            const int oldRD = radialDist;
            ChangeRadius((event.mouseWheelScroll.delta >= 0));
            if ((mode != None) && (radialDist != oldRD) && (!isPaintingMode))
            {
                InvalidateHover();  // without this, it will grow, but not shrink
                UpdateHovered();    // only necessary because of InvalidateHover
                // unfortunately, InvalidateHover wipes the whole board when painting-mode is enabled
                
                // almost works with just this and ModifyCell; but it only grows; doesn't shrink
                // and it doesn't wipe the board in painting-mode
                /* if (savedState.contains(hoveredCell->UUID))
                    RestoreCell(hoveredCell->UUID);
                StoreCell(hoveredCell); */
                
                ModifyCell(hoveredCell); // redraws
                shouldDisplay = true; // only necessary because of InvalidateHover
            }
        }
        break;
        
        case sf::Event::MouseMoved:
        {
            if (!isInsideWindow()) { return; }
            
            const Cell* const oldptr = hoveredCell;
            /* if (!UpdateHovered()) {
                const bool sd {shouldDisplay};
                const bool so {shouldOutline};
                InvalidateHover();
                shouldDisplay = sd;
                shouldOutline = so;
            } */
            if (!UpdateHovered()) {
                shouldDisplay = false;
                shouldOutline = false;
                //InvalidateHover();
                return;
            }
            
            // only update if ptr changed
            if (hoveredCell != oldptr) { 
                ModifyCell(hoveredCell);
            }  
            //if ((hoveredCell != oldptr) && (hoveredCell) && (oldptr)) { ModifyCell(id); }  // only update if ptr changed
            //if (!hoveredCell) { InvalidateHover(); }
            
        }
        break;
        
        case sf::Event::MouseButtonPressed:
        {
            if (!isInsideWindow()) { return; }
            // std::cout << "mousebutton: " << event.mouseButton.button << '\n';
            switch (event.mouseButton.button)
            {
                case 0: //Left-click
                {
                    if (mode != None) { break; } // only allow one button at a time
                    SwitchMode(Push);
                    setOutlineColor(sf::Color::Cyan);
                    if (!shouldDrawGrid)
                        setFillColor({0x20, 0x77, 0x77, 0x64});
                }
                break;
                
                case 1: //Right-click
                {
                    if (mode != None) { break; } // only allow one button at a time
                    SwitchMode(Pull);
                    setOutlineColor(sf::Color::Magenta);
                    if (!shouldDrawGrid)
                        setFillColor({0xFF, 0x00, 0x77, 0x32});
                }
                break;
                
                default:
                break;
            }
        }
        break;
        
        case sf::Event::MouseButtonReleased:
        {
            // these need to be handled even if they occur outside of the window-area
            switch (event.mouseButton.button)
            {
                case 0: //Left-click
                    // handling the case where two mouse-buttons are held;
                    // the second press switches the mode, so releases do nothing if the mode doesn't match
                    if (mode == Push) { SwitchMode(None); setFillColor(sf::Color::Transparent); }
                    else { break; }
                break;
                
                case 1: //Right-click
                    if (mode == Pull) { SwitchMode(None); setFillColor(sf::Color::Transparent); }
                    else { break; }
                break;
                
                default:
                break;
            }
        }
        break;
        
        default:
        break;
    }
    
    return;
}
