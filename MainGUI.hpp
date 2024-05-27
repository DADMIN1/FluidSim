#ifndef FLUIDSYM_MAINGUI_INCLUDED
#define FLUIDSYM_MAINGUI_INCLUDED

#include <SFML/Graphics/RenderWindow.hpp>

#include <imgui.h>
#include <imgui-SFML.h>


// TODO: prevent auto-rewriting 'imgui.ini' on exit
class MainGUI: public sf::RenderWindow
{
    int m_width  {420};
    int m_height {1000};
    bool showDemoWindow {false};
    
    // bitwise-OR flags together flags (into zero)
    ImGuiWindowFlags window_flags { 0 
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize
        // | ImGuiWindowFlags_NoBackground //transparent
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_MenuBar // adds a menubar
    };
    
    sf::Clock clock; // ImGui::SFML::Update() needs deltatime
    ImGuiContext* m_context;
    //ImGuiIO& imguiIO;  // you're not supposed to store this?
    
    public:
    const bool initErrorFlag;
    bool showMainGUI {true};
    bool Initialize();
    void Create(); // calls sf::RenderWindow.create(...) with some arguments
    void FrameLoop(); // performs a single round of clear/draw/display and event-processing for gradient_window
    
    // constructors do not actually create the window; call '.create()' later
    MainGUI(): sf::RenderWindow(), 
    m_context{ImGui::CreateContext()}, /* imguiIO{ImGui::GetIO()}, */ initErrorFlag{Initialize()}
    { ; }
    
    ~MainGUI() {
        ImGui::DestroyContext(m_context);
        //ImGui::SFML::Shutdown();  // destroys ALL! contexts
    }
};



#endif
