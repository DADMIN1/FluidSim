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
        float density;
        sf::Color outlineColor;
        //sf::Color fillColor;  // problem: Cell.UpdateColor will overwrite fillColor every frame
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
        case Push: case Pull: case Drag:
            savedState.at(cellID).cellptr->setOutlineColor(sf::Color::Cyan);
            savedState.at(cellID).cellptr->setOutlineThickness(2.5f);
            break;
        
        // these modes don't need to store any state?
        case None: case Fill: case Erase:
        default:
            break;
    }
    return;
}

// restores original state and removes entry from map
void Mouse_T::RestoreCell(const std::size_t cellID)
{
    CellState_T state = savedState.extract(cellID).mapped();
    // accounting for any external changes made to density (not from Mouse) since it was stored
    const float densityAdjustment = state.cellptr->density - state.originalState.density;
    *state.cellptr = state.originalState;
    state.cellptr->density += densityAdjustment;
    return;
}

void Mouse_T::InvalidateHover()
{
    if (hoveredCell)
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
    switch(nextmode)
    {
        case Push: case Pull: case Drag: case Fill: case Erase:
            break;
        
        // these modes don't need to store any state?
        case None:
        default:
            InvalidateHover();
            break;
    }
    mode = nextmode;
    return;
}

void Mouse_T::UpdateHovered(const int x, const int y)
{
    setPosition(x, y); // moving the sf::CircleShape
    /* if (!matrixptr) {
        std::cerr << "invalid matrixptr, can't search for cell\n";
        return;
    } */
    
    if (hoveredCell) // checking if we're still hovering the same cell
    {
        //auto& [minX, minY] = hoveredCell->getPosition();
        if (hoveredCell->getGlobalBounds().contains(x, y)) // TODO: global or local bounds?
            return; // still inside oldcell
        else { // restore original state to previous cell (before hoveredCell is updated)
            RestoreCell(hoveredCell->UUID);
        }
    }
    
    // finding indecies for new hoveredCell  // TODO: do this better
    unsigned int xi = x/SPATIAL_RESOLUTION;
    unsigned int yi = y/SPATIAL_RESOLUTION;
    if ((xi > DiffusionField_T::maxIX) || (yi > DiffusionField_T::maxIY)) {
        std::cerr << "bad indecies calculated: ";
        std::cerr << xi << ", " << yi << '\n';
        return;
    }
    assert((xi <= DiffusionField_T::maxIX) && (yi <= DiffusionField_T::maxIY) && "index of hovered-cell out of range");
    
    hoveredCell = matrixptr->at(xi).at(yi);  // this crashes if the window has been resized
    const auto ID = StoreCell(hoveredCell);
    ModifyCell(ID);
    return;
}

void Mouse_T::HandleEvent(sf::Event event)
{
    if (mode == None) { return; }
    
    switch(event.type)
    {
        case sf::Event::MouseLeft:  // mouse left the window
        {
            InvalidateHover();
            break;
        }
        /* case sf::Event::MouseEntered:  // no special handling required
            break; */ // can't fallthrough because it has no data, unlike MouseMoved
        case sf::Event::MouseMoved:
        {
            const auto [winsizeX, winsizeY] = window.getSize();
            const auto [mouseX, mouseY] = event.mouseMove;
            const bool insideWindow {
                (mouseX >= 0) && (mouseY >= 0) && 
                (u_int(mouseX) < winsizeX) && (u_int(mouseY) < winsizeY)
            };
            
            if (insideWindow) {
                // If the window has been resized, BOXWIDTH/HEIGHT will not match window-dimensions
                if ((mouseX >= BOXWIDTH) || (mouseY >= BOXHEIGHT)) {
                    InvalidateHover();
                    shouldDisplay = false;
                    break;
                }
                shouldDisplay = true;
                UpdateHovered(mouseX, mouseY);
            } else {
                InvalidateHover();
                shouldDisplay = false;
                //mode = None;
            }
            break;
        }
        /* case sf::Event::MouseButtonPressed:
        {
            switch (event.mouseButton.button)
            {
                case 0: //Left-click
                {
                    
                }
                
                default:
                    std::cout << "mousebutton: " << event.mouseButton.button << '\n';
                    break;
            }
            break;
        } */
        
        default: break;
    }
    
    return;
}
