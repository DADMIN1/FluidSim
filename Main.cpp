#include <iostream>
#include <vector>

#include "Fluid.hpp"

// inspired by Sebastian Lague


int main(int argc, char** argv)
{
    std::cout << "fluid sim\n";
    //assert(false && "asserts are active!");
    assert((timestepRatio > 0) && "timestep-ratio is zero!");
    assert(sleepDelay.asMicroseconds() > 0 && "sleepDelay is zero or negative!");
    std::cout << "sleepdelay = " << sleepDelay.asMilliseconds() << "ms\n"; 
    std::cout << "(" << sleepDelay.asMicroseconds() << ") us\n";

    sf::RenderWindow mainwindow (sf::VideoMode(BOXWIDTH, BOXHEIGHT), "FLUIDSIM");
    Fluid fluid (&mainwindow);

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
                    //case sf::Keyboard::
                    default:
                        break;
                }
            }
        }

        mainwindow.clear();
        fluid.Update();
        fluid.Draw();
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
