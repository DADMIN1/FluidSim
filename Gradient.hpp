#ifndef FLUIDSIM_GRADIENT_INCLUDED
#define FLUIDSIM_GRADIENT_INCLUDED

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderWindow.hpp>


#define GRADIENT_T_FWDDECLARE
#define GRADIENT_T_FWDDECLARE_STATIC


#ifdef GRADIENT_T_FWDDECLARE
struct Gradient_T;

/* forward-declaring the class-members as static avoids embedding the huge array (GradientRaw) into the header.
  This also allows the (external) definition of 'GradientRaw' to be constexpr, even though the declaration isn't 
  (because specifying constexpr always requires a definition) */
  #ifdef GRADIENT_T_FWDDECLARE_STATIC
  struct Gradient_T
  {
      static const unsigned char GradientRaw[1024][3];
      static const sf::Color Lookup(const unsigned int index);
  };
  #endif // FWDDECLARE_STATIC

#else  //FWDDECLARE
// Full class definition in the header
struct Gradient_T
{
    inline static constexpr unsigned char GradientRaw[1024][3] = {
      #define EMBED_GRADIENT // removes the declaration syntax from the definition
      #include "GradientRaw.cpp"
      #undef EMBED_GRADIENT
    };
    
    inline static const sf::Color Lookup(const unsigned int index) 
    {return sf::Color(GradientRaw[index][0], GradientRaw[index][1], GradientRaw[index][2]);}
};

#endif //FWDDECLARE


class GradientWindow_T: public sf::RenderWindow
{
    Gradient_T* m_gradient;
    sf::RenderTexture m_texture;
    sf::Sprite m_sprite;
    static constexpr int m_width {1024}; //pixels
    static constexpr int m_height {127};
    
    const bool ownsPtr; // prevents the destructor from deleting pointers it does not own
    void Initialize();  // setup steps shared by both constructors
    
    public:
    // constructors do not actually create the window; call '.create()' later
    GradientWindow_T(): sf::RenderWindow(), 
      m_gradient{new Gradient_T()}, ownsPtr{true}
    { Initialize(); }
    
    GradientWindow_T(Gradient_T* const gradientPtr): sf::RenderWindow(), 
      m_gradient{gradientPtr}, ownsPtr{false}
    { Initialize(); }
    
    void Create(); // calls sf::RenderWindow.create(...) with some arguments
    void FrameLoop(); // returns after gradient_window is closed
    ~GradientWindow_T();
    // gcc warns if the destructor is defined here (due to deleting an incomplete type)
    // but it's fine if the definition is provided in the .cpp file instead
};

#endif  // header guard
