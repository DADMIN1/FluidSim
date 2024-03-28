#include <iostream>
#include <vector>

#include <SFML/Window/Event.hpp>  // sf::Keyboard::Key


struct Keybind
{
    sf::Keyboard::Key keycode;
    //sf::Keyboard::Key modifiers;
    std::string name;
    std::string description;
    std::string extrainfo;
    enum Section {
        important,
        general,
        debug,
    } section;
};

// Keybind a = {.keycode = sf::Keyboard::Key::A, .name = "", .description = ""};
// Keybind b = {sf::Keyboard::Key::B, "", ""};

struct AllKeybinds
{
    static constexpr auto numkeybinds{9};
    std::vector<Keybind> all;
    
    #define KEY(key, description) \
    [[maybe_unused]] Keybind& keybind_##key = all.emplace_back(sf::Keyboard::Key::key, #key, description);
    
    AllKeybinds()
    {
        all.reserve(numkeybinds); // if you don't reserve it, 'extrainfo' may mysteriously disappear
        KEY(Q, "Exits the program");
        KEY(F1, "print these keybinds");
        KEY(Space, "pause/unpause simulation");
        KEY(BackSpace, "freeze particles");
        KEY(G, "toggle gravity");
        KEY(T, "toggle particle transparency");
        KEY(M, "toggle mouse interaction");
        KEY(N, "print mouse position");
        KEY(F2, "open the gradient-viewing window");
        
        keybind_F2.extrainfo = "close it with ESC, Q, or F2 again";
        keybind_BackSpace.extrainfo = "velocities are all set to 0\n"
                         "             also pauses the simulation";
    }
    #undef KEY
} Keybinds{};


void PrintKeybinds()
{
    if(Keybinds.all.size() > Keybinds.numkeybinds) {
        std::cerr << "you need to reserve more keybinds\n";
        std::cerr << "output may be incomplete\n";
    }
    
    std::cout << "\n\n";
    std::cout << "------------------------------------\n";
    std::cout << "    --------  KEYBINDS  --------    \n";
    std::cout << "------------------------------------\n";
    std::cout << '\n';
    
    //const int alignpoint {12}; // should be greater than longest string by at least 3;
    for (const Keybind& kb: Keybinds.all)
    {
        const std::size_t alignment { (kb.name.length()+2) % 4 }; // +2 accounting for ": "
        const std::size_t paddinglength { (alignment == 0)? 0 : 4-alignment };
        
        //const int paddinglength = alignment - kb.name.length() - 2; // accounting for ": "
        //if (paddinglength < 0) { std::cerr << "alignment is too short for: " << kb.name << '\n'; continue; }
        std::string padding;
        for (std::size_t s{0}; s<paddinglength; ++s) { padding.append(" "); }
        
        std::cout << kb.name << ": " << padding << kb.description << '\n';
        if (kb.extrainfo.length() > 0) {
            for (std::size_t s{0}; s<kb.name.length(); ++s) { std::cout << " "; }
            std::cout << "    " << kb.extrainfo << '\n';
        }
        std::cout << '\n';
    }
    
    std::cout << "------------------------------------\n";
    std::cout << "\n\n";
    return;
}
