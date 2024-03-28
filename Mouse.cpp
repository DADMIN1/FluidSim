#include "Mouse.hpp"

#include <iostream>
#include <unordered_map>


struct CellState_T
{
    using Cell = DiffusionField_T::Cell;
    using CellMatrix = DiffusionField_T::CellMatrix;
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


std::unordered_map<std::size_t, CellState_T> savedState {};


std::size_t Mouse_T::StoreCell(Cell* const cellptr)
{
    const std::size_t ID = cellptr->UUID;
    //savedState[cellptr->UUID] = CellState_T(cellptr); // can't do because some members are const
    const auto result = savedState.emplace(ID, cellptr);
    assert(result.second && "Cell-state was not saved! (entry already exists)");
    return ID;
}

// modifies properties based on mouse's current mode
void Mouse_T::ModifyCell(const std::size_t cellID)
{
    switch(mode)
    { 
        case None:
        {
            CellState_T& state = savedState.at(cellID);
            state.cellptr->setOutlineColor(sf::Color::Cyan);
            state.cellptr->setOutlineThickness(2.5f);
        }
        break;
        
        case Pull:
        case Push:
        {
            CellState_T& state = savedState.at(cellID);
            state.cellptr->setOutlineColor(sf::Color::Cyan);
            state.cellptr->setOutlineThickness(2.5f);
            state.mod.density = ((mode==Push)? strength : -strength);
            state.cellptr->density += state.mod.density;
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
    return;
}

void Mouse_T::InvalidateHover()
{
    if (hoveredCell && savedState.contains(hoveredCell->UUID))
    {
        // restore all stored cells here?
        RestoreCell(hoveredCell->UUID);
    }
    shouldDisplay = false;
    hoveredCell = nullptr;
    return;
}


void Mouse_T::SwitchMode(Mode nextmode)
{
    if (mode == nextmode) { return; }
    // handling transition from current mode
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
            const auto id = UpdateHovered();
            ModifyCell(id);  // just drawing hoveredCell's outline
        }
        break;
        
        case Push:
        case Pull:
        {
            const auto id = UpdateHovered();
            ModifyCell(id);
            shouldDisplay = true;
        }
        break;
        
        /* case Drag: case Fill: case Erase:
        break; */
        
        case Disabled:
        default:
            InvalidateHover();
        break;
    }
    return;
}


std::size_t Mouse_T::UpdateHovered()
{
    const auto [x, y] = sf::Mouse::getPosition(window);
    setPosition(x, y); // moving the sf::CircleShape
    if (hoveredCell) // checking if we're still hovering the same cell
    {
        //auto& [minX, minY] = hoveredCell->getPosition();
        if (hoveredCell->getGlobalBounds().contains(x, y))
        { // still inside oldcell
            if (!savedState.contains(hoveredCell->UUID))
                StoreCell(hoveredCell);
            return hoveredCell->UUID;
        }
        else 
        { // restore original state to previous cell (before hoveredCell is updated)
            if (savedState.contains(hoveredCell->UUID))
                RestoreCell(hoveredCell->UUID);  // removes from map
            hoveredCell = nullptr;
        }
    }
    
    // finding indecies for new hoveredCell  // TODO: do this better
    unsigned int xi = x/SPATIAL_RESOLUTION;
    unsigned int yi = y/SPATIAL_RESOLUTION;
    /* if ((xi > DiffusionField_T::maxIX) || (yi > DiffusionField_T::maxIY)) {
        std::cerr << "bad indecies calculated: ";
        std::cerr << xi << ", " << yi << '\n';
        return;
    } */
    assert((xi <= DiffusionField_T::maxIX) && (yi <= DiffusionField_T::maxIY) && "index of hovered-cell out of range");
    
    hoveredCell = matrixptr->at(xi).at(yi);
    const auto ID = StoreCell(hoveredCell);
    //ModifyCell(ID);
    return ID;
}

void Mouse_T::HandleEvent(sf::Event event)
{
    if (mode == Disabled) { return; }
    
    switch(event.type)
    {
        case sf::Event::MouseLeft:  // mouse left the window
            InvalidateHover();
        break;
        
        /* case sf::Event::MouseEntered:  // no special handling required
            break; */ // can't fallthrough because it has no data, unlike MouseMoved
        case sf::Event::MouseMoved:
        {
            const auto [winsizeX, winsizeY] = window.getSize();
            //const auto [mouseX, mouseY] = event.mouseMove;
            const auto [mouseX, mouseY] = sf::Mouse::getPosition(window);
            const bool insideWindow {
                (mouseX >= 0) && (mouseY >= 0) && 
                (u_int(mouseX) < winsizeX) && (u_int(mouseY) < winsizeY)
            };
            
            if (insideWindow) {
                const Cell* const oldptr = hoveredCell;
                const auto id = UpdateHovered();
                if (hoveredCell != oldptr) { ModifyCell(id); }  // only update if ptr changed
                //if (!hoveredCell) { InvalidateHover(); }
                /* for (const auto& [i, state]: savedState) {
                    ;
                } */
            } else {
                InvalidateHover();
            }
        }
        break;
        
        case sf::Event::MouseButtonPressed:
        {
            // std::cout << "mousebutton: " << event.mouseButton.button << '\n';
            switch (event.mouseButton.button)
            {
                case 0: //Left-click
                {
                    if (mode != None) { break; } // only allow one button at a time
                    SwitchMode(Push);
                    setOutlineColor(sf::Color::Cyan);
                }
                break;
                
                case 1: //Right-click
                {
                    if (mode != None) { break; } // only allow one button at a time
                    SwitchMode(Pull);
                    setOutlineColor(sf::Color::Magenta);
                }
                break;
                
                default:
                break;
            }
        }
        break;
        
        case sf::Event::MouseButtonReleased:
        {
            switch (event.mouseButton.button)
            {
                case 0: //Left-click
                    // handling the case where two mouse-buttons are held;
                    // the second press switches the mode, so releases do nothing if the mode doesn't match
                    if (mode == Push) { SwitchMode(None); } else { break; }
                break;
                
                case 1: //Right-click
                    if (mode == Pull) { SwitchMode(None); } else { break; }
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
