#include "Mouse.hpp"

#include <iostream>


void Mouse_T::UpdateHovered(const int x, const int y)
{
    setPosition(x, y); // moving the sf::CircleShape
    if (!matrixptr) {
        std::cerr << "invalid matrixptr, can't search for cell\n";
        return;
    }
    if (hoveredCell) {
        //auto& [minX, minY] = hoveredCell->getPosition();
        if (hoveredCell->getGlobalBounds().contains(x, y)) // TODO: global or local bounds?
            return; // still inside oldcell
        else { // restore original state to previous cell (before hoveredCell is updated)
            float densityAdjustment = hoveredCell->density - savedState.density;
            *hoveredCell = savedState;
            hoveredCell->density += densityAdjustment;
        }
    }
    unsigned int xi = x/SPATIAL_RESOLUTION;
    unsigned int yi = y/SPATIAL_RESOLUTION;
    if ((xi > DiffusionField_T::maxIX) || (yi > DiffusionField_T::maxIY)) {
        std::cerr << "bad indecies calculated: ";
        std::cerr << xi << ", " << yi << '\n';
        return;
    }
    assert((xi <= DiffusionField_T::maxIX) && (yi <= DiffusionField_T::maxIY) && "index of hovered-cell out of range");
    //DiffusionField_T::Cell* cell = matrixptr->at(xi).at(yi); // TODO: do this better
    hoveredCell = matrixptr->at(xi).at(yi);  // this crashes if the window has been resized
    savedState = *hoveredCell;
    if (hoveredCell->getGlobalBounds().contains(x, y)) {
        hoveredCell->setOutlineColor(sf::Color::Cyan);
        hoveredCell->setOutlineThickness(2.5f);
    }
    else {
        mode = None;
        hoveredCell = nullptr;
        insideWindow = false;
    }
    return;
}

void Mouse_T::HandleEvent(sf::Event event)
{
    switch(event.type)
    {
        case sf::Event::MouseMoved:
        {
            const auto [winsizeX, winsizeY] = window.getSize();
            const auto [mouseX, mouseY] = event.mouseMove;
            if ((mouseX < 0) || (mouseY < 0)) {
                insideWindow = false;
                hoveredCell = nullptr;
                break;
            }
            else if ((u_int(mouseX) < winsizeX) && (u_int(mouseY) < winsizeY)) {
                if ((mouseX >= BOXWIDTH) || (mouseY >= BOXHEIGHT)) {
                    insideWindow = false;
                    hoveredCell = nullptr;
                    break;
                }
                insideWindow = true;
                UpdateHovered(mouseX, mouseY);
                break;
            }
            else insideWindow = false;
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
