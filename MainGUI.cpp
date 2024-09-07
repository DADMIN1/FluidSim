#include "MainGUI.hpp"
#include "Slider.hpp"
#include "Shader.hpp" // turbulence_ptrs

#include <iostream> // only used in MainGUI::Initialize()

#include <SFML/Window.hpp>  // defines sf::Event
#include <imgui.h>
#include <imgui-SFML.h>


// Main.cpp
extern bool usingVsync;
extern const int framerateCap;
extern sf::RenderWindow* mainwindowPtr;
extern sf::RenderWindow* gradientWinPtr; // only for windowcount and setting vsync
// required for Mouse controls (when painting-debug is toggled)
extern bool shouldDrawGrid;
extern bool ToggleGridDisplay();

// required for 'threshold' slider. set during 'Initialize()'
static Shader* turbulence_ptr = nullptr;
static float* turbulence_threshold_ptr = nullptr;


// bitwise-OR flags together flags (into zero)
constexpr ImGuiWindowFlags window_flags { 
      ImGuiWindowFlags_None
    //| ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    // | ImGuiWindowFlags_NoBackground //transparent
    | ImGuiWindowFlags_NoCollapse
    //| ImGuiWindowFlags_MenuBar // adds a menubar
    //| ImGuiWindowFlags_AlwaysAutoResize  // doesn't allow manual resizing (of width)
};
constexpr ImGuiWindowFlags subwindow_flags {
      ImGuiWindowFlags_None
    | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoScrollbar
};
constexpr ImGuiChildFlags child_flags {
      ImGuiChildFlags_None
    | ImGuiChildFlags_Border  // apparently this always has to be set for backwards-compatibility?
    // | ImGuiChildFlags_ResizeX           // Allow resize from right border (layout direction). Enable .ini saving (unless ImGuiWindowFlags_NoSavedSettings passed to window flags)
    // | ImGuiChildFlags_ResizeY           // Allow resize from bottom border (layout direction)
    // ResizeX/Y are not allowed in combination with "AlwaysAutoResize"!! (Assert fails @imgui/imgui.cpp:5440: 'ImGui::BeginChildEx')
    | ImGuiChildFlags_AutoResizeX       // Enable auto-resizing width. Read "IMPORTANT: Size measurement" details above.
    | ImGuiChildFlags_AutoResizeY       // Enable auto-resizing height. Read "IMPORTANT: Size measurement" details above.
    | ImGuiChildFlags_AlwaysAutoResize  // Combined with AutoResizeX/AutoResizeY. Always measure size even when child is hidden, always return true, always disable clipping optimization! NOT RECOMMENDED.
};

constexpr ImGuiWindowFlags demowindow_flags {
      ImGuiWindowFlags_None 
    | ImGuiWindowFlags_MenuBar
    // | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    //| ImGuiWindowFlags_AlwaysAutoResize  // Resize every window to its content every frame
    | ImGuiWindowFlags_NoSavedSettings  // Never load/save settings in .ini file  // interferes with Debug/demo window?
    | ImGuiWindowFlags_HorizontalScrollbar // disallowed by default
    | ImGuiWindowFlags_NoFocusOnAppearing  // Disable taking focus when transitioning from hidden to visible state
    | ImGuiWindowFlags_NoBringToFrontOnFocus  // prevent it from overlapping other debug windows
    //| ImGuiWindowFlags_AlwaysVerticalScrollbar // Always show vertical scrollbar (even if ContentSize.y < Size.y)
    //| ImGuiWindowFlags_AlwaysHorizontalScrollbar  // Always show horizontal scrollbar (even if ContentSize.x < Size.x)
    | ImGuiWindowFlags_NoNavInputs  // No gamepad/keyboard navigation within the window
    | ImGuiWindowFlags_NoNavFocus   // No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB)
    | ImGuiWindowFlags_NoNav  // = ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus
    //| ImGuiWindowFlags_NoDecoration  // = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse
    //| ImGuiWindowFlags_NoInputs      // = ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus
};


bool MainGUI::Initialize()
{
    setVerticalSyncEnabled(usingVsync);
    if (!ImGui::SFML::Init(*this)) {
        std::cerr << "imgui-sfml failed to init! exiting.\n";
        return true; //error has occcured
    }
    // ImGuiContext gets created by 'ImGui::SFML::Init()'
    
    ImGuiIO& imguiIO = ImGui::GetIO(); // required for configflags and framerate
    
    //imguiIO.ConfigInputTrickleEventQueue      = false; // spreads interactions (like simultaneous keypresses) across multiple frames
    //imguiIO.ConfigInputTextEnterKeepActive    = true ; // keep input focused after hitting enter
    imguiIO.ConfigWindowsResizeFromEdges      = false ; // can be annoying, and it requires BackendFlags_HasMouseCursors anyway
    imguiIO.ConfigWindowsMoveFromTitleBarOnly = true ;
    
    imguiIO.ConfigFlags = ImGuiConfigFlags {
        ImGuiConfigFlags_None
        //| ImGuiConfigFlags_NavEnableKeyboard     // Master keyboard navigation enable flag. Enable full Tabbing + directional arrows + space/enter to activate.
        | ImGuiConfigFlags_NavNoCaptureKeyboard  // Instruct navigation to not set the io.WantCaptureKeyboard flag when io.NavActive is set.
        | ImGuiConfigFlags_NoMouseCursorChange   // Prevent imgui from altering mouse visibility/shape
        //| ImGuiConfigFlags_IsSRGB // Application is SRGB-aware. NOT used by core Dear ImGui (only used by backends, maybe)
    };
    
    //imguiIO.BackendFlags = ImGuiBackendFlags_HasMouseCursors;  // required for 'ResizeFromEdges'
    imguiIO.IniFilename = NULL; // disable autosaving of the 'imgui.ini' config file (just stores window states)
    
    // shaders must be initialized before this!
    turbulence_ptr = Shader::NameLookup["turbulence"];
    if ((!turbulence_ptr) || !turbulence_ptr->isValid) {
        std::cerr << "turbulence-pointer is null or invalid! exiting!\n";
        return true;
    }
    else if (turbulence_ptr && turbulence_ptr->isValid)
        turbulence_threshold_ptr = &turbulence_ptr->uniform_vars["threshold"];
    if (!turbulence_threshold_ptr) {
        std::cerr << "threshold-pointer is null! exiting!\n";
        return true;
    };
    
    return false;
}

void MainGUI::Create()
{
    if (isOpen()) { return; }
    constexpr auto m_style = sf::Style::Close; // also implies titlebar
    // sf::Style::Default = Titlebar | Resize | Close
    sf::RenderWindow::create(sf::VideoMode(m_width, m_height), "MainGUI [FLUIDSIM]", m_style);
    setVerticalSyncEnabled(usingVsync);
    setFramerateLimit(framerateCap);
    clock.restart();
    return;
}


void MainGUI::FollowMainWindow()
{
    if(!isEnabled || !mainwindowPtr || !mainwindowPtr->isOpen()) return;
    // set height to match mainwindow
    m_height = mainwindowPtr->getSize().y;
    //setSize({u_int(m_width), u_int(m_height)}); //this glitches out when debug window is active (constant resizing)
    
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
    if( mainwindowPtr->isOpen()) total += 1;
    if(gradientWinPtr->isOpen()) total += 1;
    return total;
}

void MainGUI::DrawFPS_Section() // FPS display and VSync
{
    // the framerate is reported as divided among each window, so we compensate
    // can't tell if the Nvidia overlay is wrong or ImGui? I'm assuming the latter, since Nvidia's matches VSync.
    float framerate = ImGui::GetIO().Framerate;
    float framerate_adj = framerate * NumWindowsOpen();
    
    ImGui::Text("%.1f FPS (%.3f ms/frame)", framerate, 1000.0f/framerate);
    ImGui::Text("%.1f FPS (actual) (%.3f ms/frame)", framerate_adj, 1000.0f/framerate_adj);
    ImGui::Separator();
    
    if (ImGui::Checkbox("VSync:", &usingVsync)) // returns true if state has changed
    {
        if( mainwindowPtr)  mainwindowPtr->setVerticalSyncEnabled(usingVsync);
        if(gradientWinPtr) gradientWinPtr->setVerticalSyncEnabled(usingVsync);
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


void MainGUI::AdjustActiveSlider(sf::Keyboard::Key plus_minus)
{
    if (!Slider::lastactive) return;
    Slider& slider {*Slider::lastactive};
    constexpr float one_over_64  = 1.f/64.f;
    constexpr float one_over_255 = 1.f/255.f;
    float value = *slider.heldvar;
    float valueDelta = std::abs(slider.max-slider.min)*one_over_255*one_over_64 + std::abs(value/slider.max)*one_over_64;
    if (valueDelta < 0.000001f) valueDelta = 0.000001f;
    else if (valueDelta < slider.stepsizeMin) valueDelta = slider.stepsizeMin;
    else if (valueDelta > slider.stepsizeMax) valueDelta = slider.stepsizeMax;
    
    const bool isShiftPressed {sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)};
    if (isShiftPressed) valueDelta *= 4.f;
    
    bool isPositive{false};
    switch (plus_minus)
    {
        case sf::Keyboard::Add:
        case sf::Keyboard::Equal:
             isPositive = true; [[fallthrough]];
        case sf::Keyboard::Subtract:
        case sf::Keyboard::Dash:
        {
            value += (isPositive? valueDelta : -valueDelta);
            if(value < slider.min) value = slider.min;
            if(value > slider.max) value = slider.max;
            *slider.heldvar = value;
            slider.Callback();
            return;
        }
        break;
        
        default: break; // shouldn't happen
    }
    return;
}


void MainGUI::HandleWindowEvents(std::vector<sf::Keyboard::Key>& unhandled_keypresses)
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
                    
                    // catching this to prevent mouse-toggling when alt-tabbing (or navigating by keyboard)
                    case sf::Keyboard::Tab: break;
                    
                    default:
                        unhandled_keypresses.push_back(event.key.code);
                    break;
                }
            }
            break;
            
            // disabled because this forces the debug window offscreen (since every section expands to m_width)
            /* case sf::Event::Resized:
            {
                auto [newwidth, newheight] = event.size;
                m_width = newwidth;
                m_height = newheight;
                if (dockedToMain) {
                    //setSize({m_width, m_height});
                    FollowMainWindow();
                }
            }
            break; */
            
            // handling the Mouse side-buttons. Can't be handled through 'unhandled_keypresses' because it's the wrong event-type
            case sf::Event::MouseButtonPressed:
            {
                switch (event.mouseButton.button)
                {
                    case 3: case 4: //side-buttons
                        MouseParams->realptr->ClearPreservedOverlays();
                    break;
                    
                    default: break;
                }
            }
            break;
            
            default:
            break;
        }
    }
    
    return;
}


// ========================= //
//  ---- SLIDER MACROS ----  //
// ========================= //

#define FMTSTRING(fmtstring) #fmtstring ": %." PRECISION() "f"
// FMTSTRING gets concatenated into a single string. ('name: %.3f')
// the spaces are actually necessary; C++11 requires a space between literal and string macro

#define VARIABLEPTR(field) &PSTRUCT()->PREFIX(field)

// local variables 'min', 'max', and 'xor_sliderflags' are expected to exist
#define DEFSLIDER(name, field) static Slider slider_##name {"##"#name, VARIABLEPTR(field), min, max, FMTSTRING(name), xor_sliderflags}
// Imgui hides text following a double-octothorpe (here it prevents the redundant label (since name is displayed in formatstring)

// optional arg can be additional callbacks/hooks to execute on slider interation
#define MAKESLIDER(field, ...) DEFSLIDER(field, field);\
    if(slider_##field) {           \
       slider_##field.Activate();  \
       __VA_OPT__(__VA_ARGS__;)    \
    } \
    [[maybe_unused]] bool resetButton_##field = slider_##field.ResetButton("Reset##"#field);
// Reset-buttons need unique names, otherwise they all reset the same slider.
// It also shouldn't match the slider's name, that causes it to act like the slider! (in addition to normal button behavior)

// when the slider's displayed text shouldn't be the field
#define MAKESLIDERNAMED(name, field, ...) DEFSLIDER(name, field);\
    if(slider_##name) {          \
       slider_##name.Activate(); \
       __VA_OPT__(__VA_ARGS__;)  \
    } \
    [[maybe_unused]] bool resetButton_##name = slider_##name.ResetButton("Reset##"#name);
// TODO: accept/set callback definitions within the macro

// use the slider macros by defining input macros like this: 
/* 
    #define PREFIX(field) momentum##field
    #define PSTRUCT() SimulParams
    #define PRECISION() "3" 
    MAKESLIDER(Transfer)
*/
// 'DEFSLIDER' expects local variables 'min', 'max', and 'xor_sliderflags' to be defined. (xor_sliderflags is negated from default-flags)
// the 'PREFIX' macro can be used to add a prefix to the field-name if it doesn't match the struct-member
// (in the example, it would construct: 'SimulParams->momentumTransfer' instead: of 'SimulParams->Transfer')


float MainGUI::DrawSimulParams(float next_height)
{
    ImGui::Begin("Simulation Parameters", nullptr, subwindow_flags^ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowPos({0, next_height});
    ImGui::SetWindowSize({m_width, -1});  // -1 retains current size
    
    // unfortunately negative timescales don't work properly
    static Slider slider_Timescale {"##Timescale", &timestepMultiplier, 0.001f, 2.0f, "Timescale: %.3fx"};
    slider_Timescale();
    ImGui::SameLine();
    if(ImGui::Button("Reset##Timescale")) timestepMultiplier = 1.0f;
    
    ImGui::SeparatorText("Momentum");
    
    #define PREFIX(fieldname) momentum##fieldname
    #define PSTRUCT() SimulParams
    #define PRECISION() "3"
    // you can define these as normal variables, but 'PRECISION' just cannot
    float min = 0.0f;
    float max = 1.0f;
    constexpr int xor_sliderflags=ImGuiSliderFlags_None; // uses default-flags (definition required for slider-macros)
    
    MAKESLIDER(Transfer) 
    MAKESLIDER(Distribution)
    
    #undef PREFIX
    #undef PSTRUCT
    #undef PRECISION
    
    next_height += ImGui::GetWindowHeight(); // 'GetWindowHeight' returns height of current section
    ImGui::End();
    return next_height;
}


float MainGUI::DrawFluidParams(float next_height)
{
    if (!FluidParams) return next_height; // TODO: give some kind of indicator that it's been invalidated
    //ImGui::Separator();
    ImGui::Begin("Fluid Parameters", nullptr, subwindow_flags^ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowPos({0, next_height});
    int numlines = 0;
    
    // seems slightly slower with const
    const sf::Color enabledColor  {0x1A, 0xEE, 0x22, 0xFF}; // green
    const sf::Color disableColor  {0xFF, 0x20, 0x20, 0xFF}; // red
    const sf::Color inactiveColor {0x5F, 0x5F, 0x5F, 0xFF}; // grey
    
    numlines += 1; // gravity & xgravity are on the same line
    if (SimulParams->hasGravity) {
        ImGui::Text("Gravity:");  ImGui::SameLine();
        ImGui::TextColored(enabledColor, "Enabled");
    } else {
        ImGui::TextColored(inactiveColor, "Gravity:");
        ImGui::SameLine(); ImGui::TextColored(disableColor, "Disabled");
    }
    
    ImGui::SameLine(0, 8.0f);
    ImGui::TextColored(inactiveColor-sf::Color{0,0,0,0x9F}, "|");
    ImGui::SameLine(0, 8.0f);
    
    if (SimulParams->hasXGravity) {
        ImGui::Text("XGravity:"); ImGui::SameLine();
        ImGui::TextColored(enabledColor, "Enabled");
    } else {
        ImGui::TextColored(inactiveColor, "XGravity:");
        ImGui::SameLine(); ImGui::TextColored(disableColor, "Disabled");
    }
    
    numlines += 5;
    #define PREFIX(fieldname) fieldname
    #define PSTRUCT() FluidParams
    #define PRECISION() "3"
    int xor_sliderflags = ImGuiSliderFlags_None;
    // you can define these as normal variables, but 'precision' just cannot
    float min = -3.0f;
    float max =  3.0f;
    
    // gravity sliders //
    MAKESLIDER(gravity , SimulParams->hasGravity  = true;);
    MAKESLIDER(xgravity, SimulParams->hasXGravity = true;);
    
    // fdensity / viscosity sliders //
    #undef  PRECISION
    #define PRECISION() "6"
    min = -0.5f; max = 0.5f;
    MAKESLIDER(viscosity);
    slider_viscosity.stepsizeMin = 0.00025f;
    slider_viscosity.stepsizeMax = 0.005f;
    min = 0.001f; // fdensity should not go below zero
    MAKESLIDER(fdensity);
    slider_fdensity.stepsizeMin = 0.000001f;
    slider_fdensity.stepsizeMax = 0.001f;
    
    #undef  PRECISION
    #define PRECISION() "2"
    min = -2.0f; max = 2.0f;
    xor_sliderflags = ImGuiSliderFlags_Logarithmic; // negating logarithmic flag
    MAKESLIDER(bounceDampening);
    
    // TODO: button to reset defaults
    
    #undef  VARIABLEPTR
    #define VARIABLEPTR(input) input
    min = 0.0f; max = 50.f;
    
    numlines += 3;
    ImGui::SeparatorText("Gradient Speed-Thresholds");
    MAKESLIDERNAMED(Min, &Fluid::gradient_thresholdLow ); max = 75.f;
    MAKESLIDERNAMED(Max, &Fluid::gradient_thresholdHigh);
    slider_Min.stepsizeMin = 0.25f;
    slider_Min.stepsizeMax = 2.5f;
    slider_Max.stepsizeMin = 0.25f;
    slider_Max.stepsizeMax = 2.5f;
    
    #undef PREFIX
    #undef PSTRUCT
    #undef PRECISION
    
    // for some reason this can't autosize correctly. // Note: titlebar also takes ~25 pixels
    ImGui::SetWindowSize({m_width, 25.0f*numlines});
    next_height += 25.0f*numlines;
    ImGui::End();
    
    return next_height;
}


// TODO: display size of mouse (RD) in UI
float MainGUI::DrawMouseParams(float next_height)
{
    constexpr int xor_sliderflags = ImGuiSliderFlags_None;
    
    const sf::Color enabledColor {0x1A, 0xEE, 0x22, 0xFF}; // green
    const sf::Color disableColor {0xFF, 0x20, 0x20, 0xFF}; // red
    // creates an enabled/disabled line of text from a bool, with colors
    [[maybe_unused]] auto BooleanText = [&enabledColor, &disableColor](const char* label, bool B)
    {
        if(B) { 
            ImGui::TextColored(enabledColor, "%s", label); 
            ImGui::SameLine();  ImGui::Text("Enabled"); }
        else  {
            ImGui::TextColored(disableColor, "%s", label);
            ImGui::SameLine(); ImGui::TextDisabled("Disabled"); 
        }
    };
    
    // applies the colors in the opposite order
    [[maybe_unused]] auto BooleanTextFlip = [&enabledColor, &disableColor](const char* label, bool B)
    {
        if(B) { 
            ImGui::Text("%s", label);
            ImGui::SameLine(); ImGui::TextColored(enabledColor, "%s", "Enabled"); }
        else  {
            ImGui::TextDisabled("%s", label);
            ImGui::SameLine(); ImGui::TextColored(disableColor, "Disabled"); 
        }
    };
    
    // Selects color for status text
    auto ModeColor = [](const Mouse_T::Mode& m) {
        switch(m) {
            case (Mouse_T::Mode::None):     return sf::Color{0xAA, 0xAA, 0xAA, 0xCC};
            case (Mouse_T::Mode::Push):     return sf::Color::Cyan;
            case (Mouse_T::Mode::Pull):     return sf::Color::Magenta;
            case (Mouse_T::Mode::Disabled): return sf::Color{0xAA, 0xAA, 0xAA, 0x77};
            
            default:
              case (Mouse_T::Mode::Drag):
              case (Mouse_T::Mode::Fill):
              case (Mouse_T::Mode::Erase):
            return sf::Color{0xFF, 0xFF, 0xFF, 0xFF};
        }
    };
    
    const sf::Color modeColor = ModeColor(this->MouseParams->mode);
    ImGui::PushStyleColor(ImGuiCol_Border    , modeColor-sf::Color{0x22222222});
    ImGui::PushStyleColor(ImGuiCol_TitleBg   , modeColor-sf::Color{0x20202077});
    ImGui::PushStyleColor(ImGuiCol_ChildBg   , modeColor-sf::Color{0x111111BB});
    ImGui::PushStyleColor(ImGuiCol_WindowBg  , modeColor-sf::Color{0x222222BB});
    ImGui::PushStyleColor(ImGuiCol_Separator , modeColor-sf::Color{0x20202020});
    
    ImGui::Begin("Mouse", nullptr, subwindow_flags^ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowPos({0, next_height});
    ImGui::SetWindowSize({m_width, -1});  // -1 retains current size
    
    ImGui::PushStyleColor(ImGuiCol_Text, modeColor);
    const auto Statusline = std::string{"[ActiveState]: "} + std::string{MouseParams->realptr->ModeToString()};
    ImGui::SeparatorText(Statusline.c_str());
    //ImGui::TextColored(modeColor , "ActiveState: %s", MouseParams->realptr->ModeToString());
    
    ImGui::PopStyleColor(1);  // text previous text-color
    // this style causes the text to follow the mode when it's enabled, and white when disabled
    ImGui::PushStyleColor(ImGuiCol_Text, modeColor+sf::Color::White*modeColor);
    
    //const bool isDisabled = (this->MouseParams->mode == Mouse_T::Mode::Disabled);
    const bool isEnabled = (MouseParams->isPaintingDebug 
        || (MouseParams->isPaintingMode && MouseParams->realptr->isActive(true)));
    
    static std::size_t numlocked{0};
    static std::size_t numUnlocked{0};
    numlocked = Mouse_T::preservedOverlays.size(); // always update (in case of clears/overwrites)
    if (isEnabled) {  // otherwise the numUnlocked keeps changing (only makes sense in debug)
        numUnlocked = Mouse_T::savedState.size();
    } else numUnlocked = 0; // otherwise it doesn't reset lol
    
    ImGui::BeginDisabled(!isEnabled);
    
    ImGui::Text("Painted:%lu",  numlocked+numUnlocked); ImGui::SameLine();
    ImGui::Text("unlocked:%lu", numUnlocked);           ImGui::SameLine();
    ImGui::Text("locked:%lu",   numlocked); // '%lu' == long unsigned int
    ImGui::Separator();
    
    ImGui::EndDisabled();
    
    //BooleanText("Painting:", MouseParams->isPaintingMode ); ImGui::SameLine();
    //BooleanText("   Debug:", MouseParams->isPaintingDebug); 
    BooleanTextFlip("Painting:", MouseParams->isPaintingMode ); ImGui::SameLine();
    BooleanTextFlip("   Debug:", MouseParams->isPaintingDebug); ImGui::SameLine();
    if (ImGui::Button("Toggle")) {
        bool newstate = MouseParams->isPaintingDebug = !MouseParams->isPaintingDebug;
        MouseParams->isPaintingMode = newstate; // painting and grid match state
        shouldDrawGrid = newstate;
    }
    ImGui::Separator();
    ImGui::PopStyleColor(6);
    
    float min{1.f}; float max{255.f};
    #define PRECISION() // no decimal displayed
    
    static bool shouldDrawGrid_old;
    DEFSLIDER(Strength, &MouseParams->strength);
    slider_Strength.stepsizeMin = 1.0f;
    slider_Strength.stepsizeMax = 2.5f;
    if(slider_Strength) {
        slider_Strength.Activate();
        shouldDrawGrid = true;
    }
    if(ImGui::IsItemDeactivatedAfterEdit()) { shouldDrawGrid = shouldDrawGrid_old; } // waiting until slider is released
    else if(!ImGui::IsItemActive()) { shouldDrawGrid_old = shouldDrawGrid; }
    
    // creating the callback must be done in two steps, since capturing lambdas CANNOT be stored in any kind of function-pointer
    // static lambda is also required, otherwise the second lambda would be forced to capture it. Alternative, a static member variable would work
    static const auto Lambda = [ MouseParams=this->MouseParams ](){ MouseParams->realptr->RecalculateModDensities(); };
    slider_Strength.Callback = [](){ Lambda(); };
    
    if(slider_Strength.ResetButton("Reset##Strength")) { Lambda(); }
    
    #undef PRECISION
    
    ImGui::Separator();
    next_height += ImGui::GetWindowHeight();
    ImGui::End();
    return next_height;
}


float MainGUI::DrawTurbSection(float next_height)
{
    ImGui::Begin("turbulence_section", nullptr, subwindow_flags);
    ImGui::SetWindowPos({0, next_height});
    ImGui::SetWindowSize({m_width, -1});  // -1 retains current size
    
    // TODO: disable if not using turbulence shader
    bool isTurbulent = FluidParams->realptr->isTurbulent; // TODO: need a better condition than this (check if the appropriate shader is active)
    
    ImGui::Text("Turbulence:"); ImGui::SameLine();
    if (isTurbulent)
         ImGui::TextColored({0xFF, 0x00, 0x08, 0xFF}, "Enabled" );
    else ImGui::TextColored({0x00, 0x40, 0xFF, 0xFF}, "Disabled");
    
    if(!isTurbulent) ImGui::BeginDisabled();
    static Slider slider_tthreshold {"##shader_parameter", turbulence_threshold_ptr, 0.f, 1.f, "threshold: %.2f", ImGuiSliderFlags_Logarithmic};
    slider_tthreshold.Callback = [](){ turbulence_ptr->ApplyUniform("threshold", *turbulence_threshold_ptr); };
    slider_tthreshold();
    if(slider_tthreshold.ResetButton("Reset##tthreshold")) { slider_tthreshold.Callback(); }
    if(!isTurbulent) ImGui::EndDisabled();
    
    next_height += ImGui::GetWindowHeight();
    ImGui::End();
    return next_height;
}


void MainGUI::FrameLoop(std::vector<sf::Keyboard::Key>& unhandled_keypresses) 
{
    if (!isEnabled || !isOpen()) { return; }
    if (dockedToMain) FollowMainWindow();
    
    sf::RenderWindow::setActive(); // possibly unnecessary?
    HandleWindowEvents(unhandled_keypresses);
    ImGui::SFML::Update(*this, clock.restart());
    sf::RenderWindow::clear();
    
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
    
    // Simulation Parameters
    next_height = DrawSimulParams(next_height);
    next_height = DrawFluidParams(next_height);
    next_height = DrawTurbSection(next_height);
    next_height = DrawMouseParams(next_height);
    
    
    // Demo-Window Toggle Button
    ImGui::Begin("DemoToggle", nullptr, subwindow_flags);
    ImGui::SetWindowPos({0, next_height});
    
    //ImGui::Separator();
    if(ImGui::Button("Demo Window")) 
    {
        showDemoWindow = !showDemoWindow;
        unsigned int nextWidth = (showDemoWindow? m_width*2.f : m_width);
        sf::RenderWindow::setSize({nextWidth, sf::RenderWindow::getSize().y});
    }
    //ImGui::Separator();
    
    ImGui::SetWindowSize({m_width, -1});
    next_height += ImGui::GetWindowHeight();
    ImGui::End(); // Bottom half (Demo Window
    
    // Actual Demo Window
    if(showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow, demowindow_flags);
        ImGui::SetWindowPos ("Dear ImGui Demo", {m_width,  0}); // selecting window by name
        ImGui::SetWindowSize("Dear ImGui Demo", {m_width, m_height});  // using '-1' for height here disables resizing from edges, regardless of windowflags/IOflags
    }
    
    ImGui::SFML::Render(*this);
    sf::RenderWindow::display();
    return;
}


// SLIDER MACROS //
#undef FMTSTRING
#undef VARIABLEPTR
#undef DEFSLIDER
#undef MAKESLIDER
#undef MAKESLIDERNAMED

