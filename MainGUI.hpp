#ifndef FLUIDSYM_MAINGUI_INCLUDED
#define FLUIDSYM_MAINGUI_INCLUDED

#include "Globals.hpp" // windowheight, timestep-Ratio/Multiplier
#include "Fluid.hpp"
#include "Simulation.hpp"
// forward-declarations of the parameter-structs aren't practical;
// the constructors reference members, and 'delete' (in destructor and 'set_Params') requires full definition

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
    sf::Clock clock; // ImGui::SFML::Update() needs deltatime
    ImGuiContext* m_context;
    //ImGuiIO& imguiIO;  // you're not supposed to store this?
    
    struct FluidParameters
    {
        friend class Fluid;
        const Fluid* realptr;
        float& gravity;
        float& viscosity;
        float& fdensity;
        float& bounceDampening;
        FluidParameters(Fluid* fluid): realptr{fluid},
            gravity        {fluid->gravity  }, 
            viscosity      {fluid->viscosity}, 
            fdensity       {fluid->fdensity }, 
            bounceDampening{fluid->bounceDampening}
        { ; }
    };
    
    struct SimulParameters
    {
        friend class Simulation;
        const Simulation* realptr;
        float& momentumTransfer;
        float& momentumDistribution;
        SimulParameters(Simulation* simulation): realptr{simulation},
            momentumTransfer     {simulation->momentumTransfer},
            momentumDistribution {simulation->momentumDistribution}
        { ; }
        //void MakeSliderFloats();
    };
    
    FluidParameters* fluidParams {nullptr};
    SimulParameters* simulParams {nullptr};
    void DrawFluidParams(float& start_height); // modifies parameter
    void DrawSimulParams(float& start_height);
    
    
    // internal draw functions
    void DrawFocusIndicator();
    void DrawFPS_Section(); // FPS display and VSync
    void DrawDockingControls(); // status and switches
    void HandleWindowEvents();
    
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
    
    
    void SetFluidParams(Fluid* fluidPtr) { 
        if (fluidParams) { delete fluidParams; }
        fluidParams = new FluidParameters(fluidPtr);
    }
    
    void SetSimulationParams(Simulation* simulationPtr) { 
        if (simulParams) { delete simulParams; }
        simulParams = new SimulParameters(simulationPtr);
    }
    
    // constructors do not actually create the window; call '.create()' later
    MainGUI(): sf::RenderWindow(), 
    m_context{ImGui::CreateContext()}, /* imguiIO{ImGui::GetIO()}, */ initErrorFlag{Initialize()}
    { ; }
    
    ~MainGUI() {
        if (fluidParams) { delete fluidParams; }
        if (simulParams) { delete simulParams; }
        ImGui::DestroyContext(m_context);
        //ImGui::SFML::Shutdown();  // destroys ALL! contexts
    }
};



#endif
