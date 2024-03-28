#include <iostream>
#include <cassert>

//#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>  // defines sf::Event

#include "Fluid.hpp"
//#include "ValarrayTest.hpp"
#include "Diffusion.hpp"
#include "Mouse.hpp"
#include "Gradient.hpp"


// inspired by Sebastian Lague


// TODO: handle window resizing
// TODO: imgui


extern void EmbedMacroTest();  // MacroTest.cpp

bool isPaused{false};
bool TogglePause()
{
    isPaused = !isPaused;
    return isPaused;
}


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
    
    sf::RenderWindow mainwindow (sf::VideoMode(BOXWIDTH, BOXHEIGHT), "FLUIDSIM");
    Fluid fluid;
    if (!fluid.Initialize())
    {
        std::cout << "fluid initialization failed!!";
        return 1;
    }
    Mouse_T mouse(mainwindow, fluid.GetCellMatrixPtr());

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
                            std::cout << "gravity " << (fluid.ToggleGravity()? "enabled":"disabled") << '\n';
                            break;
                        
                        case sf::Keyboard::Space:
                            std::cout << (TogglePause()?"paused":"unpaused") << '\n';
                            break;
                        
                        case sf::Keyboard::BackSpace:
                            if (!isPaused) TogglePause();
                            fluid.Freeze();
                            std::cout << "Velocities have been zeroed (and gravity disabled)\n";
                            goto frameAdvance;
                            break;
                        
                        case sf::Keyboard::F1:
                            if (gradientWindow.isOpen()) { gradientWindow.close(); }
                            else
                            {
                                gradientWindow.Create();
                                gradientWindow.FrameLoop();
                            }
                            break;
                        
                        case sf::Keyboard::T:
                            std::cout << "transparency " << (fluid.ToggleTransparency()? "enabled":"disabled") << '\n';
                            break;
                        
                        case sf::Keyboard::M:
                            std::cout << "Mouse is " << (mouse.ToggleActive()? "enabled":"disabled") << '\n';
                            break;
                        
                        //case sf::Keyboard::_:
                        //  break;
                        default:
                            break;
                    }
                    break;
                }
                
                case sf::Event::MouseMoved:
                case sf::Event::MouseLeft:
                case sf::Event::MouseEntered:
                case sf::Event::MouseButtonPressed:
                case sf::Event::MouseButtonReleased:
                    mouse.HandleEvent(event);
                    break;
                
                default: break;
            }
        }
        
        // marked unlikely to (hopefully) optimize for the unpaused state
        if (isPaused) [[unlikely]] { continue; }
    
// This jump exists to redraw the screen BEFORE pausing (pause-statement will be hit following frame)
frameAdvance:
        mainwindow.clear();

        fluid.Update();
        fluid.UpdateDensities();
        fluid.ApplyDiffusion();
        mainwindow.draw(fluid.DrawGrid());
        mainwindow.draw(fluid.Draw());
        
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
