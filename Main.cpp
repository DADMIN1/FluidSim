#include <iostream>
#include <vector>

#include "Fluid.hpp"

// inspired by Sebastian Lague

constexpr int framerateCap{60};
constexpr int sleepDelay {1000/framerateCap};

int main(int argc, char** argv)
{
    std::cout << "fluid sim\n";

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
