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


// the framerate is reported as divided among each window
int NumWindowsOpen() {
    int total = 1; //assuming MainGUI is always open to call this
    if (mainwindowPtr->isOpen())     total += 1;
    if (gradientWindowPtr->isOpen()) total += 1;
    return total;
}


void MainGUI::FrameLoop() 
{
    if (!isEnabled || !isOpen()) { return; }
    sf::RenderWindow::clear();
    if (dockedToMain) FollowMainWindow();
    
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
    
    ImGui::SFML::Update(*this, clock.restart());
    
    // If you pass a bool* into 'Begin()', it will show an 'x' to close that menu (state written to bool).
    // passing nullptr disables that closing button.
    ImGui::Begin("Main", nullptr, window_flags);
    ImGui::SetWindowPos({0, 75}); // leave space at the top for Nvidia's FPS/Vsync indicators
    ImGui::SetWindowSize({m_width, 200});
    
    // focus indicator
    ImGui::Separator();
    ImGui::Text("Focus: ");
    ImGui::SameLine();
    if (hasFocus()) ImGui::TextColored({0, 255, 0, 255}, "Side-Panel"    );
    else            ImGui::TextColored({255, 0, 0, 255}, "Primary-Window");
    ImGui::Separator();
    
    // the framerate is reported as divided among each window, so we compensate
    float framerate = ImGui::GetIO().Framerate * NumWindowsOpen();
    ImGui::Text("%.1f FPS (%.3f ms/frame)", framerate, 1000.0f/framerate);
    
    if (ImGui::Checkbox("VSync:", &usingVsync)) // returns true if state has changed
    {
        //std::cout << "vsync toggle event!\n";
        if (mainwindowPtr) mainwindowPtr->setVerticalSyncEnabled(usingVsync);
        if (gradientWindowPtr) gradientWindowPtr->setVerticalSyncEnabled(usingVsync);
        setVerticalSyncEnabled(usingVsync);
    }
    ImGui::SameLine();
    ImGui::Text("%s", (usingVsync? "Enabled" : "Disabled"));
    
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
    ImGui::End(); // Top half (Main)
    
    // Demo window
    ImGui::Begin("DemoToggle", nullptr, window_flags^ImGuiWindowFlags_MenuBar); // take away menubar
    ImGui::SetWindowPos({0, 275});
    ImGui::SetWindowSize({m_width, 725});
    if(ImGui::Button("Demo Window"))  // returns true if state has changed
        showDemoWindow = !showDemoWindow;
    if(showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
        ImGui::SetWindowPos("Dear ImGui Demo", {0, 315});
        ImGui::SetWindowSize("Dear ImGui Demo", {m_width, 685});
    }
    ImGui::End();
    
    ImGui::SFML::Render(*this);
    sf::RenderWindow::display();
    return;
}
