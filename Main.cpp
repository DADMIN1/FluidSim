#include <iostream>
#include <vector>

#include "Fluid.hpp"

// inspired by Sebastian Lague


int main(int argc, char** argv)
{
    std::cout << "fluid sim\n";
    assert((timestepRatio > 0) && "timestep-ratio is zero!");

    sf::RenderWindow mainwindow (sf::VideoMode(BOXWIDTH, BOXHEIGHT), "FLUIDSIM");
    Fluid fluid (&mainwindow);

    // frameloop
    while (mainwindow.isOpen())
    {
        sf::Event event;
        while (mainwindow.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
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
        const sf::Time framedelay (sf::milliseconds(sleepDelay));
        sf::sleep(framedelay);
    }

    
    return 0;
}
