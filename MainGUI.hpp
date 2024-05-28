#ifndef FLUIDSYM_MAINGUI_INCLUDED
#define FLUIDSYM_MAINGUI_INCLUDED

#include "Globals.hpp" // windowheight
#include "Fluid.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/WindowStyle.hpp>

#include <imgui.h>
#include <imgui-SFML.h>


// TODO: prevent auto-rewriting 'imgui.ini' on exit
class MainGUI: public sf::RenderWindow
{
    float m_width  {360}; // ImVec2 (used by SetWindowSize/Position) only holds floats
    float m_height {BOXHEIGHT};
    bool showDemoWindow {false};
    
    // bitwise-OR flags together flags (into zero)
    const ImGuiWindowFlags window_flags { 0 
        //| ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize
        // | ImGuiWindowFlags_NoBackground //transparent
        | ImGuiWindowFlags_NoCollapse
        //| ImGuiWindowFlags_MenuBar // adds a menubar
        //| ImGuiWindowFlags_AlwaysAutoResize  // doesn't allow manual resizing (of width)
    };
    const ImGuiWindowFlags subwindow_flags { 0
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
    };
    const ImGuiChildFlags child_flags { 0
        //| ImGuiChildFlags_ResizeY
        | ImGuiChildFlags_AutoResizeY
        //| ImGuiChildFlags_AutoResizeX
        | ImGuiChildFlags_AlwaysAutoResize
    };
    
    
    struct FluidParameters
    {
        friend class Fluid;
        const Fluid* realptr;
        float& gravity;
        float& viscosity;
        float& fdensity;
        float& bounceDampening;
        FluidParameters(Fluid& fluid): realptr{&fluid},
            gravity        {fluid.gravity  }, 
            viscosity      {fluid.viscosity}, 
            fdensity       {fluid.fdensity }, 
            bounceDampening{fluid.bounceDampening}
        { ; }
    };
    
    FluidParameters* fluidptr {nullptr};
    
    sf::Clock clock; // ImGui::SFML::Update() needs deltatime
    ImGuiContext* m_context;
    //ImGuiIO& imguiIO;  // you're not supposed to store this?
    
    void HandleWindowEvents();
    
    // internal draw functions
    void DrawFocusIndicator();
    void DrawFPS_Section(); // FPS display and VSync
    void DrawDockingControls(); // status and switches
    void DrawFluidParameters(float& start_height);  // modifies next_height
    
    public:
    void SetFluidPtr(Fluid& fluid) { 
        if (fluidptr) { delete fluidptr; }
        fluidptr = new FluidParameters(fluid);
    }
    
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
        if (fluidptr) { delete fluidptr; }
    }
};



#endif
