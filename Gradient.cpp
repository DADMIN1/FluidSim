#include "Gradient.hpp"


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

