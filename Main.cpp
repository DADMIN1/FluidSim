#include <iostream>
#include <cassert>

//#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>  // defines sf::Event
#include <imgui.h> // required for imgui versionchecks
#include <imgui-SFML.h> // imgui::SFML::Shutdown()

#include "Simulation.hpp"
#include "Mouse.hpp"
#include "GradientWindow.hpp"
//#include "ValarrayTest.hpp"
#include "Threading.hpp"
#include "Shader.hpp"
#include "MainGUI.hpp"


float timestepRatio{1.0f}; // normalizing timesteps to make physics independent of frame-rate
float timestepMultiplier{1.0f};


extern void EmbedMacroTest();  // MacroTest.cpp
extern void PrintKeybinds();   // Keybinds.cpp

// MacroTest.cpp
extern void ModuloTest();
extern void PrintComptimeCoords();

// Cell.cpp
extern void AdjacentCellsTest();

// Mouse.cpp
extern sf::RectangleShape hoverOutline;
bool windowClearDisabled{false};  // option used by the turbulence shader

// for MainGUI.cpp (controlling VSync)
sf::RenderWindow* mainwindowPtr {nullptr};
sf::RenderWindow* gradientWinPtr{nullptr}; // pointing to base-class, not 'GradientWindow*'; otherwise MainGUI would require definition
bool usingVsync {true};

// cell-grid
bool shouldDrawGrid {false};
bool ToggleGridDisplay() { shouldDrawGrid = !shouldDrawGrid; return shouldDrawGrid; }
// TODO: overlay displaying stats for hovered cell/particles


int main(int argc, char** argv)
{
    std::cout << "FLUIDSIM\n";
    //assert(false && "asserts are active!");
    
    for (int C{0}; C < argc; ++C) {
        std::string arg {argv[C]};
        std::cout << "C: " << C << " \t arg: " << arg << '\n';
    }
    
    // ValarrayExample();
    // ValarrayTest();
    
    std::cout << "max indecies: " 
        << Cell::maxIX << ", " 
        << Cell::maxIY << '\n';
    
    //EmbedMacroTest();
    //ModuloTest();
    //PrintComptimeCoords();
    //AdjacentCellsTest();
    
    ThreadManager threadManager{};
    threadManager.PrintThreadcount();
    /* threadManager.ContainerDivTest();
    threadManager.ContainerDivTestMT(); */
    
    // Title-bar is implied (for Style::Close)
    constexpr auto mainstyle = sf::Style::Close;  // disabling resizing
    sf::RenderWindow mainwindow (sf::VideoMode(BOXWIDTH, BOXHEIGHT), "FLUIDSIM", mainstyle);
    mainwindow.setPosition({2600, 0}); // move to right monitor
    // mainwindow.setFramerateLimit(framerateCap);
    mainwindow.setVerticalSyncEnabled(usingVsync);
    
    GradientWindow gradientWindow{};
    if(!gradientWindow.Initialize(mainwindow.getPosition().x)) {
        std::cerr << "GradientWindow initialization failed! exiting.\n";
        return 4;
    }
    //gradientWindow.Create();  // not necessary to create the window, apparently
    gradientWindow.AdjustPosition();
    
     mainwindowPtr = &mainwindow;
    gradientWinPtr = &gradientWindow;
    Fluid::SetActiveGradient(&gradientWindow.MasterGradient);
    
    Simulation simulation{};
    if (!simulation.Initialize()) {
        std::cerr << "simulation failed to initialize! exiting.\n";
        return 1;
    }
    
    Mouse_T mouse(mainwindow, simulation.GetDiffusionFieldPtr());
    auto&& [gridSprite, fluidSprite] = simulation.GetSprites();
    auto [cellOverlay, outlineOverlay] {mouse.GetOverlaySprites()};
    // must be loaded before mainGUI
    const std::map<sf::Keyboard::Key, Shader*>& shader_map = Shader::LoadAll();
    const Shader* empty = shader_map.cbegin()->second;
    if (!empty->InvokeSwitch()) { 
        std::cerr << "ragequitting because empty shader didn't load.\n";
        return 2;
    }
    
    assert(IMGUI_CHECKVERSION() && "ImGui version-check failed!");
    std::cout << "using imgui v" << IMGUI_VERSION << '\n';
    
    MainGUI mainGUI{};
    if (mainGUI.initErrorFlag) {
        std::cerr << "mainGUI failed to init! exiting.\n";
        return 3;
    }
    mainGUI.SetupFluidParameters(&simulation.fluid);
    mainGUI.SetupSimulParameters(&simulation);
    mainGUI.SetupMouseParameters(&mouse);
    mainGUI.Create();
    
    // this is the only method to take back focus from the new window; 'requestFocus()' just gets ignored
    mainwindow.setVisible(false);
    mainwindow.setVisible(true);
    mainwindow.requestFocus();
    
    // TODO: refactor into mouse?
    hoverOutline.setFillColor(sf::Color::Transparent);
    hoverOutline.setOutlineColor(sf::Color::Cyan);
    hoverOutline.setOutlineThickness(2.5f);
    
    PrintKeybinds();
    
    sf::Clock frametimer{};
    
    const auto HandleKeypress = [&](const sf::Keyboard::Key& keycode)
    {
        const bool isShiftPressed {sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)};
        switch (keycode)
        {
            case sf::Keyboard::Q:
                if (gradientWindow.isOpen()) gradientWindow.close();
                if (mainGUI.isOpen()) mainGUI.close();
                mainwindow.close();
            break;
            
            case sf::Keyboard::G:
                std::cout << (isShiftPressed?"xgravity ":"gravity ") 
                << (simulation.ToggleGravity(isShiftPressed)? "enabled":"disabled") << '\n';
            break;
            
            case sf::Keyboard::Space:
                std::cout << (simulation.TogglePause()?"paused":"unpaused") << '\n';
            break;
            
            case sf::Keyboard::R:
                mouse.Reset(); // can't carry over locked cells at the moment
                simulation.Reset();
                std::cout << "Reset Simulation\n";
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
             
             /* Toggling window-visibility screws with the FPS calc in MainGUI (NumWindowsOpen), because it still counts as open.
              and there's no easy way to check for 'isEnabled' (because it only has access to the base-class 'RenderWindow')
              Simply opening/closing the window repeatedly works fine, and doesn't seem to cause any problems (yet) */
            
            //#define GRADIENTWINDOW_NOCLOSE
            #ifdef GRADIENTWINDOW_NOCLOSE
              case sf::Keyboard::F2:
                  gradientWindow.stored_xposition = mainwindow.getPosition().x;
                  if (!gradientWindow.isOpen()) {gradientWindow.isEnabled=true; gradientWindow.Create();}
                  else gradientWindow.ToggleEnabled(); // calls AdjustPosition
              break;
            #else // using close instead of 'ToggleEnabled'
              case sf::Keyboard::F2:
                  if (gradientWindow.isOpen()) { gradientWindow.close(); }
                  else { gradientWindow.isEnabled=true; gradientWindow.Create(); }
              break;
            #endif
            
            case sf::Keyboard::Tilde:
                // just pass focus to mainGUI if it's already open
                if (mainGUI.isEnabled) {
                    #define WINDOWMANAGERJANK
                    // Window-Manager moves the mainGUI window on it's own just from toggling the visibility off/on (and won't give focus otherwise lmao)
                    // so the GUI window will always need to be repositioned, even if it's not docked
                    #ifdef WINDOWMANAGERJANK
                        mainGUI.FollowMainWindow();
                    #endif
                    
                    mainGUI.setVisible(false);
                    mainGUI.setVisible(true);
                    mainGUI.requestFocus();
                    
                    #ifdef WINDOWMANAGERJANK
                        mainGUI.FollowMainWindow(); // have to do it twice because it clips into the main window the first time
                    #endif
                    #undef WINDOWMANAGERJANK
                    break;
                }
                else mainGUI.ToggleEnabled();
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
                // TODO: clear overlays if exiting painting-Mode?
                mouse.isPaintingMode = !mouse.isPaintingMode;
                std::cout << "Painting mode: " << ((mouse.isPaintingMode)? "enabled": "disabled") << '\n';
            }
            break;
            
            case sf::Keyboard::K:
                mouse.ClearPreservedOverlays();
                std::cout << "Cleared painted regions\n";
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
                const std::string previous_name = Shader::current->name;
                //Shader* selectedShader = shader_map.at(event.key.code);
                shader_map.at(keycode)->InvokeSwitch();
                // enable transparency and turbulence-mode automatically
                if (Shader::current->name == "turbulence") {
                    if(!simulation.ToggleTurbulence()) simulation.ToggleTurbulence();
                    if(previous_name == "turbulence") {
                        windowClearDisabled = !windowClearDisabled;
                        std::cout << "window clears " << (windowClearDisabled ? "disabled" : "enabled") << "\n";
                    }
                }
                else { if(simulation.ToggleTurbulence()) simulation.ToggleTurbulence(); } // disabling
                // TODO: restore previous settings for turbulence, transparency, and window-clears
            }
            break;
            
            case sf::Keyboard::Pause:
                windowClearDisabled = !windowClearDisabled;
                std::cout << "window clears " << (windowClearDisabled ? "disabled" : "enabled") << "\n";
            break;
            
            case sf::Keyboard::Add:
            case sf::Keyboard::Subtract:
            case sf::Keyboard::Dash:
            case sf::Keyboard::Equal:
                mainGUI.AdjustActiveSlider(keycode);
            break;
            
            // case sf::Keyboard::_:
            // break;
            
            default:
            break;
        }
        return;
    }; // End of HandleKeypress lambda
    
    // disables mainwindow, uses alternative minimal frameloop
    // #define TESTING_SECONDARY_WINDOWS
    #ifdef  TESTING_SECONDARY_WINDOWS
    gradientWindow.Create();
    gradientWindow.ToggleEnabled();
    gradientWindow.setVisible(true);
    gradientWindow.isEnabled = true;
    mainwindow.close();
    
    while(gradientWindow.isOpen() /* || mainGUI.isOpen() */) {
        std::vector<sf::Keyboard::Key> unhandled_keypresses{};
        if (gradientWindow.isOpen()) gradientWindow.FrameLoop();
        if (mainGUI.isOpen()) mainGUI.FrameLoop(unhandled_keypresses);
        for(const sf::Keyboard::Key& key: unhandled_keypresses) { HandleKeypress(key); } // still required for most keybinds (including the exit shortcuts!)
    }
    
    if (mainGUI.isOpen()) mainGUI.close();
    #endif
    
    // frameloop
    while (mainwindow.isOpen())
    {
        // imgui needs window and deltatime. You can pass mousePosition and displaySize instead of window
        frametimer.restart();
        std::vector<sf::Keyboard::Key> unhandled_keypresses{};
        // 'unhandled_keypresses' is used as an output parameter so that it can be passed to multiple other windows
        // and it puts the declaration in the outer scope
        
        if (gradientWindow.isOpen()) gradientWindow.FrameLoop(); 
        if (mainGUI.isOpen()) mainGUI.FrameLoop(unhandled_keypresses);
        for(const sf::Keyboard::Key& key: unhandled_keypresses) { HandleKeypress(key); }
        
        sf::Event event;
        while (mainwindow.pollEvent(event))
        {
            switch(event.type)
            {
                case sf::Event::Closed:
                    if (gradientWindow.isOpen()) gradientWindow.close();
                    if (mainGUI.isOpen()) mainGUI.close();
                    mainwindow.close();
                break;
                
                case sf::Event::KeyPressed:
                    HandleKeypress(event.key.code);
                break;
                
                case sf::Event::MouseButtonPressed:
                    if((event.mouseButton.button==3)||(event.mouseButton.button==4)) // side-buttons
                    { mouse.ClearPreservedOverlays(); break; } // manually handling this in case mouse isn't active
                [[fallthrough]];
                case sf::Event::MouseMoved:
                //case sf::Event::MouseLeft:
                //case sf::Event::MouseEntered:
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
        
        // TODO: can I get rid of this now?
        // alternate render logic for turbulence shader that skips window-clearing and the grid
        if (Shader::current->name == "turbulence") {
            if (simulation.isPaused || !windowClearDisabled) 
                mainwindow.clear(sf::Color::Transparent); // always clear when paused so you can modify and observe the effects of the 'threshold' parameter
            
            if (mouse.isActive(true) && mouse.isPaintingMode) { 
                mouse.RedrawOutlines();
                mainwindow.draw(outlineOverlay);
                hoverOutline.setFillColor({0x1A, 0xFF, 0x1A, 0x82});
                mainwindow.draw(hoverOutline); 
            }
            else if (mouse.shouldOutline) { hoverOutline.setFillColor(sf::Color::Transparent); mainwindow.draw(hoverOutline); }
            if (mouse.shouldDisplay) { mainwindow.draw(mouse); }
            
            simulation.Update();
            simulation.RedrawFluid(windowClearDisabled);
            mainwindow.draw(fluidSprite, Shader::current);
            
            // unlike the normal frameloop, here the mouse-outline is drawn even if the mouse is inactive;
            // without it, there's no visual indicator that the mouse is enabled, and no position.
            mainwindow.display();
            timestepRatio = float(frametimer.getElapsedTime().asMicroseconds() / 16666.66667);
            timestepRatio *= timestepMultiplier;
            continue;
        }
        
        
        if (!windowClearDisabled)
        mainwindow.clear(sf::Color::Transparent);
        simulation.Update();
        
        if (shouldDrawGrid || (mouse.isPaintingMode && mouse.isPaintingDebug)) {
            simulation.RedrawGrid();
            mainwindow.draw(gridSprite);
            mouse.shouldOutline = true;
        }
        
        if(!mouse.isActive()) { mouse.shouldDisplay = false; mouse.shouldOutline = false; }
        else if (mouse.isPaintingMode || mouse.isPaintingDebug) {
            if (mouse.isActive(true) || mouse.isPaintingDebug)
            {
                mouse.RedrawOverlay();
                mainwindow.draw(cellOverlay);
                mouse.shouldOutline = true;
                if (!mouse.isPaintingDebug)
                mouse.shouldDisplay = false;
            }
            else if (mouse.isActive()) 
                mouse.shouldOutline = !mouse.shouldDisplay;
        } 
        else if (mouse.isActive()) { mouse.shouldDisplay = true; }
        
        simulation.RedrawFluid(!windowClearDisabled);
        mainwindow.draw(fluidSprite, Shader::current);
        if (mouse.shouldOutline) { mouse.RedrawOutlines(); mainwindow.draw(outlineOverlay); }
        if (mouse.shouldDisplay) { mainwindow.draw(mouse); }
        
        mainwindow.display();
        
        timestepRatio = float(frametimer.getElapsedTime().asMicroseconds() / 16666.66667);
        timestepRatio *= timestepMultiplier;
    }
    
    ImGui::SFML::Shutdown();  // destroys ALL! contexts
    
    PrintSpeedcapInfo();
    #ifdef PMEMPTYCOUNTER
    std::cout << "pmemptycounter: " << pmemptycounter << '\n';
    #endif
    return 0;
}
