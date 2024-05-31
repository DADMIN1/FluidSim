#ifndef FLUIDSYM_MAINGUI_INCLUDED
#define FLUIDSYM_MAINGUI_INCLUDED

#include "Globals.hpp" // windowheight, timestep-Ratio/Multiplier
#include "Fluid.hpp"
#include "Simulation.hpp"
#include "Mouse.hpp"
// forward-declarations of the parameter-structs aren't practical;
// the constructors reference members, and 'delete' (in destructor and 'set_Params') requires full definition

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/WindowStyle.hpp>

#include <imgui.h>
#include <imgui-SFML.h>


class MainGUI: public sf::RenderWindow
{
    float m_width  {360}; // ImVec2 (used by SetWindowSize/Position) only holds floats
    float m_height {BOXHEIGHT};
    bool showDemoWindow {false};
    sf::Clock clock; // ImGui::SFML::Update() needs deltatime
    ImGuiContext* m_context;
    //ImGuiIO& imguiIO;  // you're not supposed to store this?
    
    friend int main(int, char**);
    
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
    
    struct MouseParameters
    {
        friend class Mouse_T;
        const Mouse_T* realptr;
        Mouse_T::Mode& mode;
        float& strength;
        bool& isPaintingMode;
        bool& isPaintingDebug;
        Cell*& hoveredCell;
        MouseParameters(Mouse_T* mouse): realptr{mouse},
            mode            {mouse->mode},
            strength        {mouse->strength},
            isPaintingMode  {mouse->isPaintingMode},
            isPaintingDebug {mouse->isPaintingDebug},
            hoveredCell     {mouse->hoveredCell}
        { ; }
    };
    
    // these variable names must follow this pattern (including uppercase) for the 'SETUPFUNCTION' macro (below)
    FluidParameters* FluidParams {nullptr};
    SimulParameters* SimulParams {nullptr};
    MouseParameters* MouseParams {nullptr};
    void DrawFluidParams(float& start_height); // modifies parameter
    void DrawSimulParams(float& start_height);
    void DrawMouseParams(float& start_height);
    
    
    // initializes a 'Parameter' struct and sets the corresponding 'Params' pointer (above)
    template <typename T, typename ParamStruct, ParamStruct* MainGUI::*structPtr> 
    void SetupParameters(T* realptr) {
        auto& memberPtr { this->*structPtr };
        if (memberPtr)  { delete memberPtr;};
        memberPtr = new ParamStruct(realptr);
    }
    
    // This macro constructs a function that simply calls an instantiation of 'SetupParameters' (derived from Classname)
    // An optional second argument aliases the real class-name, for cases where the variable-names don't match
    #define SETUPFUNCTION(Classname, ...) \
    __VA_OPT__(using Classname = __VA_ARGS__;) \
    void Setup##Classname##Parameters(Classname* ptr) \
    { SetupParameters<Classname, Classname##Parameters, &MainGUI::Classname##Params>(ptr); }
    
    // Setup-Functions //
    SETUPFUNCTION(Fluid);             // void SetupFluidParameters(Fluid*);
    SETUPFUNCTION(Simul, Simulation); // void SetupSimulParameters(Simulation*);
    SETUPFUNCTION(Mouse, Mouse_T);    // void SetupMouseParameters(Mouse_T*);
    
    #undef SETUPFUNCTION
    
    
    // internal draw functions
    void DrawFocusIndicator();
    void DrawFPS_Section(); // FPS display and VSync
    void DrawDockingControls(); // status and switches
    void HandleWindowEvents();
    
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
        if (FluidParams) { delete FluidParams; }
        if (SimulParams) { delete SimulParams; }
        if (MouseParams) { delete MouseParams; }
        ImGui::DestroyContext(m_context);
        //ImGui::SFML::Shutdown();  // destroys ALL! contexts
    }
};



#endif
