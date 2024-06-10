#include "MainGUI.hpp"
#include "Gradient.hpp" // gradientWindow type-definition

#include <SFML/Window.hpp>  // defines sf::Event


// Main.cpp
extern bool usingVsync;
constexpr int framerateCap{300}; // const/constexpr makes 'extern' fail, so this is duplicated here
extern sf::RenderWindow* mainwindowPtr;
extern GradientWindow_T* gradientWindowPtr;

// Main.cpp
// required for Mouse controls (when painting-debug is toggled)
extern bool shouldDrawGrid;
extern bool ToggleGridDisplay();


// bitwise-OR flags together flags (into zero)
constexpr ImGuiWindowFlags window_flags { 0 
    //| ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    // | ImGuiWindowFlags_NoBackground //transparent
    | ImGuiWindowFlags_NoCollapse
    //| ImGuiWindowFlags_MenuBar // adds a menubar
    //| ImGuiWindowFlags_AlwaysAutoResize  // doesn't allow manual resizing (of width)
};
constexpr ImGuiWindowFlags subwindow_flags { 0
    | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoScrollbar
};
constexpr ImGuiChildFlags child_flags { 0
    //| ImGuiChildFlags_ResizeY
    | ImGuiChildFlags_AutoResizeY
    //| ImGuiChildFlags_AutoResizeX
    | ImGuiChildFlags_AlwaysAutoResize
};


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
    
    ImGuiIO& imguiIO = ImGui::GetIO(); // required for configflags and framerate
    //imguiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //imguiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    imguiIO.IniFilename = NULL; // disable autosaving of the 'imgui.ini' config file (just stores window states)
    return false;
}

void MainGUI::Create()
{
    if (isOpen()) { return; }
    constexpr auto m_style = sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close;
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

// TODO: enforce min/max values
void MainGUI::AdjustActiveSlider(sf::Keyboard::Key plus_minus)
{
    if (!lastactiveSlider) return;
    
    float valueDelta = std::abs(*lastactiveSlider) * 0.01f;
    if (valueDelta < 0.001f) valueDelta = 0.001f;
    const bool isShiftPressed {sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)};
    if (isShiftPressed) valueDelta *= 10.f;
    
    bool isPositive{false};
    switch (plus_minus)
    {
        case sf::Keyboard::Add:
        case sf::Keyboard::Equal:
             isPositive = true;
        [[fallthrough]];
        case sf::Keyboard::Subtract:
        case sf::Keyboard::Dash:
            *lastactiveSlider += (isPositive? valueDelta : -valueDelta);
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
                        unhandled_keypresses.push_back(event.key.code);
                    break;
                }
            }
            break;
            
            case sf::Event::Resized:
            {
                auto [newwidth, newheight] = event.size;
                m_width = newwidth;
                m_height = newheight;
                if (dockedToMain) {
                    //setSize({m_width, m_height});
                    FollowMainWindow();
                }
            }
            break;
            
            default:
            break;
        }
    }
    
    return;
}

/* 
void MainGUI::SimulParameters::MakeSliderFloats()
{
    auto sliderflags = ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic | \
                           ImGuiSliderFlags_NoInput | ImGuiSliderFlags_NoRoundToFormat;
    
    #define ALLSLIDERS() XMACRO(momentumTransfer) XMACRO(momentumDistribution)
    
    #define FMTSTRING(field) #field": %."precision()"f"
    
    #define XMACRO(field) ImGui::SliderFloat(#field, &this->field, \
        0.f, 1.f, FMTSTRING(field), sliderflags);
    
    #define precision() "3"
    
    ALLSLIDERS();
    
    #undef ALL
    #undef FMTSTRING
    #undef XMACRO
    #undef precision
    ImGui::SliderFloat("momentumTransfer",     &this->momentumTransfer,     0.f, 1.f, "momentumTransfer"": %.""3""f",     sliderflags);
    ImGui::SliderFloat("momentumDistribution", &this->momentumDistribution, 0.f, 1.f, "momentumDistribution"": %.""3""f", sliderflags);
    
    const float min = 0.0f;
    const float max = 1.0f;
    float v[2] = {this->momentumTransfer, this->momentumDistribution};
    ImGui::SliderFloat2( "SimulationParams", v, min, max, "%.3f", ImGuiSliderFlags_NoRoundToFormat );
    
    return;
} */


// SLIDER MACROS //
// #define ALLSLIDERS()
#define FMTSTRING(field) #field ": %." PRECISION() "f"
// FMTSTRING gets concatenated into a single string. ('name: %.3f')
// the spaces are actually necessary; C++11 requires a space between literal and string macro

// local variables 'min', 'max', and 'sliderflags' are expected to exist
#define XMACRO(field) ImGui::SliderFloat(#field, &PSTRUCT()->REAL(field), \
    min, max, FMTSTRING(field), sliderflags)

// optional arg can be additional callbacks/hooks to execute on slider interation
#define MAKESLIDER(field, ...) if(XMACRO(field)) { \
    lastactiveSlider = &PSTRUCT()->REAL(field);    \
    __VA_OPT__(__VA_ARGS__;)                       \
}

// use the slider macros by defining input macros like this: 
/* 
    #define PSTRUCT() SimulParams
    #define REAL(field) momentum##field
    #define PRECISION() "3" 
    MAKESLIDER(Transfer)
*/
// 'XMACRO' expects local variables 'min', 'max', and 'sliderflags' to exist
// the 'REAL' macro can be used to add a prefix to the field-name if it doesn't match the struct-member
// (in the example, it would construct: 'SimulParams->momentumTransfer' instead: of 'SimulParams->Transfer')




void MainGUI::DrawSimulParams(float& next_height)
{
    ImGui::Begin("Simulation Parameters", nullptr, subwindow_flags^ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowPos({0, next_height});
    ImGui::SetWindowSize({m_width, -1});  // -1 retains current size
    
    auto sliderflags = ImGuiSliderFlags_NoRoundToFormat | \
      ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput;
    
    // unfortunately negative timescales don't work properly
    if (ImGui::SliderFloat("Timescale", &timestepMultiplier, 0.001f, 2.0f, "%.3fx", sliderflags))
        lastactiveSlider = &timestepMultiplier;
    ImGui::SeparatorText("Momentum");
    
    #define PSTRUCT() SimulParams
    #define REAL(fieldname) momentum##fieldname
    #define PRECISION() "3"
    // you can define these as normal variables, but 'PRECISION' just cannot
    float min = 0.0f;
    float max = 1.0f;
    
    MAKESLIDER(Transfer) 
    MAKESLIDER(Distribution)
    //ImGui::SliderFloat("Transfer",     &SimulParams->momentumTransfer,     min, max, "Transfer: %.3f",     sliderflags); 
    //ImGui::SliderFloat("Distribution", &SimulParams->momentumDistribution, min, max, "Distribution: %.3f", sliderflags);
    
    #undef PSTRUCT
    #undef REAL
    #undef PRECISION
    
    //next_height += 50.f;
    next_height += ImGui::GetWindowHeight(); // 'GetWindowHeight' returns height of current section
    ImGui::End();
}


void MainGUI::DrawFluidParams(float& next_height)
{
    if (!FluidParams) return;  // TODO: give some kind of indicator that it's been invalidated
    //ImGui::Separator();
    ImGui::Begin("Fluid Parameters", nullptr, subwindow_flags^ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowPos({0, next_height});
    int numlines = 1;
    
    sf::Color enabledColor = {0x1A, 0xEE, 0x22, 0xFF}; // green
    sf::Color disableColor = {0xFF, 0x20, 0x20, 0xFF}; // red
    
    if (SimulParams->hasGravity) {
        ImGui::Text("Gravity:");  ImGui::SameLine();
        ImGui::TextColored(enabledColor, "Enabled");
    } else {
        ImGui::TextColored(sf::Color{0x5F, 0x5F, 0x5F, 0xFF}, "Gravity:");
        ImGui::SameLine();   ImGui::TextColored(disableColor, "Disabled");
    }
    
    if (SimulParams->hasXGravity) {
        ImGui::Text("XGravity:");  ImGui::SameLine();
        ImGui::TextColored(enabledColor, "Enabled");
    } else {
        ImGui::TextColored(sf::Color{0x5F, 0x5F, 0x5F, 0xFF}, "XGravity:");
        ImGui::SameLine();   ImGui::TextColored(disableColor, "Disabled");
    }
    
    auto sliderflags = ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat\
                     | ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput;
    // input is disabled because it's also activated with 'Tab' (normally Ctrl+click);
    // which you're likely to hit accidentally when trying to activate mouse
    
    // you can define these as normal variables, but 'precision' just cannot
    float min = -3.0f;
    float max =  3.0f;
    
    #define PSTRUCT() FluidParams
    #define REAL(fieldname) fieldname
    #define PRECISION() "3"
    // gravity sliders //
    MAKESLIDER(gravity , SimulParams->hasGravity  = true;);
    MAKESLIDER(xgravity, SimulParams->hasXGravity = true;);
    
    // fdensity / viscosity sliders //
    #undef PRECISION
    #define PRECISION() "6"
    min = -0.5f;
    max =  0.5f;
    MAKESLIDER(viscosity);
    min = 0.001f; // fdensity should not go below zero
    MAKESLIDER(fdensity);
    
    #undef PRECISION
    #define PRECISION() "2"
    min = -2.0f;
    max =  2.0f;
    sliderflags ^= ImGuiSliderFlags_Logarithmic; // negating logarithmic flag
    MAKESLIDER(bounceDampening);
    
    // TODO: button to reset defaults
    
    #undef PSTRUCT
    #undef REAL
    #undef PRECISION
    
    numlines += 3;
    ImGui::SeparatorText("Gradient Speed-Thresholds");
    ImGui::SliderFloat("Min", &Fluid::gradient_thresholdLow, 0.0f, 50.f, "%.2f", sliderflags);
    ImGui::SliderFloat("Max", &Fluid::gradient_thresholdHigh, 0.f, 75.f, "%.2f", sliderflags);
    
    numlines += 1;
    ImGui::Separator();
    ImGui::Text("Turbulence:"); ImGui::SameLine();
    if (FluidParams->realptr->isTurbulent) 
         ImGui::TextColored({0xFF, 0x00, 0x08, 0xFF}, "Enabled" );
    else ImGui::TextColored({0x00, 0x40, 0xFF, 0xFF}, "Disabled");
    ImGui::Separator();
    
    // for some reason this can't autosize correctly. // Note: titlebar also takes ~25 pixels
    ImGui::SetWindowSize({m_width, 25.0f*numlines});
    next_height += 25.0f*numlines;
    ImGui::End();
    //ImGui::Separator();
    return;
}


void MainGUI::DrawMouseParams(float& next_height)
{
    auto sliderflags = ImGuiSliderFlags_NoRoundToFormat | \
      ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput;
    
    sf::Color enabledColor = {0x1A, 0xEE, 0x22, 0xFF}; // green
    sf::Color disableColor = {0xFF, 0x20, 0x20, 0xFF}; // red
    // creates an enabled/disabled line of text from a bool, with colors
    [[maybe_unused]] auto BooleanText = [&, enabledColor, disableColor] (const char* label, bool B)
    {
        if(B) { 
            ImGui::TextColored(enabledColor, "%s", label); 
            ImGui::SameLine(); ImGui::Text("Enabled" ); }
        else  {
            ImGui::TextColored(disableColor, "%s", label);
            ImGui::SameLine(); ImGui::TextDisabled("Disabled"); 
            }
    };
    
    // applies the colors in the opposite order
    [[maybe_unused]] auto BooleanTextFlip = [&, enabledColor, disableColor] (const char* label, bool B)
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
    auto ModeColor = [](const Mouse_T::Mode& m) -> ImVec4 {
        switch(m) {
            case (Mouse_T::Mode::None):     return sf::Color{0xAA, 0xAA, 0xAA, 0xCC};
            case (Mouse_T::Mode::Push):     return sf::Color::Cyan;
            case (Mouse_T::Mode::Pull):     return sf::Color::Magenta;
            case (Mouse_T::Mode::Disabled): return sf::Color{0xAA, 0xAA, 0xAA, 0x77};
            
            default:
            case (Mouse_T::Mode::Drag):
            case (Mouse_T::Mode::Fill):
            case (Mouse_T::Mode::Erase):
            return {0xFF, 0xFF, 0xFF, 0xFF};
        }
    };
    
    sf::Color modeColor = ModeColor(this->MouseParams->mode);
    ImGui::PushStyleColor(ImGuiCol_Border, modeColor - sf::Color{0x22222222});
    ImGui::PushStyleColor(ImGuiCol_TitleBg, modeColor - sf::Color{0x20202077});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, modeColor - sf::Color{0x111111BB});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, modeColor - sf::Color{0x222222BB});
    ImGui::PushStyleColor(ImGuiCol_Separator, modeColor - sf::Color{0x20202020});
    
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
    
    ImGui::Text("Painted:%lu", numlocked+numUnlocked); ImGui::SameLine();
    ImGui::Text("unlocked:%lu", numUnlocked);          ImGui::SameLine();
    ImGui::Text("locked:%lu", numlocked); // '%lu' == long unsigned int
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
    
    static bool shouldDrawGrid_old;
    if(ImGui::SliderFloat("Strength", &MouseParams->strength, 1.f, 255.f, "%f", sliderflags))
    {
        shouldDrawGrid = true;
        MouseParams->realptr->RecalculateModDensities();
        lastactiveSlider = &MouseParams->strength;
    }
    if(ImGui::IsItemDeactivatedAfterEdit()) { shouldDrawGrid = shouldDrawGrid_old; } // waiting until slider is released
    else if(!ImGui::IsItemActive()) { shouldDrawGrid_old = shouldDrawGrid; }
    
    next_height += ImGui::GetWindowHeight();
    ImGui::End();
    return;
}


// SLIDER MACROS //
#undef FMTSTRING
#undef XMACRO
#undef MAKESLIDER


void MainGUI::FrameLoop(std::vector<sf::Keyboard::Key>& unhandled_keypresses) 
{
    if (!isEnabled || !isOpen()) { return; }
    sf::RenderWindow::clear();
    if (dockedToMain) FollowMainWindow();
    
    HandleWindowEvents(unhandled_keypresses);
    
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
    
    // Simulation Parameters
    DrawSimulParams(next_height);
    DrawFluidParams(next_height);
    DrawMouseParams(next_height);
    // TODO: demo-window has very little space left!
    
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
