#include "GradientWindow.hpp"

#include "imgui.h"


// setting up constexpr C-strings for widget identifiers (imgui functions require 'const char*' parameters)
template<int C> constexpr static char num_to_char {static_cast<char>(0x30+C)}; // ascii numbers start at 0x30
template<int C> constexpr static char column_c_str[8] {'C','O','L','U','M','N', num_to_char<C>, '\0'};

constexpr static const std::array<const char*, 8> columns {
    column_c_str<1>, column_c_str<2>, column_c_str<3>, column_c_str<4>,
    column_c_str<5>, column_c_str<6>, column_c_str<7>, column_c_str<8>,
};

// Access/re-open columns like this: 'ImGui::Begin(columns[N])'
// Alternatively, just Macro everything in. (only possible with literal num, not variable)
#define INTO_COLUMN(num, ...) \
    ImGui::Begin("COLUMN" #num); \
        __VA_OPT__(__VA_ARGS__); \
    ImGui::End();


//-------------------------------------------------------//
// Comptime conversions from sf::Color to vec4 of floats //
// imgui's color-widgets only accept 'float*' parameters //
//-------------------------------------------------------//

// bitshift example/testing
// repeatedly bitshift 8-right (then downcast) to sequentially isolate 8-bit segments of an int32/64
static_assert((0x12345678 >> 0x08) == 0x00123456);
static_assert((0x12345678 >> 0x10) == 0x00001234);
static_assert((0x12345678 >> 0x18) == 0x00000012);
static_assert((0x12345678 >> 0x1F) == 0x00000000); // not 0x20? (compiler warns 'out-of-range'?)


// Using bitshifts to isolate components of an sf::Color for comptime conversions
template<typename T, sf::Uint32 sfcolor> // integer representation of sf::Color (because sf::Color class is not constexpr)
consteval std::array<T,4> ColorToVec4() {
    constexpr sf::Uint8 A {static_cast<sf::Uint8>(sfcolor >> 0x00)}; // notice that the fields are defined backwards
    constexpr sf::Uint8 B {static_cast<sf::Uint8>(sfcolor >> 0x08)};
    constexpr sf::Uint8 G {static_cast<sf::Uint8>(sfcolor >> 0x10)};
    constexpr sf::Uint8 R {static_cast<sf::Uint8>(sfcolor >> 0x18)};
    return {static_cast<T>(R), static_cast<T>(G), static_cast<T>(B),static_cast<T>(A)};
}

// ImGui color-widgets only accept normalized floats
template<sf::Uint32 sfcolor> float Vec4Color[4] {
    ColorToVec4<float,sfcolor>()[0] / 255.0f,
    ColorToVec4<float,sfcolor>()[1] / 255.0f,
    ColorToVec4<float,sfcolor>()[2] / 255.0f,
    ColorToVec4<float,sfcolor>()[3] / 255.0f,
};

/* internally, ImGui stores it's color-variables as (static) ImVec4,
    but that requires a C-style cast on every function call: 
    `ImGui::ColorEdit3("MyColor", (float*)&color);` 
    Instead, I just use C-style arrays, which implicitly convert to 'float*'
*/

template<int C> constexpr static char color_c_str[7] {'C', 'O', 'L', 'O', 'R', num_to_char<C>, '\0'};
constexpr static const std::array<const char*, 8> colornames {
    color_c_str<1>, color_c_str<2>, color_c_str<3>, color_c_str<4>,
    color_c_str<5>, color_c_str<6>, color_c_str<7>, color_c_str<8>,
};

// allocating 10 seperate instances of Vec4Color (directly using Vec4Color with equivalent values would reference the same instance)
constexpr static const std::array<float* const, 8> colors {
    Vec4Color<0xFFFFFF00>, Vec4Color<0xFFFFFF01>, Vec4Color<0xFFFFFF02>, Vec4Color<0xFFFFFF03>,
    Vec4Color<0xFFFFFF04>, Vec4Color<0xFFFFFF05>, Vec4Color<0xFFFFFF06>, Vec4Color<0xFFFFFF07>,
};

static std::array<bool, 8> ColorNeedsDefault { true, true, true, true, true, true, true, true, };

// Creates a widget and sets it's initial color
#define IMGUICOLORWIDGET(Widgetclass, number, colorval, ...) \
{ \
    static auto& colorvar {colors[number]}; \
    bool& shouldSetDefault {ColorNeedsDefault[number]}; \
    if(shouldSetDefault) { shouldSetDefault = false; \
        colorvar[0] = Vec4Color<colorval>[0]; \
        colorvar[1] = Vec4Color<colorval>[1]; \
        colorvar[2] = Vec4Color<colorval>[2]; \
        colorvar[3] = Vec4Color<colorval>[3]; \
    } \
    Widgetclass( #Widgetclass "##" #number , \
        colorvar __VA_OPT__(, __VA_ARGS__) \
    ); \
}
/* Because the color-pointer is determined by number, any two widgets with matching numbers will have their colors linked (even across windows)
 In addition, ImGui will "physically" link two widgets if they share an identifier (physically-linked widgets can still maintain seperate color-values)
 "Physically-linked" - Meaning that interactions will apply to both simultaneously (including opening multiple context menus)
 This cannot occur across (ImGui) windows, since each identifier is seeded with the current window's ID
 Sharing a color between two widgets of the same type (within the same window) will have the side-effect of physically-linking them due to matching identifiers
 As a workaround, pass one of the numbers as an expression: '2' and '1+1', for example */


// from imgui_demo.cpp - line#226
// Helper to display a little (?) mark which shows a tooltip when hovered.
namespace ImGui { static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip(); 
    }
}}


static bool demoToggleDisplayed{true};
void DisplayDemoToggle()
{
    static bool showDemoWindow{false};
    constexpr int FIXED_X_POSITION{1024}; // previously 0
    
    //float next_height = GradientNS::headSpace;
    float next_height = 0;
    ImGui::SetNextWindowSizeConstraints({512, -1}, {512, -1});
    ImGui::SetNextWindowSize({GradientNS::pixelCount, -1});
    ImGui::SetNextWindowPos({FIXED_X_POSITION, next_height});
    
    // must be called before the 'End' call (for GetWindowHeight)
    #define NEXTPOSITION \
      next_height += ImGui::GetWindowHeight(); \
      ImGui::SetNextWindowSizeConstraints({512, 0.0f}, {512, GradientNS::windowHeight}); \
      ImGui::SetNextWindowSize({GradientNS::pixelCount, -1}, ImGuiCond_Appearing); \
      ImGui::SetNextWindowPos({ImGui::GetWindowPos().x, next_height}, ImGuiCond_Appearing);
    
    if(demoToggleDisplayed){
        ImGui::Begin("DemoToggle", &demoToggleDisplayed);
        if(ImGui::Button("Demo Button")) { showDemoWindow = !showDemoWindow; }
        NEXTPOSITION;
        ImGui::End();
    } else { showDemoWindow = false; }
    
    static constexpr ImGuiWindowFlags demowindow_flags {
        ImGuiWindowFlags_None 
        | ImGuiWindowFlags_MenuBar
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoSavedSettings  // Never load/save settings in .ini file
    };
    
    if(showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow, demowindow_flags);
        ImGui::SetWindowPos ("Dear ImGui Demo", {FIXED_X_POSITION, next_height}, ImGuiCond_Appearing); // selecting window by name
        ImGui::SetWindowSize("Dear ImGui Demo", {512, -1}, ImGuiCond_Appearing); 
    }
    
    return;
}


void GradientWindow::DisplayTestWindows()
{
    // setting up multiple windows: ("column1", "column2", etc)
    constexpr int numcolumns{4};
    for(int N{0}; N < numcolumns; ++N)
    {
        constexpr float halfwidth = GradientNS::pixelCount/numcolumns;
        
        if(N >= 2){
            float hspace = GradientNS::headSpace;
            if(N == 3) {hspace = GradientNS::bandHeight;} // fourth column is beyond the end of gradient
            ImGui::Begin(columns[N]);
            ImGui::SetWindowPos({(N+N-2)*halfwidth, hspace}, ImGuiCond_Once);
            ImGui::SetWindowSize({halfwidth*2, GradientNS::windowHeight-hspace}, ImGuiCond_Once);
            ImGui::End();
        } else {
            //if(demoToggleDisplayed) continue;
            ImGui::Begin(columns[N]);
            ImGui::SetWindowPos({N*halfwidth, GradientNS::headSpace}, ImGuiCond_Once);
            ImGui::SetWindowSize({halfwidth, GradientNS::windowHeight-GradientNS::headSpace}, ImGuiCond_Once);
            ImGui::End();
        }
    }
    
    
    //if(!demoToggleDisplayed) {
        INTO_COLUMN ( 1,
            IMGUICOLORWIDGET(ImGui::ColorPicker4, 0, 0x1122FF00);
            ImGui::Separator();
            IMGUICOLORWIDGET(ImGui::ColorEdit4, 1, 0x00FF6400);
        );
        
        INTO_COLUMN ( 2,
            // names only determine whether the widgets are physically linked (user interactions applied to both simultaneously)
            // these widgets are still 'linked' because they use the same pointer (Vec4Color)
            ImGui::SeparatorText("Linked Color");
            ImGui::ColorEdit4(colornames[3], Vec4Color<0xFF006400>);
            ImGui::ColorEdit4(colornames[4], Vec4Color<0xFF006400>);
            
            ImGui::SeparatorText("Linked Physically");
            ImGui::ColorEdit4(colornames[2], Vec4Color<0x00FF00FF>);
            ImGui::ColorEdit4(colornames[2], Vec4Color<0xFF006400>); // and by pointer
            ImGui::SameLine(); ImGui::Text("+Colorlink");
            
            ImGui::SeparatorText("Linked Across Columns");
            ImGui::ColorEdit4("ColorPicker", colors[0]); // linked to Column1 colorpicker
            ImGui::ColorEdit4("ColorLink2" , colors[1]);
            // linking physically across columns is not possible because the window-ID is always included in the hash
        );
    //}
    
    // flag states to be used by checkboxes
    constexpr int numColorEditFlags{23};
    static std::array<std::tuple<bool, ImGuiColorEditFlags_, const char*>, numColorEditFlags> ImguiColorFlags
    {{
        { false, ImGuiColorEditFlags_NoAlpha        , "NoAlpha"        }, // ColorEdit, ColorPicker, ColorButton: ignore Alpha component (will only read 3 components from the input pointer).
        { false, ImGuiColorEditFlags_NoPicker       , "NoPicker"       }, // ColorEdit: disable picker when clicking on color square.
        { false, ImGuiColorEditFlags_NoOptions      , "NoOptions"      }, // ColorEdit: disable toggling options menu when right-clicking on inputs/small preview.
        { false, ImGuiColorEditFlags_NoSmallPreview , "NoSmallPreview" }, // ColorEdit, ColorPicker: disable color square preview next to the inputs. (e.g. to show only the inputs)
        { false, ImGuiColorEditFlags_NoInputs       , "NoInputs"       }, // ColorEdit, ColorPicker: disable inputs sliders/text widgets (e.g. to show only the small preview color square).
        { false, ImGuiColorEditFlags_NoTooltip      , "NoTooltip"      }, // ColorEdit, ColorPicker, ColorButton: disable tooltip when hovering the preview.
        { false, ImGuiColorEditFlags_NoLabel        , "NoLabel"        }, // ColorEdit, ColorPicker: disable display of inline text label (the label is still forwarded to the tooltip and picker).
        { false, ImGuiColorEditFlags_NoSidePreview  , "NoSidePreview"  }, // ColorPicker: disable bigger color preview on right side of the picker, use small color square preview instead.
        { false, ImGuiColorEditFlags_NoDragDrop     , "NoDragDrop"     }, // ColorEdit: disable drag and drop target. ColorButton: disable drag and drop source.
        { false, ImGuiColorEditFlags_NoBorder       , "NoBorder"       }, // ColorButton: disable border (which is enforced by default)
        
        // index == 10
        // User Options (right-click on widget to change some of them).
        { false, ImGuiColorEditFlags_AlphaBar        , "AlphaBar"         },  // ColorEdit, ColorPicker: show vertical alpha bar/gradient in picker.
        { false, ImGuiColorEditFlags_AlphaPreview    , "AlphaPreview"     },  // ColorEdit, ColorPicker, ColorButton: display preview as a transparent color over a checkerboard, instead of opaque.
        { false, ImGuiColorEditFlags_AlphaPreviewHalf, "AlphaPreviewHalf" },  // ColorEdit, ColorPicker, ColorButton: display half opaque / half checkerboard, instead of opaque.
        { false, ImGuiColorEditFlags_HDR             , "HDR"              },  // (WIP) ColorEdit: Currently only disable 0.0f..1.0f limits in RGBA edition (note: you probably want to use ImGuiColorEditFlags_Float flag as well).
        { false, ImGuiColorEditFlags_DisplayRGB      , "DisplayRGB"       },  // ColorEdit: override _display_ type among RGB/HSV/Hex. ColorPicker: select any combination using one or more of RGB/HSV/Hex.
        { false, ImGuiColorEditFlags_DisplayHSV      , "DisplayHSV"       },  // 
        { false, ImGuiColorEditFlags_DisplayHex      , "DisplayHex"       },  // 
        { false, ImGuiColorEditFlags_Uint8           , "Uint8"            },  // ColorEdit, ColorPicker, ColorButton: _display_ values formatted as 0..255.
        { false, ImGuiColorEditFlags_Float           , "Float"            },  // ColorEdit, ColorPicker, ColorButton: _display_ values formatted as 0.0f..1.0f floats instead of 0..255 integers. No round-trip of value via integers.
        { false, ImGuiColorEditFlags_PickerHueBar    , "PickerHueBar"     },  // ColorPicker: bar for Hue, rectangle for Sat/Value.
        { false, ImGuiColorEditFlags_PickerHueWheel  , "PickerHueWheel"   },  // ColorPicker: wheel for Hue, triangle for Sat/Value.
        { false, ImGuiColorEditFlags_InputRGB        , "InputRGB"         },  // ColorEdit, ColorPicker: input and output data in RGB format.
        { false, ImGuiColorEditFlags_InputHSV        , "InputHSV"         },  // ColorEdit, ColorPicker: input and output data in HSV format.
    }};
    //static_assert(std::get<1>(flags[10]) == ImGuiColorEditFlags_AlphaBar);
    
    // maintains a reference to the active (checkbox) boolean for each group of exclusive flags
    static std::array<std::pair<bool*, ImGuiColorEditFlags_>, 4> ImguiColorFlagMasks 
    {{
        {nullptr, ImGuiColorEditFlags_DisplayMask_ },
        {nullptr, ImGuiColorEditFlags_DataTypeMask_},
        {nullptr, ImGuiColorEditFlags_PickerMask_  },
        {nullptr, ImGuiColorEditFlags_InputMask_   },
    }};
    
    
    ImGuiColorEditFlags selectedflags { ImGuiColorEditFlags_None };
    ImGui::Begin("COLUMN3"); ImGui::SeparatorText("ColorEditFlags");
    if (ImGui::BeginTable("ColorFlagTable", 3))
    {
        for(int i{0}; i < numColorEditFlags; ++i)
        {
            bool isExclusive{false};
            auto& [boolvar, flagval, name] {ImguiColorFlags[i]};
            
            for (auto& [boolptr, mask]: ImguiColorFlagMasks) {
                if(mask & flagval) {
                    isExclusive = true;
                    if(boolvar) { boolptr = &boolvar; break; }
                    else { // only disable the checkbox if there's actually an active conflict
                        if(!boolptr) { isExclusive = false; break; }
                        isExclusive = *boolptr;
                    }
                }
            }
            
            if(boolvar) { selectedflags |= flagval; }
            ImGui::BeginDisabled(isExclusive && !boolvar); // 
            ImGui::TableNextColumn(); ImGui::Checkbox(name, &boolvar);
            ImGui::EndDisabled();
        }
        ImGui::EndTable();
    }
    ImGui::End(); //column 3
    
    
    // references for flag-masks (from ImGui)
    /* constexpr ImGuiColorEditFlags DefaultOptions_ = ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueBar;
    constexpr ImGuiColorEditFlags DisplayMask_    = ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_DisplayHex;
    constexpr ImGuiColorEditFlags DataTypeMask_   = ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_Float;
    constexpr ImGuiColorEditFlags PickerMask_     = ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_PickerHueBar;
    constexpr ImGuiColorEditFlags InputMask_      = ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_InputHSV;
    ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview; */
    
    //if((selectedflags & ImGuiColorEditFlags_DisplayMask_ ) == 0) selectedflags |= ImGuiColorEditFlags_DefaultOptions_ & ImGuiColorEditFlags_DisplayMask_;
    //if((selectedflags & ImGuiColorEditFlags_DataTypeMask_) == 0) selectedflags |= ImGuiColorEditFlags_DefaultOptions_ & ImGuiColorEditFlags_DataTypeMask_;
    //if((selectedflags & ImGuiColorEditFlags_PickerMask_  ) == 0) selectedflags |= ImGuiColorEditFlags_DefaultOptions_ & ImGuiColorEditFlags_PickerMask_;
    //if((selectedflags & ImGuiColorEditFlags_InputMask_   ) == 0) selectedflags |= ImGuiColorEditFlags_DefaultOptions_ & ImGuiColorEditFlags_InputMask_;
    //static_assert(((DefaultOptions_|ImGuiColorEditFlags_PickerMask_) ^ ImGuiColorEditFlags_PickerHueBar  ) == (ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueWheel));
    //static_assert(((DefaultOptions_|ImGuiColorEditFlags_PickerMask_) ^ ImGuiColorEditFlags_PickerHueWheel) == (ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueBar));
    
    // need to create each combination of selectedflags + colorpicker-type (wheel/bar), but NOT BOTH TYPES (fails assertion)
    // however, simply performing an xor won't work if the flag isn't already applied
    // instead, merge BOTH flags in first; '| PickerMask', and then xor out the excluded one
    ImGuiColorEditFlags pickerflags1 = ((selectedflags | ImGuiColorEditFlags_PickerMask_) ^ (std::get<0>(ImguiColorFlags[20])? ImGuiColorEditFlags_PickerHueBar   : ImGuiColorEditFlags_PickerHueWheel));
    ImGuiColorEditFlags pickerflags2 = ((selectedflags | ImGuiColorEditFlags_PickerMask_) ^ (std::get<0>(ImguiColorFlags[19])? ImGuiColorEditFlags_PickerHueWheel : ImGuiColorEditFlags_PickerHueBar  ));
    
    
    ImGui::Begin("COLUMN4"); ImGui::SeparatorText("Applied Flags");
    ImGui::Separator();
    IMGUICOLORWIDGET(ImGui::ColorEdit3, 5, 0x05F7FBCF, selectedflags); ImGui::Separator();
    IMGUICOLORWIDGET(ImGui::ColorEdit4, 6, 0x05F7FBCF, selectedflags);
    ImGui::SameLine(); ImGui::HelpMarker(
        "With the ImGuiColorEditFlags_NoInputs flag you can hide all the slider/text inputs.\n"
        "With the ImGuiColorEditFlags_NoLabel flag you can pass a non-empty label which will only "
        "be used for the tooltip and picker popup."
    );
    
    ImGui::Spacing();
    
    ImGui::SeparatorText("ColorPickers");
    
    static bool isLayoutHorizontal{true};
    ImGui::Checkbox("Horizontal layout", &isLayoutHorizontal);
    
    float width = ImGui::GetContentRegionAvail().x;
    if(isLayoutHorizontal ) {
        width *= 0.5f;
        pickerflags1 |= ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview;  // these two absolutely destroy horizontal spacing
        pickerflags2 |= ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview;
        ImGui::SetNextItemWidth(width);
    }
    
    IMGUICOLORWIDGET(ImGui::ColorPicker4,   7, 0x7788FFAA, pickerflags1);
    if(isLayoutHorizontal) { ImGui::SameLine(); ImGui::SetNextItemWidth(width); }
    IMGUICOLORWIDGET(ImGui::ColorPicker4, 8-1, 0x7788FFAA, pickerflags2);
    // passing '7' and '8-1' is a ridiculous hack to get two (different) widgets pointing to the same color, without physically linking them
    
    ImGui::Separator();
    ImGui::End(); //column 4
    
    if(demoToggleDisplayed) { DisplayDemoToggle(); }
    
    return;
}
