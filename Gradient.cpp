#include "Gradient.hpp"
#include "GradientWindow.hpp"

#include <SFML/Window.hpp>  // defines sf::Event
#include <imgui.h>
#include <imgui-SFML.h>


/* -------------------------------- Gradient_T Implementations -------------------------------- */
#ifdef GRADIENT_T_FWDDECLARE
  #ifdef GRADIENT_T_FWDDECLARE_STATIC
  // definitions for the static forward-declaration implementation
  
  constexpr unsigned char Gradient_T::GradientRaw[1024][3] = {
        #define EMBED_GRADIENT // removes the declaration syntax from the definition
        #include "GradientRaw.cpp"
        #undef EMBED_GRADIENT
  };
  
  const sf::Color Gradient_T::LookupDefault(const unsigned int index)
  {return sf::Color(GradientRaw[index][0], GradientRaw[index][1], GradientRaw[index][2]);}

  #else // FWDDECLARE_STATIC

  // Full class definition
  struct Gradient_T
  {
      static constexpr unsigned char GradientRaw[1024][3] = {
        #define EMBED_GRADIENT // removes the declaration syntax from the definition
        #include "GradientRaw.cpp"
        #undef EMBED_GRADIENT
      };
      
      static const sf::Color LookupDefault(const unsigned int index) 
      {return sf::Color(GradientRaw[index][0], GradientRaw[index][1], GradientRaw[index][2]);}
      
      std::array<sf::Color, 1024> gradientdata{};
      const sf::Color& Lookup(const unsigned int index) const {return gradientdata[index];}
      
      Gradient_T() { for (std::size_t i{0}; i<1024; ++i) { gradientdata[i] = LookupDefault(i); } }
  };

  #endif // FWDDECLARE_STATIC
#endif // FWDDECLARE

// Note: the full class-definition is toggled (inverse) by the STATIC flag, not the base FWDDECLARE
// because if FWDDECLARE was undefined, the full class definition was already embedded in the header


/* ------------------------------------- GradientWindow ------------------------------------- */


// Main.cpp
extern bool usingVsync;


bool GradientWindow::Initialize(int xposition)
{
    if(isOpen()) { return false; } //should not call initialize twice
    stored_xposition = xposition;
    AdjustPosition();
    
    if(!current_gradient) return false;
    for(const GradientView& gv : gradientViews) {
        if(!gv.m_gradient) return false;
    }
    
    if(!ImGui::SFML::Init(*this)) return false;
    ImGuiIO& imguiIO = ImGui::GetIO();
    
    imguiIO.ConfigInputTrickleEventQueue      = false; // spreads interactions (like simultaneous keypresses) across multiple frames
    imguiIO.ConfigInputTextEnterKeepActive    = true ; // keep input focused after hitting enter
    //imguiIO.ConfigWindowsResizeFromEdges      = false; // can be annoying, and it requires BackendFlags_HasMouseCursors anyway
    imguiIO.ConfigWindowsMoveFromTitleBarOnly = true ;
    
    imguiIO.ConfigFlags = ImGuiConfigFlags {
        ImGuiConfigFlags_None
        | ImGuiConfigFlags_NavEnableKeyboard     // Master keyboard navigation enable flag. Enable full Tabbing + directional arrows + space/enter to activate.
        | ImGuiConfigFlags_NavNoCaptureKeyboard  // Instruct navigation to not set the io.WantCaptureKeyboard flag when io.NavActive is set.
        | ImGuiConfigFlags_NoMouseCursorChange   // Prevent imgui from altering mouse visibility/shape
        //| ImGuiConfigFlags_IsSRGB // Application is SRGB-aware. NOT used by core Dear ImGui (only used by backends, maybe)
    };
    
    imguiIO.BackendFlags = ImGuiBackendFlags_HasMouseCursors;  // required for 'ResizeFromEdges'
    imguiIO.IniFilename = NULL; // disable autosaving of the 'imgui.ini' config file (just stores window states)
    
    return true;
}


void GradientWindow::Create()
{
    if(isOpen()) { return; }
    constexpr auto m_style = sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close;
    sf::RenderWindow::create(sf::VideoMode(GradientNS::windowWidth, GradientNS::windowHeight), "GradientWindow [FLUIDSIM]", m_style);
    setVerticalSyncEnabled(usingVsync);
    AdjustPosition();
    setVisible(isEnabled);
    clock.restart();
    return;
}

// only used by the keybind in main, and in the eventloop below
void GradientWindow::ToggleEnabled()
{
    if (!sf::RenderWindow::isOpen()) {
        GradientWindow::Create();
        isEnabled = false; // gets toggled back to true
    }
    isEnabled = !isEnabled;
    sf::RenderWindow::setVisible(isEnabled);
    AdjustPosition();
    return;
}


void GradientWindow::EventLoop()
{
    sf::Event gw_event;
    while (sf::RenderWindow::pollEvent(gw_event)) 
    {
        ImGui::SFML::ProcessEvent(*this, gw_event);
        switch (gw_event.type) 
        {
            case sf::Event::Closed:
                isEnabled = false;
                sf::RenderWindow::close();
            break;
            
            case sf::Event::KeyPressed:
            {
                switch (gw_event.key.code) {
                    case sf::Keyboard::Key::Escape:
                    case sf::Keyboard::Key::Q:
                    case sf::Keyboard::F2:
                        isEnabled = false;
                        sf::RenderWindow::close();
                    break;
                    
                    /* case sf::Keyboard::F2:
                        ToggleEnabled();
                    break; */
                    
                    default: break;
                }
            }
            break;
            
            // prevents contents from scaling/moving on resize
            case sf::Event::Resized:
            {
                auto [newwidth, newheight] = gw_event.size;
                sf::View newview {sf::RenderWindow::getView()};
                newview.setViewport({0, 0, float{float(GradientNS::windowWidth)/newwidth}, float{float(GradientNS::windowHeight)/newheight}});
                sf::RenderWindow::setView(newview);
            }
            break;
            
            default: break;
        }
    }
    
    return;
}


void GradientWindow::FrameLoop()
{
    if(!isEnabled || !isOpen()) { return; }
    
    sf::RenderWindow::setActive(); // required; otherwise closing the window completely blacks-out the mainwindow
    ImGui::SFML::Update(*this, clock.restart()); // calls ImGui::NewFrame()
    EventLoop();
    
    // performing SFML 'draw'-calls before 'Update' also activates the current context (sometimes)
    sf::RenderWindow::clear();
    sf::RenderWindow::draw(gradientViews[0]);
    sf::RenderWindow::draw(gradientViews[1]);
    
    DisplayTestWindows();
    
    ImGui::SFML::Render(*this);
    sf::RenderWindow::display();
    return;
}



