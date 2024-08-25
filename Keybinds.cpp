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
    bool trailing_newline{true};  // inserts an empty line after each keybind (when printed)
    
    // maybe 'Tag' would work better than 'Section'?
    enum Section {
        unspecified,
        important,
        debug,
        interactions,
        GradientEditor,
        shaders,
        enum_count, // encodes the 'size' of this enum list. Always last (obviously)
        // 'enum_count' is also used to indicate 'invalid' or 'default'
    } section;
    // this relies on the auto-assignment of enum values; any manual assignments will break it
    static constexpr auto numsections{Section::enum_count};
};

// Keybind a = {.keycode = sf::Keyboard::Key::A, .name = "", .description = ""};
// Keybind b = {sf::Keyboard::Key::B, "", ""};

struct AllKeybinds
{
    static constexpr auto numkeybinds{28};
    std::vector<Keybind> all;  // TODO: array instead?
    std::vector<std::vector<Keybind*>> sections; 
    
    // note that 'current_section' should never be set to 'enum_count'; 
    // that would cause the 'KEY' macro (below) to segfault (OOB index into 'sections')
    Keybind::Section current_section {Keybind::Section::unspecified};
    bool trailing_newline{true};
    
    #define KEY(key, description) \
    [[maybe_unused]] Keybind& keybind_##key = all.emplace_back(sf::Keyboard::Key::key, #key, description, "", trailing_newline, current_section); \
    sections[current_section].emplace_back(&keybind_##key)
    // ^ this emplace back (WITHOUT a semicolon) allows a dereference off the 'KEY' macro itself
    
    AllKeybinds()
    {
        all.reserve(numkeybinds); // if you don't call reserve, 'extrainfo' may (usually will) mysteriously disappear
        sections.resize(Keybind::numsections);
        for (auto& section: sections) { section.reserve(numkeybinds); }
        // reserving 'numkeybinds' for each section is really overkill, but it could make sense if section-enums are rewritten as bitflags
        
        current_section = Keybind::Section::important;
        KEY(F1, "print these keybinds");
        KEY(Q,  "Exits the program");
        KEY(F2, "open the gradient window")
            -> extrainfo = "close it with Q or F2 again";
        KEY(Tilde, "open the side-panel")
            ->extrainfo = "(while open) switches focus between side-panel and main-window\n"
            "  - Left/Right: switch docking side of side-panel\n"
            "  - Q/ESC: close side-panel\n";
        
        current_section = Keybind::Section::interactions;
        KEY(Space, "pause/unpause simulation");
        KEY(BackSpace, "freeze particles")
            -> extrainfo = "all velocities are zeroed. (it also pauses the simulation)";
        KEY(R, "Reset the simulation");
        KEY(G, "toggle gravity \n(+Shift):  xgravity");
        KEY(Tab, "toggle mouse interactions");
        KEY(P, "toggle painting-mode");
        KEY(K, "clear painted cells (painting mode)");
        KEY(C, "toggle cell-grid display");
        KEY(T, "toggle turbulence-mode")
            -> extrainfo = "modifies physics calculations to encourage perpetual motion";
        KEY(Y, "toggle particle transparency");
        KEY(U, "toggle particle-scaling direction (positive/negative)");
        current_section = Keybind::Section::unspecified;
        //KEY(Subtract, "");
        KEY(Add, "increment/decrement: ~0.4-1.5% (+Shift: 4x-faster)")
            -> extrainfo = "+/- Keys control the last active slider in GUI";
        KEY(N, "print mouse position");
        
        current_section = Keybind::Section::GradientEditor;
        KEY(Left, "change selection") -> name = "Left/Right Arrowkeys";
        { KEY(Space, "lock selection"); } // TODO: prevent redefinition errors
        KEY(L, "toggle color-editor lock")
            -> extrainfo = "prevents color-editor from updating when selection changes";
        
        KEY(Escape, "deselect all points/segments") -> name = "ESC/R-Click";
        
        // Shaders //
        current_section = Keybind::Section::shaders;
        trailing_newline = false;
        // TODO: generate these calls from the keymap in Shaders
        KEY(Num0, "Shader: 'empty'");
        KEY(Num1, "Shader: 'brighter'");
        KEY(Num2, "Shader: 'darker'");
        KEY(Num3, "Shader: 'red'");
        KEY(Num4, "Shader: 'cherry_blossoms'");
        KEY(Num5, "Shader: 'turbulence'")
            -> extrainfo = "When 'turbulence' is the active shader, this key will toggle also toggle framebuffer-clears during the frame-loop\n"
            "    note: while this shader is active and the simulation is paused, the framebuffer will always be cleared\n" 
            "          (so that the effects of the 'threshold' parameter can be observed)\n"
            "  activating this shader automatically enables Turbulence-mode\n";
        
        KEY(Pause, "(Pause-Break): toggle framebuffer-clears") // Pause-Break
            -> extrainfo = "while the 'turbulence' shader is active and the simulation is paused, the framebuffer will always be cleared\n"
            " (so that the effects of the 'threshold' parameter can be observed)\n";
        trailing_newline = true;
        
        keybind_Tab.extrainfo = "(while the mouse is enabled, it will be displayed as a circle)\n\n"
            "  Mouse-interactions: (hold)\n"
            "      Left-Click  = Push (increases density)\n"
            "      Right-Click = Pull (negative density)\n"
            "    ScrollWheel resizes effect radius\n"
            "    Side-buttons clear any painted areas (in painting mode)\n";
        // just abusing string concatenation for more visually-intuitive formatting
        
        keybind_P.extrainfo = "in painting-mode, mouse-interactions stay active over traveled areas\n"
                            "     (until mouse-button is released)\n"
                            "  - While actively painting, clicking the opposite mouse-button will lock the painted cells.\n"
                            "  - Use the mouse's side-buttons to clear any locked cells. (Or hit 'K')";
        
        //keybind_D.extrainfo = "the cell-grid visualizes the local density of small regions";
        
    }
    #undef KEY
} Keybinds{};


std::string CreateSectionHeader(Keybind::Section S)
{
    constexpr std::string_view dividerline = "------------------------------------\n";
    std::string header{dividerline};
    std::string_view section_name;
    
    #define INSERT_DASHES() \
    header.append(space_padding_count, ' '); \
    header.append(flanking_dash_count, '-'); \
    header.append(space_padding_count, ' ');
    
    switch (S) 
    {
        default:                              section_name = "invalid";      break;
        case Keybind::Section::unspecified:   section_name = "unspecified";  break;
        case Keybind::Section::interactions:  section_name = "interactions"; break;
        case Keybind::Section::important:     section_name = "important";    break;
        case Keybind::Section::debug:         section_name = "debug";        break;
        case Keybind::Section::GradientEditor:section_name = "Gradient Editor"; goto fallthrough;
        case Keybind::Section::shaders:       section_name = "shaders";
        fallthrough:
            // TODO: convert section_names to uppercase
            auto remaining_length = dividerline.length() - section_name.length();
            assert((remaining_length > 8) && "Section name is too long");
            // spaces inserted between the name, edge, and flanking dashes
            constexpr auto space_padding_count = 2;
            auto flanking_dash_count = (remaining_length/2) - (space_padding_count*2);
            header.clear();
            header.append("\n");
            header.append(flanking_dash_count, '-'); 
            header.append(section_name); 
            header.append(flanking_dash_count, '-');
            //header.append(section_name); header.append("\n");
            //header.append(dividerline);
        break;
    }
    #undef INSERT_DASHES
    return header;
}


void PrintKeybinds()
{
    std::cout << "\n\n";
    std::cout << "------------------------------------\n";
    std::cout << "    --------  KEYBINDS  --------    \n";
    std::cout << "------------------------------------\n";
    std::cout << '\n';
    
    Keybind::Section previous_section = Keybind::Section::enum_count;
    
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
        if (kb.section != previous_section) {
            if (previous_section != Keybind::Section::enum_count)
            std::cout << CreateSectionHeader(kb.section) << '\n';
            previous_section = kb.section;
        }
        
        const std::size_t alignment { (kb.name.length()+1) % 4 }; // +1 accounting for ": "
        const std::size_t paddinglength { (alignment == 0)? 0 : 4-alignment };
        
        //const int paddinglength = alignment - kb.name.length() - 2; // accounting for ": "
        //if (paddinglength < 0) { std::cerr << "alignment is too short for: " << kb.name << '\n'; continue; }
        std::string padding = std::string(paddinglength, ' ');
        
        std::cout << kb.name << ": " << padding << kb.description << '\n';
        if (kb.extrainfo.length() > 0) {
            //for (std::size_t s{0}; s<kb.name.length(); ++s) { std::cout << " "; }
            std::cout << "  " << kb.extrainfo << '\n';
        }
        if (kb.trailing_newline) std::cout << '\n';
    }
    
    std::cout << '\n';
    std::cout << "------------------------------------\n";
    std::cout << "    ------  END KEYBINDS  ------    \n";
    std::cout << "------------------------------------\n";
    std::cout << "\n\n";
    return;
}
