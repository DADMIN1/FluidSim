#include "Gradient.hpp"
#include <SFML/Window.hpp>  // defines sf::Event


/* -------------------------------- Gradient_T Implementations -------------------------------- */
#ifdef GRADIENT_T_FWDDECLARE
  #ifdef GRADIENT_T_FWDDECLARE_STATIC
  // definitions for the static forward-declaration implementation
  
  constexpr unsigned char Gradient_T::GradientRaw[1024][3] = {
        #define EMBED_GRADIENT // removes the declaration syntax from the definition
        #include "GradientRaw.cpp"
        #undef EMBED_GRADIENT
  };
  
  const sf::Color Gradient_T::Lookup(const unsigned int index) 
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
      
      static const sf::Color Lookup(const unsigned int index) 
      {return sf::Color(GradientRaw[index][0], GradientRaw[index][1], GradientRaw[index][2]);}
  };

  #endif // FWDDECLARE_STATIC
#endif // FWDDECLARE

// Note: the full class-definition is toggled (inverse) by the STATIC flag, not the base FWDDECLARE
// because if FWDDECLARE was undefined, the full class definition was already embedded in the header


/* ------------------------------------- GradientWindow_T ------------------------------------- */
void GradientWindow_T::Initialize()
{
    m_texture.create(m_width, m_height);
    for (std::size_t i{0}; i<1024; ++i) {
        sf::RectangleShape band(sf::Vector2f{1, 127});
        band.setFillColor(m_gradient->Lookup(i));
        band.setPosition(i,0);
        m_texture.draw(band);
    }
    m_texture.display();
    m_sprite = sf::Sprite(m_texture.getTexture()); // must call .display before constructing sprite
    return;
}


GradientWindow_T::~GradientWindow_T()  // TODO: call RenderWindow destructor??
{
    if (ownsPtr) delete m_gradient;
}


void GradientWindow_T::Create(bool useVsync, int xposition)
{
    // sf::Style::None
    // for some reason it's only letting me use auto here
    constexpr auto m_style = sf::Style::Close;  // Title-bar is implied (for Style::Close)
    // sf::Style::Default = Titlebar | Resize | Close
    sf::RenderWindow::create(sf::VideoMode(m_width, m_height), "Gradient", m_style);
    setVerticalSyncEnabled(useVsync);
    setPosition({xposition+360, 360});
    return;
}


void GradientWindow_T::FrameLoop()
{
    if (!isOpen()) { return; }
    // only drawing it once; never cleared
    sf::RenderWindow::clear();
    sf::RenderWindow::draw(m_sprite);
    sf::RenderWindow::display();
    
    if (hasFocus())
    {
        sf::Event gw_event;
        while (pollEvent(gw_event)) {
            switch (gw_event.type) 
            {
                case sf::Event::Closed:
                    close(); return;
                case sf::Event::KeyPressed:
                {
                    switch (gw_event.key.code) {
                        case sf::Keyboard::Key::Q:
                        case sf::Keyboard::Key::Escape:
                        case sf::Keyboard::F2:
                            close(); return;
                        default: break;
                    }
                }
                default: break;
            }
        }
        // no need to redraw
    }
    return;
}



