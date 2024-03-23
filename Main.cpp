#include <iostream>
#include <cassert>

//#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>  // defines sf::Event

#include "Fluid.hpp"
#include "ValarrayTest.hpp"
#include "Diffusion.hpp"
#include "Mouse.hpp"
#include "Gradient.hpp"

// inspired by Sebastian Lague


// TODO: handle window resizing
// TODO: imgui


void EmbedMacroTest() 
{
    std::cout << "\n\n";
    // Normal include (defines GradientRaw)
    #include "GradientRaw.cpp"
    const auto& rawNormal = GradientRaw;
    std::cout << int(GradientRaw[1][2]) << '\n';  // defined by include
    std::cout << int(rawNormal[1][2]) << '\n';
    // Note that you have to (explicitly) convert to int if you want to print the numbers
    std::cout << "\n\n";
    
    // Embed include
    constexpr unsigned char rawEmbed[1024][3] = {
        #define EMBED_GRADIENT
        #include "GradientRaw.cpp"
    };
    std::cout << int(rawEmbed[1][2]) << '\n';
    std::cout << "\n\n";
    
    // unfortunately, range-for-loops refuse to deduce the types, 
    // so we have to macro the 'triplet' definition onto every line.
    // if you use unsigned char for triplet instead, you'll have to explicity convert to int later
    using triplet = std::array<unsigned int, 3>;
    using gradientArrT = std::array<triplet, 1024>;
    constexpr gradientArrT epic {
        #define EMBED_TYPECAST triplet  // required for conversion to std::arrays
        #define EMBED_GRADIENT
        #include "GradientRaw.cpp"
    };
    for (auto [r,g,b]: epic)
    {
        std::cout << r << ' ' << g << ' ' << b << '\n';
        // explicit conversion required if unsigned char
        // std::cout << int(r) << ' ' << int(g) << ' ' << int(b) << '\n';
    }
    std::cout << "\n\n";
    return;
}


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
    Mouse_T mouse(fluid.GetCellMatrixPtr());
    

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
                        case sf::Keyboard::F1:
                            if (gradientWindow.isOpen()) { gradientWindow.close(); }
                            else
                            {
                                gradientWindow.Create();
                                gradientWindow.FrameLoop();
                            }
                            break;
                        
                        //case sf::Keyboard::
                        default:
                            break;
                    }
                    break;
                }
                // TODO: refactor into Mouse_T member function
                case sf::Event::MouseMoved:
                {
                    const auto [winsizeX, winsizeY] = mainwindow.getSize();
                    const auto [mouseX, mouseY] = event.mouseMove;
                    if ((mouseX < 0) || (mouseY < 0)) {
                        mouse.insideWindow = false;
                        //mouse.hoveredCell = nullptr;
                        break;
                    }
                    else if ((u_int(mouseX) < winsizeX) && (u_int(mouseY) < winsizeY)) {
                        if ((mouseX >= BOXWIDTH) || (mouseY >= BOXHEIGHT)) { // TODO: handle window resizes so we don't crash
                            mouse.insideWindow = false;
                            //mouse.hoveredCell = nullptr;
                            break;
                        }
                        mouse.insideWindow = true;
                        mouse.UpdatePosition(mouseX, mouseY);
                        break;
                    }
                    else mouse.insideWindow = false;
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
