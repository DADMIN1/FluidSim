#include "Mouse.hpp"

#include <iostream>


void Mouse_T::UpdatePosition(const int x, const int y)
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
        // TODO: restore original density
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
