#include <iostream>
#include <cassert>

//#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>  // defines sf::Event

#include "imgui/imgui.h"
#include "imgui/sfml/imgui-SFML.h"

#include "Simulation.hpp"
#include "Mouse.hpp"
#include "Gradient.hpp"
//#include "ValarrayTest.hpp"
#include "Threading.hpp"
#include "Shader.hpp"



// TODO: overlay displaying stats for hovered cell/particles

extern void EmbedMacroTest();  // MacroTest.cpp
extern void PrintKeybinds();   // Keybinds.cpp


float timestepRatio{1.0/float(framerateCap/60.0)};  // normalizing timesteps to make physics independent of frame-rate


// TODO: refactor these elsewhere
// Mouse.cpp
extern bool shouldDrawGrid;
bool ToggleGridDisplay();
extern sf::RectangleShape hoverOutline;
bool windowClearDisabled{false};  // option used by the turbulence shader


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
    mainwindow.setFramerateLimit(300); // TODO: remove manual frame-delay
    
    assert(IMGUI_CHECKVERSION() && "ImGui version-check failed!");
    std::cout << "using imgui v" << IMGUI_VERSION << '\n';
    //ImGui::CreateContext();
    
    if (!ImGui::SFML::Init(mainwindow)) {
        std::cerr << "imgui-sfml failed to init! exiting.\n";
        return 3;
    }
    
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
    
    const std::map<sf::Keyboard::Key, Shader*>& shader_map = Shader::LoadAll();
    const Shader* empty = shader_map.cbegin()->second;
    if (!empty->InvokeSwitch()) { 
        std::cerr << "ragequitting because empty shader didn't load.\n";
        return 2;
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
            ImGui::SFML::ProcessEvent(mainwindow, event);
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
                            std::cout << "Fluid is: " << (simulation.ToggleTurbulence()? "Turbulent":"not turbulent") << '\n';
                        break;
                        
                        case sf::Keyboard::Y:
                            std::cout << "transparency " << (simulation.ToggleTransparency()? "enabled":"disabled") << '\n';
                        break;
                        
                        case sf::Keyboard::U:
                            std::cout << "Particle scaling is: " << (Fluid::ToggleParticleScaling()? "Positive":"Negative") << '\n';
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
                        
                        case sf::Keyboard::Num0:
                        case sf::Keyboard::Num1:
                        case sf::Keyboard::Num2:
                        case sf::Keyboard::Num3:
                        case sf::Keyboard::Num4:
                        case sf::Keyboard::Num5:
                        {
                            std::string previous_name = Shader::current->name;
                            //Shader* selectedShader = shader_map.at(event.key.code);
                            shader_map.at(event.key.code)->InvokeSwitch();
                            // enable transparency and turbulence-mode automatically
                            if (Shader::current->name == "turbulence") {
                                if(!simulation.ToggleTurbulence()) simulation.ToggleTurbulence();
                                if(previous_name == "turbulence") {
                                    windowClearDisabled = !windowClearDisabled;
                                    std::cout << "window clears " << (windowClearDisabled ? "enabled" : "disabled") << "\n";
                                }
                            }
                            // TODO: restore previous settings for turbulence, transparency, and window-clears
                        }
                        break;
                        
                        case sf::Keyboard::Pause:
                            windowClearDisabled = !windowClearDisabled;
                            std::cout << "window clears " << (windowClearDisabled ? "enabled" : "disabled") << "\n";
                        break;
                        
                        case sf::Keyboard::Add:
                        case sf::Keyboard::Subtract:
                        {
                            if (Shader::current->name != "turbulence") {
                                std::cerr << "turbulence is not active\n";
                                break;
                            }
                            float threshold = Shader::current->uniform_vars.at("threshold");
                            threshold += ((event.key.code==sf::Keyboard::Add)? 0.01 : -0.01);
                            if (threshold < 0) threshold = 0.0f;
                            Shader::current->GetWritePtr()->ApplyUniform("threshold", threshold);
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
        
        // imgui needs window and deltatime. You can pass mousePosition and displaySize instead of window
        ImGui::SFML::Update(mainwindow, frametimer.getElapsedTime());
        ImGui::ShowDemoWindow();
        
        // alternate render logic for turbulence shader that skips window-clearing and the grid
        if (Shader::current->name == "turbulence") {
            if (simulation.isPaused || !windowClearDisabled) 
                mainwindow.clear(sf::Color::Transparent); // always clear when paused so you can modify and observe the effects of the 'threshold' parameter
            
            if (mouse.isActive(true) && mouse.isPaintingMode) { 
                mouse.DrawOutlines();
                hoverOutline.setFillColor({0x1A, 0xFF, 0x1A, 0x82});
                mainwindow.draw(hoverOutline); 
            }
            else if (mouse.shouldOutline) { hoverOutline.setFillColor(sf::Color::Transparent); mainwindow.draw(hoverOutline); }
            if (mouse.shouldDisplay) { mainwindow.draw(mouse); }
            
            simulation.Update();
            simulation.RedrawFluid();
            mainwindow.draw(fluidSprite, Shader::current);
            
            // unlike the normal frameloop, here the mouse-outline is drawn even if the mouse is inactive;
            // without it, there's no visual indicator that the mouse is enabled, and no position.
            ImGui::SFML::Render(mainwindow);
            mainwindow.display();
            goto framedelay;
        }
        
        if (!windowClearDisabled)
        mainwindow.clear(sf::Color::Transparent);
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
        mainwindow.draw(fluidSprite, Shader::current);
        
        if (mouse.shouldDisplay) {
            mainwindow.draw(mouse);
        }
        
        ImGui::SFML::Render(mainwindow);
        mainwindow.display();
        
        framedelay:
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
    
    ImGui::SFML::Shutdown();
    
    PrintSpeedcapInfo();
    #ifdef PMEMPTYCOUNTER
    std::cout << "pmemptycounter: " << pmemptycounter << '\n';
    #endif
    return 0;
}
