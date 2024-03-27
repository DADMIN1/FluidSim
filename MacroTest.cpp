#include <iostream>
#include <array>


void EmbedMacroTest()
{
    std::cout << "\n\n";
    // Normal include (defines GradientRaw)
    #include "GradientRaw.cpp"
    const auto& rawNormal = GradientRaw;
    std::cout << int(GradientRaw[1][2]) << '\n';  // defined by include
    std::cout << int(rawNormal[1][2]) << '\n';
    // Note that you have to (explicitly) convert to int if you want to print the numbers
    std::cout << "\n\n";
    
    // Embed include
    constexpr unsigned char rawEmbed[1024][3] = {
        #define EMBED_GRADIENT
        #include "GradientRaw.cpp"
    };
    std::cout << int(rawEmbed[1][2]) << '\n';
    std::cout << "\n\n";
    
    // unfortunately, range-for-loops refuse to deduce the types, 
    // so we have to macro the 'triplet' definition onto every line.
    // if you use unsigned char for triplet instead, you'll have to explicity convert to int later
    using triplet = std::array<unsigned int, 3>;
    using gradientArrT = std::array<triplet, 1024>;
    constexpr gradientArrT epic {
        #define EMBED_TYPECAST triplet  // required for conversion to std::arrays
        #define EMBED_GRADIENT
        #include "GradientRaw.cpp"
    };
    for (auto [r,g,b]: epic)
    {
        std::cout << r << ' ' << g << ' ' << b << '\n';
        // explicit conversion required if unsigned char
        // std::cout << int(r) << ' ' << int(g) << ' ' << int(b) << '\n';
    }
    std::cout << "\n\n";
    return;
}
