#ifndef FLUIDSIM_VALARRAYTEST_INCLUDED
#define FLUIDSIM_VALARRAYTEST_INCLUDED

#include <valarray>
#include <iostream>
#include <cassert>


// https://en.cppreference.com/w/cpp/numeric/valarray/operator_arith2
void PrintValarray(std::string_view rem, auto const& v, bool nl = false)
{
    if constexpr (std::is_scalar_v<std::decay_t<decltype(v)>>) {  // what is this block for??
        std::cout << "took first if statement\n";  // currently, never prints
        std::cout << rem << ": " << v;
    }
    else {
        constexpr size_t linebreakthreshold = 8;
        std::string_view spacing = " ";
        std::string_view undofinalspacing = "\b\b ";  // deleting trailing comma lol
        
        std::cout << rem << ": {";
        if (v.size() > linebreakthreshold) { 
            spacing = "\n    ";
            undofinalspacing = "\b\b\b\b\n ";
        }
        std::cout << spacing;
        for (std::size_t itercount=0; auto const e : v) {
            std::cout << e << ", ";
            if ((++itercount < v.size()) && (itercount % linebreakthreshold) == 0) { std::cout << spacing; } 
            /* the additional length-check is required to prevent an extra newline 
                from occuring on the last line when the size of the array is divisible by the threshold
                (for some reason you can't just backspace over it) */
        }
        std::cout << undofinalspacing;
        std::cout << "}";
    }
    std::cout << (nl ? ";\n" : ";  ");
}


// 'true' for newline
#define SIMPLEPRINTVA(V) PrintValarray(#V, V, true)
#define PRINTVALARRAY(V, ...) PrintValarray(#V, V __VA_OPT__(= __VA_ARGS__))
#define PRINTVALARRAYOP(...) PrintValarray(#__VA_ARGS__, __VA_ARGS__, true)


void ValarrayExample()
{
    std::cout << "--Valarray Example--\n";
    std::valarray<int> x, y;
    //#define SETVALS(A, B) x=A; y=B; PRINTVALARRAY(x), PRINTVALARRAY(y)
    //SETVALS(({1, 2, 3}), ({4, 5, 6}));
    // doesn't work; parenthesis are required for grouping but then the syntax is invalid?
    // "expected ';' before '}' token"
    
    PRINTVALARRAY(x, {1,2,3,4}), PRINTVALARRAY(y, {4,3,2,1}), PRINTVALARRAYOP(x += y);
    PRINTVALARRAY(x, {4,3,2,1}), PRINTVALARRAY(y, {3,2,1,0}), PRINTVALARRAYOP(x -= y);
    PRINTVALARRAY(x, {1,2,3,4}), PRINTVALARRAY(y, {5,4,3,2}), PRINTVALARRAYOP(x *= y);
    PRINTVALARRAY(x, {1,3,4,7}), PRINTVALARRAY(y, {1,1,3,5}), PRINTVALARRAYOP(x &= y); //bitwise and
    PRINTVALARRAY(x, {1,3,4,7}), PRINTVALARRAY(y, {1,1,3,5}), PRINTVALARRAYOP(x |= y); //bitwise or
    PRINTVALARRAY(x, {1,3,4,7}), PRINTVALARRAY(y, {1,1,3,5}), PRINTVALARRAYOP(x ^= y); //bitwise xor
    PRINTVALARRAY(x, {0,1,2,4}), PRINTVALARRAY(y, {4,3,2,1}), PRINTVALARRAYOP(x <<=y);
    std::cout << '\n';
    PRINTVALARRAY(x, {1,2,3,4}), PRINTVALARRAYOP(x += 5);
    PRINTVALARRAY(x, {1,2,3,4}), PRINTVALARRAYOP(x *= 2);
    PRINTVALARRAY(x, {8,6,4,2}), PRINTVALARRAYOP(x /= 2);
    PRINTVALARRAY(x, {8,4,2,1}), PRINTVALARRAYOP(x >>=1);
    std::cout << '\n' << '\n';
}


void ValarrayTest()
{
    // can't be constexpr because brace-enclosed-initializer list copies
    std::valarray<int> testarr {
        1, 2, 3, 4, 5, 6, 7, 8,
        2, 2, 3, 4, 5, 6, 7, 8,
        3, 2, 3, 4, 5, 6, 7, 8,
        4, 2, 3, 4, 5, 6, 7, 8,
    };
    std::valarray<int> testarrtwo(8, 32);
    
    /* std::cout << "testarr size = " << testarr.size() << '\n';
    std::cout << "testarrtwo size = " << testarrtwo.size() << '\n'; */
    assert((testarr.size() == testarrtwo.size()) && "mismatched valarray sizes is undefined behavior");

    std::cout << '\n';
    SIMPLEPRINTVA(testarr);
    SIMPLEPRINTVA(testarrtwo);
    std::cout << "testarr sum = " << testarr.sum() << '\n';
    std::cout << "testarrtwo sum = " << testarrtwo.sum() << '\n';
    
    testarrtwo += testarr;
    std::cout << "combined sum = " << testarr.sum() << '\n' << '\n';
    SIMPLEPRINTVA(testarr);
    SIMPLEPRINTVA(testarrtwo);
    std::cout << '\n' << '\n';
}


#endif
