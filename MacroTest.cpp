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


// -- Start Range Tests  -- //

#include <vector>
#include <ranges>


void ModuloTest()
{
    std::vector<std::pair<int,int>> pairs = {
        {3 , 5}, {-3,  5},
        {5 , 3}, { 5, -3},
        {-7, 4}, {-7, -4},
        {-5, 3}, {-5, -3},
    };
    
    std::vector<int> regular_results{};
    std::vector<int>  proper_results{};
    std::cout << "\n----regular modulo----\n";
    for (auto [x, y]: pairs) {
        std::cout << "{" << x << "," << y << "}\t= ";
        int result = regular_results.emplace_back(x % y);
        std::cout << result << "\n";
    }
    std::cout <<  '\n';
    
    std::cout << "\n----proper modulo----\n";
    for (auto [x, y]: pairs) {
        std::cout << "{" << x << "," << y << "}\t= ";
        int result = proper_results.emplace_back(((x%y)+y)%y);
        std::cout << result << "\n";
    }
    std::cout <<  '\n';
    
    const auto zippedResults = std::ranges::zip_view(pairs, regular_results, proper_results);
    for (auto [input, resultr, resultp]: zippedResults)
    {
        for (auto [x, y]: {input})
        {
            std::cout << "{ " << x << " % " << y << " } \t\t=  " << resultr << "\n";
            std::cout << "{ (("<<x<<"%"<<y<<")+"<<y<<")%"<<y<<" } \t=  " << resultp << "\n";
            if (resultr != resultp) {
                std::cout << "mismatch!\n";
            }
        }
        std::cout << "\n";
    }
    
    std::cout <<  '\n';
    std::cout <<  '\n';
    return;
}



//#define PRINTCOMPTIMECOORDS
#ifndef PRINTCOMPTIMECOORDS // function can't be constexpr if it's printing anything
constexpr 
#endif
auto ComptimeCoords() // generates all relative coordinates for a given radial-distance
{
    constexpr int RD = 7;
    // iota_view with negative numbers is not symmetric (endpoint is not included)
    // it's correct to add one anyway, otherwise the range of values would be RD-1
    constexpr auto xaxis = std::ranges::iota_view{-RD, RD+1};
    constexpr auto inverseRD = std::views::transform(xaxis, [](const int i) {
        int insanemod {(RD-(i%RD))%-RD}; // don't ask
        return 
            (i==0)?  RD : // sets midpoint to RD instead of zero
            ((i<0)? (RD-insanemod)%RD : insanemod);
            //     the '%RD' here: ^ just sets the first number to 0 (instead of RD)
    });
    
    static_assert(xaxis.size()  == inverseRD.size());
    
    #ifdef PRINTCOMPTIMECOORDS
    for (auto x: xaxis) {
        std::cout << x << ",\t";
    }
    std::cout <<  '\n';
    for (auto mod: inverseRD) {
        std::cout << mod << ",\t";
    }
    std::cout <<  '\n';
    std::cout <<  '\n';
    #endif
    
    // pairs of coords inverted (negative) on one axis (and matching along the other)
    // inverting x instead of y is functionally equivalent, and 
    constexpr auto MakeCoordPair = [](const int x, const int y) {
        return std::pair{ std::pair{x,y}, std::pair{x,-y} };
    };
    
    constexpr auto coords = std::ranges::zip_transform_view(MakeCoordPair, xaxis, inverseRD);
    
    #ifdef PRINTCOMPTIMECOORDS
    for (const auto& [firstcoord, secondcoord]: coords) {
        for (const auto& [x, y]: {firstcoord, secondcoord}) {
            std::cout << "{ " << x << " , " << y << " }\t";
        }
        std::cout <<  '\n';
    }
    // surprised the structured binding in the inner loop works
    // it won't allow structured binding ([x,y]) to a std::pair without the braces, by the way
    // apparently because it doesn't inherit from std::initializer_list????
    std::cout << "\n\n\n";
    #endif
    
    return coords;
}



// verifies that the constexpr-version of ComptimeCoords actually works
void PrintComptimeCoords()
{
    #ifndef PRINTCOMPTIMECOORDS
    constexpr
    #endif
    auto coords = ComptimeCoords();
    for (const auto& [firstcoord, secondcoord]: coords) {
        for (const auto& [x, y]: {firstcoord, secondcoord}) {
            std::cout << "{ " << x << " , " << y << " }\t";
        }
        std::cout <<  '\n';
    }
}

