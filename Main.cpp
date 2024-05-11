#include <iostream>
#include <cassert>

//#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>  // defines sf::Event

#include "Simulation.hpp"
#include "Mouse.hpp"
#include "Gradient.hpp"
//#include "ValarrayTest.hpp"
#include "Threading.hpp"


// inspired by Sebastian Lague


// TODO: imgui
// TODO: overlay displaying stats for hovered cell/particles

extern void EmbedMacroTest();  // MacroTest.cpp
extern void PrintKeybinds();   // Keybinds.cpp


float timestepRatio{1.0/float(framerateCap/60.0)};  // normalizing timesteps to make physics independent of frame-rate


// TODO: refactor these elsewhere
// Mouse.cpp
extern bool shouldDrawGrid;
bool ToggleGridDisplay();
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
    
    // ValarrayExample();
    // ValarrayTest();
    
    /* int targetCount = 5;
    for (int s{1}; s <= targetCount; ++s) {
        std::cout << "\n\n\n BaseNCount(" << s << "):\n";
        int total = CalcBaseNCount(s);
        std::cout << "\ntotal = " << total << "\n";
    } */
    
    std::cout << "max indecies: " 
        << Cell::maxIX << ", " 
        << Cell::maxIY << '\n';
    
    
    //EmbedMacroTest();
    
    ThreadManager threadManager{};
    threadManager.PrintThreadcount();
    
    // Title-bar is implied (for Style::Close)
    constexpr auto mainstyle = sf::Style::Close;  // disabling resizing
    sf::RenderWindow mainwindow (sf::VideoMode(BOXWIDTH, BOXHEIGHT), "FLUIDSIM", mainstyle);
    mainwindow.setPosition({2600, 0}); // move to right monitor
    
    GradientWindow_T gradientWindow{};
    gradientWindow.setPosition({mainwindow.getPosition().x, 360});
    
    hoverOutline.setFillColor(sf::Color::Transparent);
    hoverOutline.setOutlineColor(sf::Color::Cyan);
    hoverOutline.setOutlineThickness(2.5f);
    
    Simulation simulation{};
    if (!simulation.Initialize()) {
        std::cerr << "simulation failed to initialize!\n";
        return 1;
    }
    auto&& [gridSprite, fluidSprite] = simulation.GetSprites();
    
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
                            std::cout << (simulation.TogglePause()?"paused":"unpaused") << '\n';
                        break;
                        
                        case sf::Keyboard::BackSpace:
                            simulation.Freeze();
                            std::cout << "Velocities have been zeroed\n";
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
                                // pausing here doesn't prevent deltatime from becoming huge?
                                //bool oldPauseState = simulation.SetPause(true);
                                gradientWindow.Create(mainwindow.getPosition().x);
                                gradientWindow.FrameLoop();
                                //simulation.SetPause(oldPauseState);
                                // resetting frametimer prevents deltatime from becoming huge and spazzing out
                                frametimer.restart();
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
                    newview.setViewport({0, 0, float{float(BOXWIDTH)/newwidth}, float{float(BOXHEIGHT)/newheight}});
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
        
        mainwindow.clear();
        
        simulation.Update();
        
        // TODO: hide the gridlines in painting-mode
        // grid must be temporarily displayed when you draw in painting-mode (otherwise the effect would be invisible)
        if (shouldDrawGrid || (mouse.isPaintingMode && mouse.isActive(true))) {
            simulation.RedrawGrid();
            mainwindow.draw(gridSprite);
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
        
        simulation.RedrawFluid();
        mainwindow.draw(fluidSprite);
        
        // TODO: allow mouse to update and be redrawn even while paused
        if (mouse.shouldDisplay) {
            mainwindow.draw(mouse);
        }
        
        mainwindow.display();
        
        // framerate cap
        #if DYNAMICFRAMEDELAY
        const sf::Time adjustedDelay = sleepDelay - frametimer.getElapsedTime();
        if (adjustedDelay == sf::Time::Zero) {
            //std::cout << "adjusted-delay is negative!: " << adjustedDelay.asMicroseconds() << "us" << '\n';
            continue;
        }
        sf::sleep(adjustedDelay);
        timestepRatio = float(frametimer.getElapsedTime().asMicroseconds() / 16666.66667);
        #else // using hardcoded delay-compensation
        sf::sleep(sleepDelay);  // bad
        #endif
        
    }
    
    PrintSpeedcapInfo();
    
    return 0;
}
