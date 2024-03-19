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
            if (event.type == sf::Event::Closed) [[unlikely]]
                mainwindow.close();
            
            else if (event.type == sf::Event::KeyPressed)
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
