#include <iostream>
#include <cassert>

//#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>  // defines sf::Event

#include "Simulation.hpp"
#include "Mouse.hpp"
#include "Gradient.hpp"
//#include "ValarrayTest.hpp"


// inspired by Sebastian Lague


// TODO: imgui


extern void EmbedMacroTest();  // MacroTest.cpp
extern void PrintKeybinds();   // Keybinds.cpp

bool isPaused{false};
bool TogglePause() { isPaused = !isPaused; return isPaused;}

// mouse.cpp
extern bool shouldDrawGrid;
bool ToggleGridDisplay();
// TODO: refactor these elsewhere

// Mouse.cpp
extern sf::RectangleShape hoverOutline;


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
    
    
    //EmbedMacroTest();
    GradientWindow_T gradientWindow{};
    
    // Title-bar is implied (for Style::Close)
    constexpr auto mainstyle = sf::Style::Close;  // disabling resizing
    sf::RenderWindow mainwindow (sf::VideoMode(BOXWIDTH, BOXHEIGHT), "FLUIDSIM", mainstyle);
    
    hoverOutline.setFillColor(sf::Color::Transparent);
    hoverOutline.setOutlineColor(sf::Color::Cyan);
    hoverOutline.setOutlineThickness(2.5f);
    
    Simulation simulation{};
    if (!simulation.Initialize()) {
        std::cerr << "simulation failed to initialize!\n";
        return 1;
    }
    
    Mouse_T mouse(mainwindow, simulation.GetDiffusionFieldPtr());
    
    PrintKeybinds();
    
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
                case sf::Event::Closed:
                    mainwindow.close();
                break;
                
                case sf::Event::KeyPressed:
                {
                    switch (event.key.code) 
                    {
                        case sf::Keyboard::Q:
                            mainwindow.close();
                        break;
                        
                        case sf::Keyboard::G:
                            std::cout << "gravity " << (simulation.ToggleGravity()? "enabled":"disabled") << '\n';
                        break;
                        
                        case sf::Keyboard::Space:
                            std::cout << (TogglePause()?"paused":"unpaused") << '\n';
                        break;
                        
                        case sf::Keyboard::BackSpace:
                            if (!isPaused) TogglePause();
                            simulation.Freeze();
                            std::cout << "Velocities have been zeroed (and gravity disabled)\n";
                            goto frameAdvance;
                        break;
                        
                        case sf::Keyboard::Tab:
                        {
                            const bool isActive = mouse.ToggleActive();
                            std::cout << "Mouse is " << (isActive? "enabled":"disabled") << '\n';
                            mainwindow.setMouseCursorVisible(!isActive);  // hide cursor when Mouse_T is displayed
                            // mainwindow.setMouseCursorGrabbed(isActive);  // for some reason this only works the first time???
                            if (isActive) {
                                mainwindow.requestFocus();
                                mainwindow.setMouseCursorGrabbed(true);
                                //std::cout << "grabbed\n";
                            } else {
                                mainwindow.setMouseCursorGrabbed(false);
                                //std::cout << "ungrabbed\n";
                            }
                            /*  For whatever reason, trying to grab the mouse after toggling to false here doesn't work.
                                once it's released the cursor, it can't seem to grab it again UNLESS the window is unfocused/refocused.
                                (even though you can manually set it to true/false any number of times earlier and still get the first grab)
                                It doesn't have to lose focus specifically; clicking the titlebar also works. Toggling the gradient-window is another easy method.
                                In fact, it seems that changing focus ALWAYS triggers the cursor-grab, even if it's supposed to be disabled.
                                Maybe it's just a problem with my window-manager, or the function is only meant to be called once? */
                        }
                        break;
                        
                        case sf::Keyboard::F1:
                            PrintKeybinds();
                        break;
                        
                        case sf::Keyboard::F2:
                            if (gradientWindow.isOpen()) { gradientWindow.close(); }
                            else
                            {
                                gradientWindow.Create();
                                gradientWindow.FrameLoop();
                            }
                        break;
                        
                        case sf::Keyboard::T:
                            std::cout << "transparency " << (simulation.ToggleTransparency()? "enabled":"disabled") << '\n';
                        break;
                        
                        case sf::Keyboard::N:
                        {
                            const auto [mouseX, mouseY] = sf::Mouse::getPosition(mainwindow);
                            std::cout << "Mouse@ [" << mouseX << ", " << mouseY << "] \n";
                        }
                        break;
                        
                        case sf::Keyboard::P:
                        {
                            mouse.isPaintingMode = !mouse.isPaintingMode;
                            std::cout << "Painting mode: " << ((mouse.isPaintingMode)? "enabled": "disabled") << '\n';
                        }
                        break;
                        
                        case sf::Keyboard::C:
                        {
                            ToggleGridDisplay();
                            std::cout << "Grid-display " << ((shouldDrawGrid) ? "enabled" : "disabled") << '\n';
                        }
                        break;
                        
                        // case sf::Keyboard::_:
                        // break;
                        
                        default:
                        break;
                    }
                }
                break;
                
                case sf::Event::MouseMoved:
                //case sf::Event::MouseLeft:
                //case sf::Event::MouseEntered:
                case sf::Event::MouseButtonPressed:
                case sf::Event::MouseButtonReleased:
                case sf::Event::MouseWheelScrolled:
                    mouse.HandleEvent(event);
                break;
                
                case sf::Event::Resized:
                {
                    auto [newwidth, newheight] = event.size;
                    sf::View newview {mainwindow.getView()};
                    //newview.setSize(newwidth, newheight);
                    newview.setViewport({0, 0, float{1000.f/newwidth}, float{1000.f/newheight}});
                    mainwindow.setView(newview);
                    
                    /* auto newviewsize = mainwindow.getView().getSize();
                    auto newport = mainwindow.getView().getViewport();
                    std::cout << "newsize: [" << newwidth << ", " << newheight << "] ";
                    std::cout << "view: [" << newviewsize.x << ", " << newviewsize.y << "] ";
                    std::cout << "viewport: [" << newport.width << ", " << newport.height << "] ";
                    std::cout << '\n'; */
                }
                break;
                
                default:
                break;
            }
        }
        
        // marked unlikely to (hopefully) optimize for the unpaused state
        if (isPaused) [[unlikely]] { continue; }
        
// This jump exists to redraw the screen BEFORE pausing (pause-statement will be hit following frame)
frameAdvance:
        mainwindow.clear();
        
        simulation.Update();
        
        // TODO: hide the gridlines in painting-mode
        // grid must be temporarily displayed when you draw in painting-mode (otherwise the effect would be invisible)
        if (shouldDrawGrid || (mouse.isPaintingMode && mouse.isActive(true))) {
            mainwindow.draw(simulation.DrawGrid());
        }
        else if (mouse.isActive()) // when grid is NOT drawn: always draw mouse-radius and disable cell-outline
        {  // TODO: refactor this logic to not check on every frame
            mouse.shouldDisplay = true;
            mouse.shouldOutline = false;
        }
        
        if (mouse.shouldOutline) { 
            mainwindow.draw(hoverOutline); 
            if (mouse.isPaintingMode && mouse.shouldDisplay) mouse.DrawOutlines();
        }
        
        mainwindow.draw(simulation.DrawFluid());
        
        // TODO: allow mouse to update and be redrawn even while paused
        if (mouse.shouldDisplay) {
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
