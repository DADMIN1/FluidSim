#include "MainGUI.hpp"
#include "Gradient.hpp" // gradientWindow type-definition

#include <SFML/Window.hpp>  // defines sf::Event

//#include <iostream>


// Main.cpp
extern bool usingVsync;
constexpr int framerateCap{300}; // const/constexpr makes 'extern' fail, so this is duplicated here
extern sf::RenderWindow* mainwindowPtr;
extern GradientWindow_T* gradientWindowPtr;


bool MainGUI::Initialize()
{
    setFramerateLimit(framerateCap);
    setVerticalSyncEnabled(usingVsync);
    if (!ImGui::SFML::Init(*this)) {
        // std::cerr << "imgui-sfml failed to init! exiting.\n";
        return true; //error has occcured
    }
    
    // creating the context (in constructor) already sets current context; DON'T call again (double-free)
    //m_context = ImGui::CreateContext();
    //ImGui::SetCurrentContext(m_context);
    
    //ImGuiIO& imguiIO = ImGui::GetIO(); // required for configflags and framerate
    //imguiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //imguiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    return false;
}

void MainGUI::Create()
{
    if (isOpen()) { return; }
    constexpr auto m_style = sf::Style::Titlebar | sf::Style::Resize;
    // sf::Style::Default = Titlebar | Resize | Close
    sf::RenderWindow::create(sf::VideoMode(m_width, m_height), "FLUIDSIM - Main GUI", m_style);
    setFramerateLimit(framerateCap);
    setVerticalSyncEnabled(usingVsync);
    clock.restart();
    return;
}


void MainGUI::FollowMainWindow()
{
    if(!isEnabled || !mainwindowPtr || !mainwindowPtr->isOpen()) return;
    // set height to match mainwindow
    m_height = mainwindowPtr->getSize().y;
    setSize({u_int(m_width), u_int(m_height)});
    
    // snap position to either side of mainwindow
    auto [mwx, mwy] = mainwindowPtr->getPosition();
    sf::Vector2i targetposition{mwx, mwy};
    
    if (dockSideLR) { targetposition.x += mainwindowPtr->getSize().x; } //right-side
    else { targetposition.x -= m_width; } //left-side
    
    setPosition(targetposition);
    return;
}

void MainGUI::ToggleEnabled()
{
    if (!sf::RenderWindow::isOpen()) {
        MainGUI::Create();
        isEnabled = false; // gets toggled back to true
    }
    isEnabled = !isEnabled;
    sf::RenderWindow::setVisible(isEnabled);
    if (isEnabled) FollowMainWindow();
    
    return;
}

// indicates which window has focus
void MainGUI::DrawFocusIndicator()
{
    ImGui::Text("Focus: ");
    ImGui::SameLine();
    if (hasFocus()) ImGui::TextColored({0, 255, 0, 255}, "Side-Panel"    );
    else            ImGui::TextColored({255, 0, 0, 255}, "Primary-Window");
    ImGui::Separator();
    return;
}

// the framerate is reported as divided among each window
int NumWindowsOpen() {
    int total = 1; //assuming MainGUI is always open to call this
    if (mainwindowPtr->isOpen())     total += 1;
    if (gradientWindowPtr->isOpen()) total += 1;
    return total;
}

void MainGUI::DrawFPS_Section() // FPS display and VSync
{
    // the framerate is reported as divided among each window, so we compensate
    float framerate = ImGui::GetIO().Framerate * NumWindowsOpen();
    ImGui::Text("%.1f FPS (%.3f ms/frame)", framerate, 1000.0f/framerate);
    ImGui::Separator();
    
    if (ImGui::Checkbox("VSync:", &usingVsync)) // returns true if state has changed
    {
        //std::cout << "vsync toggle event!\n";
        if (mainwindowPtr) mainwindowPtr->setVerticalSyncEnabled(usingVsync);
        if (gradientWindowPtr) gradientWindowPtr->setVerticalSyncEnabled(usingVsync);
        setVerticalSyncEnabled(usingVsync);
    }
    ImGui::SameLine();
    ImGui::Text("%s", (usingVsync? "Enabled" : "Disabled"));
    return;
}

void MainGUI::DrawDockingControls() // status and switches
{
    // docking controls
    ImGui::Checkbox("Docked:", &dockedToMain);
    ImGui::SameLine();
    ImGui::Text("%s ", (dockSideLR? "Right" : "Left"));
    if (dockedToMain) {
        ImGui::SameLine();
        ImGui::BeginDisabled(!dockSideLR);
        if(ImGui::Button("L")) { dockSideLR = false; FollowMainWindow(); }
        ImGui::EndDisabled();
        
        ImGui::BeginDisabled(dockSideLR);
        ImGui::SameLine();
        if(ImGui::Button("R")) { dockSideLR = true; FollowMainWindow(); }
        ImGui::EndDisabled();
    }
    ImGui::Separator();
    return;
}


void MainGUI::HandleWindowEvents()
{
    // event handling
    sf::Event event;
    while (sf::RenderWindow::pollEvent(event)) 
    {
        ImGui::SFML::ProcessEvent(*this, event);
        switch(event.type)
        {
            case sf::Event::Closed:
                isEnabled = false;
                sf::RenderWindow::close();
            break;
            
            case sf::Event::KeyPressed:
            {
                switch(event.key.code)
                {
                    case sf::Keyboard::Q:
                    case sf::Keyboard::Escape:
                        ToggleEnabled();
                    break;
                    
                    case sf::Keyboard::Tilde:  // just pass focus to mainwindow
                        if (mainwindowPtr && mainwindowPtr->isOpen()) {
                            mainwindowPtr->setVisible(false);
                            mainwindowPtr->setVisible(true);
                            mainwindowPtr->requestFocus();
                        }
                    break;
                    
                    case sf::Keyboard::Left:
                    case sf::Keyboard::Right:
                    {
                        dockedToMain = true;
                        dockSideLR = (event.key.code == sf::Keyboard::Right);
                        FollowMainWindow();
                    }
                    break;
                    
                    default:
                    break;
                }
            }
            break;
            
            case sf::Event::Resized:
            {
                // don't allow resizing if docked; it can glitch out the titlebar
                if (dockedToMain) {
                    //setSize({m_width, m_height});
                    FollowMainWindow();
                    break;
                }
                auto [newwidth, newheight] = event.size;
                m_width = newwidth;
                m_height = newheight;
            }
            break;
            
            default:
            break;
        }
    }
    
    return;
}


void MainGUI::DrawFluidParameters(float& next_height)
{
    if (!fluidptr) return;  // TODO: give some kind of indicator that it's been invalidated
    //ImGui::Separator();
    ImGui::Begin("Fluid Parameters", nullptr, subwindow_flags^ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowPos({0, next_height});
    int numlines = 1;
    
    auto sliderflags = ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic | \
                           ImGuiSliderFlags_NoInput | ImGuiSliderFlags_NoRoundToFormat;
    // input is disabled because it's also activated with 'Tab' (normally Ctrl+click);
    // which you're likely to hit accidentally when trying to activate mouse
    
    // VSliderFloat also exists, but it requires it's size to be specified (for some reason)
    #define FP(field, precision, max) ++numlines; \
        ImGui::SliderFloat(#field, &fluidptr->field, MIN(max), max, #field": %."#precision"f", sliderflags);
    
    // set min == -max
    #define MIN(f) -f
    FP(gravity,   3, 3.0f);
    FP(viscosity, 6, 0.5f); // negative values are a lot like 'turbulence' mode
    #undef MIN
    #define MIN(f) 0.001f   // fdensity should not go below zero
    FP(fdensity,  6, 0.5f);
    #undef MIN
    #define MIN(f) -f
    sliderflags ^= ImGuiSliderFlags_Logarithmic; // negating logarithmic flag
    FP(bounceDampening, 2, 2.0f); // values past one don't actually make sense (or negatives)
    
    #undef MIN
    #undef FP
    // TODO: disable gravity if it's not enabled; or auto-enable gravity
    // TODO: button to reset defaults
    
    numlines += 1;
    ImGui::Separator();
    ImGui::Text("Turbulence:"); ImGui::SameLine();
    if (fluidptr->realptr->isTurbulent) 
         ImGui::TextColored({255, 0, 8, 255}, "Enabled" );
    else ImGui::TextColored({0, 64, 255, 255}, "Disabled");
    ImGui::Separator();
    
    // for some reason this can't autosize correctly. // Note: titlebar also takes ~25 pixels
    ImGui::SetWindowSize({m_width, 25.0f*numlines});
    next_height += 25.0f*numlines;
    ImGui::End();
    //ImGui::Separator();
    return;
}


void MainGUI::FrameLoop() 
{
    if (!isEnabled || !isOpen()) { return; }
    sf::RenderWindow::clear();
    if (dockedToMain) FollowMainWindow();
    
    HandleWindowEvents();
    
    // IMGUI
    ImGui::SFML::Update(*this, clock.restart());
    
    float next_height = 75.f; // leave space at the top for Nvidia's FPS/Vsync indicators
    
    // If you pass a bool* into 'Begin()', it will show an 'x' to close that menu (state written to bool).
    // passing nullptr disables that closing button.
    ImGui::Begin("Main", nullptr, window_flags);
    ImGui::SetWindowPos({0, next_height});
    DrawFocusIndicator();
    DrawFPS_Section();
    DrawDockingControls();
    ImGui::SetWindowSize({m_width, -1});  // -1 retains current size
    next_height += ImGui::GetWindowHeight(); // 'GetWindowHeight' returns height of current section
    ImGui::End(); // Top section (Main)
    
    DrawFluidParameters(next_height);
    
    // Demo-Window Toggle Button
    ImGui::Begin("DemoToggle", nullptr, subwindow_flags);
    ImGui::SetWindowPos({0, next_height});
    
    //ImGui::Separator();
    if(ImGui::Button("Demo Window")) showDemoWindow = !showDemoWindow;
    //ImGui::Separator();
    
    ImGui::SetWindowSize({m_width, ImGui::GetWindowHeight()});
    next_height += ImGui::GetWindowHeight();
    
    // Actual Demo Window
    if(showDemoWindow)
    {
        // cannot make this a child window; it just refuses to behave correctly
        //ImGui::BeginChild("DemoWindow", {m_width, remaining_height}, 0, 0);
        ImGui::ShowDemoWindow(&showDemoWindow);
        ImGui::SetWindowPos("Dear ImGui Demo", {0, next_height}); // selecting window by name
        ImGui::SetWindowSize("Dear ImGui Demo", {m_width, m_height-next_height});
        
        //ImGui::EndChild(); //demowindow
    }
    ImGui::End(); // Bottom half (Demo Window)
    
    ImGui::SFML::Render(*this);
    sf::RenderWindow::display();
    return;
}
