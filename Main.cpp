#include <iostream>
#include <cassert>

//#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>  // defines sf::Event

#include "Fluid.hpp"
#include "ValarrayTest.hpp"
#include "Diffusion.hpp"


// inspired by Sebastian Lague

bool isPaused{false};
bool TogglePause()
{
    isPaused = !isPaused;
    return isPaused;
}

struct Mouse_T: public sf::CircleShape {
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
    bool insideWindow{false};
    
    Mouse_T(): sf::CircleShape(defaultRadius)
    {
        setOutlineThickness(3.f);
        setOutlineColor(sf::Color::White);
        setFillColor(sf::Color::Transparent);
        setOrigin(defaultRadius, defaultRadius);
    }
    
    // modifies the properties like color/outline, and density?
    /* void ModifyCell(DiffusionField_T::Cell* cell) {
        return;
    }
    
    // restores original state (undoes the changes made by ModifyCell)
    void RestoreCell(DiffusionField_T::Cell* cell) {
        return;
    } */
    
    void UpdatePosition(int x, int y)
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
    
} mouse;

// TODO: handle window resizing
// TODO: imgui

int main(int argc, char** argv)
{
    std::cout << "fluid sim\n";
    //assert(false && "asserts are active!");
    assert((timestepRatio > 0) && "timestep-ratio is zero!");
    assert(sleepDelay.asMicroseconds() > 0 && "sleepDelay is zero or negative!");
    std::cout << "sleepdelay = " << sleepDelay.asMilliseconds() << "ms\n"; 
    std::cout << "(" << sleepDelay.asMicroseconds() << ") us\n";
    
    for (int C{0}; C < argc; ++C) {
        std::string arg {argv[C]};
        std::cout << "C: " << C << " \t arg: " << arg << '\n';
    }
    
    /* ValarrayTest();
    ValarrayExample(); */
    
    /* int targetCount = 5;
    for (int s{1}; s <= targetCount; ++s) {
        std::cout << "\n\n\n BaseNCount(" << s << "):\n";
        int total = CalcBaseNCount(s);
        std::cout << "\ntotal = " << total << "\n";
    } */
    
    std::cout << "max indecies: " 
        << DiffusionField_T::maxIX << ", " 
        << DiffusionField_T::maxIY << '\n';
    
    sf::RenderWindow mainwindow (sf::VideoMode(BOXWIDTH, BOXHEIGHT), "FLUIDSIM");
    Fluid fluid;
    if (!fluid.Initialize())
    {
        std::cout << "fluid initialization failed!!";
        return 1;
    }
    mouse.matrixptr = fluid.GetCellMatrixPtr();

    #if DYNAMICFRAMEDELAY
    sf::Clock frametimer{};
    #endif
    // frameloop
    while (mainwindow.isOpen())
    {
        #if DYNAMICFRAMEDELAY
        frametimer.restart();
        #endif
        sf::Event event;
        while (mainwindow.pollEvent(event))
        {
            switch(event.type) 
            {
                case (sf::Event::Closed):
                    mainwindow.close();
                    break;
                case (sf::Event::KeyPressed):
                {
                    switch (event.key.code) 
                    {
                        case sf::Keyboard::Q:
                            mainwindow.close();
                            break;
                        case sf::Keyboard::G: 
                        {
                            bool g = fluid.ToggleGravity();
                            std::cout << "gravity " << (g?"enabled":"disabled") << '\n';
                            break;
                        }
                        case sf::Keyboard::Space:
                            std::cout << (TogglePause()?"paused":"unpaused") << '\n';
                            break;
                        case sf::Keyboard::BackSpace:
                            if (!isPaused) TogglePause();
                            fluid.Freeze();
                            std::cout << "Velocities have been zeroed (and gravity disabled)\n";
                            break;
                        
                        //case sf::Keyboard::
                        default:
                            break;
                    }
                    break;
                }
                case (sf::Event::MouseMoved):
                {
                    const auto [winsizeX, winsizeY] = mainwindow.getSize();
                    const auto [mouseX, mouseY] = event.mouseMove;
                    if ((mouseX < 0) || (mouseY < 0)) {
                        mouse.insideWindow = false;
                        mouse.hoveredCell = nullptr;
                        break;
                    }
                    else if ((u_int(mouseX) < winsizeX) && (u_int(mouseY) < winsizeY)) {
                        if ((mouseX >= BOXWIDTH) || (mouseY >= BOXHEIGHT)) { // TODO: handle window resizes so we don't crash
                            mouse.insideWindow = false;
                            mouse.hoveredCell = nullptr;
                            break;
                        }
                        mouse.insideWindow = true;
                        mouse.UpdatePosition(mouseX, mouseY);
                        break;
                    }
                    else mouse.insideWindow = false;
                    break;
                }
                /* case (sf::Event::MouseButtonPressed): 
                {
                    switch (event.mouseButton.button)
                    {
                        case (0): //Left-click
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
        }
        
        // marked unlikely because to optimize for the unpaused state
        if (isPaused) [[unlikely]] { continue; }

        mainwindow.clear();

        fluid.Update();
        fluid.UpdateDensities();
        fluid.ApplyDiffusion();
        mainwindow.draw(fluid.DrawGrid());
        mainwindow.draw(fluid.Draw());
        
        if (mouse.insideWindow) {
            mainwindow.draw(mouse);
        }
        
        mainwindow.display();

        // framerate cap
        #if DYNAMICFRAMEDELAY
        const sf::Time adjustedDelay = sleepDelay - frametimer.getElapsedTime();
        if (adjustedDelay == sf::Time::Zero) [[unlikely]] {
            //std::cout << "adjusted-delay is negative!: " << adjustedDelay.asMicroseconds() << "us" << '\n';
            std::cout << "adjusted-delay is negative!: " << adjustedDelay.asMilliseconds() << "ms" << '\n';
            continue;
        }
        sf::sleep(adjustedDelay);
        #else // using hardcoded delay-compensation
        sf::sleep(sleepDelay);
        #endif
        
    }
    
    return 0;
}
