#ifndef FLUIDSIM_GRADIENT_INCLUDED
#define FLUIDSIM_GRADIENT_INCLUDED

#include <array>
#include <SFML/Graphics/Color.hpp>

//#define GRADIENT_T_FWDDECLARE
//#define GRADIENT_T_FWDDECLARE_STATIC


#ifdef GRADIENT_T_FWDDECLARE
struct Gradient_T;

/* forward-declaring the class-members as static avoids embedding the huge array (GradientRaw) into the header.
  This also allows the (external) definition of 'GradientRaw' to be constexpr, even though the declaration isn't 
  (because specifying constexpr always requires a definition) */
  // seems like compile-times aren't really affected by embedding the GradientRaw directly
  #ifdef GRADIENT_T_FWDDECLARE_STATIC
  struct Gradient_T
  {
      static const unsigned char GradientRaw[1024][3];
      std::array<sf::Color, 1024> gradientdata{};
      const sf::Color& Lookup(const unsigned int index) const {return gradientdata[index];}
      static const sf::Color LookupDefault(const unsigned int index); // lookup performed on GradientRaw instead
      Gradient_T() { for (std::size_t i{0}; i<1024; ++i) { gradientdata[i] = LookupDefault(i); } }
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
    
    inline static const sf::Color LookupDefault(const unsigned int index) 
    {return sf::Color(GradientRaw[index][0], GradientRaw[index][1], GradientRaw[index][2]);}
    
    std::array<sf::Color, 1024> gradientdata{};
    const sf::Color& Lookup(const unsigned int index) const {return gradientdata[index];}
    
    Gradient_T() { for (std::size_t i{0}; i<1024; ++i) { gradientdata[i] = LookupDefault(i); } }
};

#endif //FWDDECLARE


#endif  // header guard
