#ifndef FLUIDSYM_MAINGUI_INCLUDED
#define FLUIDSYM_MAINGUI_INCLUDED

#include "Globals.hpp" // windowheight

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/WindowStyle.hpp>

#include <imgui.h>
#include <imgui-SFML.h>


// TODO: prevent auto-rewriting 'imgui.ini' on exit
class MainGUI: public sf::RenderWindow
{
    float m_width  {255}; // ImVec2 (used by SetWindowSize/Position) only holds floats
    float m_height {BOXHEIGHT};
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
    bool isEnabled {true};
    bool dockedToMain{true}; // keeps GUI docked left/right of mainwindow
    bool dockSideLR{true};   // false=Left, true=Right
    bool Initialize();
    void Create(); // calls sf::RenderWindow.create(...) with some arguments
    void FrameLoop(); // performs a single round of clear/draw/display and event-processing for gradient_window
    void ToggleEnabled();
    void FollowMainWindow(); // updates window position/height to match main
    
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
