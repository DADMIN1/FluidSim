#include <iostream>
#include <vector>
#include <cassert>

#include <SFML/Window/Event.hpp>  // sf::Keyboard::Key


struct Keybind
{
    sf::Keyboard::Key keycode;
    //sf::Keyboard::Key modifiers;
    std::string name;
    std::string description;
    std::string extrainfo;
    /* enum Section {
        important,
        general,
        debug,
    } section; */
};

// Keybind a = {.keycode = sf::Keyboard::Key::A, .name = "", .description = ""};
// Keybind b = {sf::Keyboard::Key::B, "", ""};

struct AllKeybinds
{
    static constexpr auto numkeybinds{18};
    std::vector<Keybind> all;
    
    #define KEY(key, description) \
    [[maybe_unused]] Keybind& keybind_##key = all.emplace_back(sf::Keyboard::Key::key, #key, description);
    
    AllKeybinds()
    {
        all.reserve(numkeybinds); // if you don't call reserve, 'extrainfo' may (usually will) mysteriously disappear
        KEY(F1, "print these keybinds");
        KEY(Q, "Exits the program");
        KEY(Space, "pause/unpause simulation");
        KEY(BackSpace, "freeze particles");
        KEY(G, "toggle gravity");
        KEY(Tab, "toggle mouse interactions");
        KEY(P, "toggle painting-mode");
        KEY(C, "toggle cell-grid display");
        KEY(T, "toggle fluid turbulence");
        KEY(Y, "toggle particle transparency");
        KEY(N, "print mouse position");
        KEY(F2, "open the gradient-viewing window");
        // Shaders //
        KEY(Num0, "Shader: 'empty'");
        KEY(Num1, "Shader: 'brighter'");
        KEY(Num2, "Shader: 'darker'");
        KEY(Num3, "Shader: 'red'");
        KEY(Num4, "Shader: 'cherry_blossoms'");
        KEY(Num5, "Shader: 'turbulence'");
        
        keybind_F2.extrainfo = "close it with ESC, Q, or F2 again";
        
        keybind_BackSpace.extrainfo = "all velocities are zeroed. (it also pauses the simulation)";
        
        keybind_Tab.extrainfo = "(while the mouse is enabled, the currently-hovered cell will be outlined)\n\n"
            "  Mouse-interactions: (hold)\n"
            "      Left-Click  = Push (increases density)\n"
            "      Right-Click = Pull (negative density)\n"
            "    ScrollWheel resizes effect radius\n";
        // just abusing string concatenation for more visually-intuitive formatting
        
        keybind_P.extrainfo = "in painting-mode, mouse-interactions stay active over traveled areas\n"
                            "  (until mouse-button is released)";
        
        //keybind_D.extrainfo = "the cell-grid visualizes the local density of small regions";
        keybind_C.extrainfo = "the cell-grid will always be (temporarily) displayed when drawing in painting-mode";
        
    }
    #undef KEY
} Keybinds{};


void PrintKeybinds()
{
    std::cout << "\n\n";
    std::cout << "------------------------------------\n";
    std::cout << "    --------  KEYBINDS  --------    \n";
    std::cout << "------------------------------------\n";
    std::cout << '\n';
    
    // if you don't reserve the correct number of keybinds, their 'extrainfo' will mysteriously disappear
    if (Keybinds.all.size() > Keybinds.numkeybinds)
    {
        std::cerr << "============  WARNING  =============\n";
        std::cerr << " you need to reserve more keybinds;\n"
                  << " output is likely to be incomplete\n";
        std::cerr << "   reserved: " << Keybinds.numkeybinds << '\n';
        std::cerr << "   entries: " << Keybinds.all.size() << '\n';
        std::cerr << "============  WARNING  =============\n\n";
    }
    assert((Keybinds.all.size() <= Keybinds.numkeybinds) && "you need to reserve more keybinds");
    // TODO: it would be nice to somehow check this with a static_assert instead
    
    //const int alignpoint {12}; // should be greater than longest string by at least 3;
    for (const Keybind& kb: Keybinds.all)
    {
        const std::size_t alignment { (kb.name.length()+1) % 4 }; // +1 accounting for ": "
        const std::size_t paddinglength { (alignment == 0)? 0 : 4-alignment };
        
        //const int paddinglength = alignment - kb.name.length() - 2; // accounting for ": "
        //if (paddinglength < 0) { std::cerr << "alignment is too short for: " << kb.name << '\n'; continue; }
        std::string padding{""};
        for (std::size_t s{0}; s<paddinglength; ++s) { padding.append(" "); }
        
        std::cout << kb.name << ": " << padding << kb.description << '\n';
        if (kb.extrainfo.length() > 0) {
            //for (std::size_t s{0}; s<kb.name.length(); ++s) { std::cout << " "; }
            std::cout << "  " << kb.extrainfo << '\n';
        }
        std::cout << '\n';
    }
    
    std::cout << "------------------------------------\n";
    std::cout << "\n\n";
    return;
}
