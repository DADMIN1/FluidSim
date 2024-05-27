#include "MainGUI.hpp"
#include "Gradient.hpp" // gradientWindow type-definition

#include <SFML/Window.hpp>  // defines sf::Event

// #include <iostream>


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
    // sf::Style::None
    // for some reason it's only letting me use auto here
    constexpr auto m_style = sf::Style::Close;  // Title-bar is implied (for Style::Close)
    // sf::Style::Default = Titlebar | Resize | Close
    sf::RenderWindow::create(sf::VideoMode(m_width, m_height), "Main", m_style);
    setFramerateLimit(framerateCap);
    setVerticalSyncEnabled(usingVsync);
    clock.restart();
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
    if (!isOpen()) { return; }
    sf::RenderWindow::clear();
    
    sf::Event event;
    while (sf::RenderWindow::pollEvent(event)) 
    {
        ImGui::SFML::ProcessEvent(*this, event);
        switch(event.type)
        {
            case sf::Event::Closed:
                sf::RenderWindow::close();
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
    ImGui::SetWindowSize({420, 200});
    
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
    ImGui::End();
    
    ImGui::Begin("DemoToggle", nullptr, window_flags^ImGuiWindowFlags_MenuBar); // take away menubar
    ImGui::SetWindowPos({0, 275});
    ImGui::SetWindowSize({420, 725});
    if(ImGui::Button("Demo Window"))  // returns true if state has changed
        showDemoWindow = !showDemoWindow;
    if(showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
        ImGui::SetWindowPos("Dear ImGui Demo", {0, 315});
        ImGui::SetWindowSize("Dear ImGui Demo", {420, 685});
    }
    ImGui::End();
    
    ImGui::SFML::Render(*this);
    sf::RenderWindow::display();
    return;
}
