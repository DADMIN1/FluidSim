#ifndef FLUIDSYM_SLIDER_INCLUDED
#define FLUIDSYM_SLIDER_INCLUDED

#include <imgui.h>
#include <imgui-SFML.h>


// SliderFlags enum from Imgui.h //

// Flags for DragFloat(), DragInt(), SliderFloat(), SliderInt() etc.
// We use the same sets of flags for DragXXX() and SliderXXX() functions as the features are the same and it makes it easier to swap them.
// (Those are per-item flags. There are shared flags in ImGuiIO: io.ConfigDragClickToInputText)
/* enum ImGuiSliderFlags_
{
    ImGuiSliderFlags_None                   = 0,
    ImGuiSliderFlags_AlwaysClamp            = 1 << 4,       // Clamp value to min/max bounds when input manually with CTRL+Click. By default CTRL+Click allows going out of bounds.
    ImGuiSliderFlags_Logarithmic            = 1 << 5,       // Make the widget logarithmic (linear otherwise). Consider using ImGuiSliderFlags_NoRoundToFormat with this if using a format-string with small amount of digits.
    ImGuiSliderFlags_NoRoundToFormat        = 1 << 6,       // Disable rounding underlying value to match precision of the display format string (e.g. %.3f values are rounded to those 3 digits)
    ImGuiSliderFlags_NoInput                = 1 << 7,       // Disable CTRL+Click or Enter key allowing to input text directly into the widget
    ImGuiSliderFlags_InvalidMask_           = 0x7000000F,   // [Internal] We treat using those bits as being potentially a 'float power' argument from the previous API that has got miscast to this enum, and will trigger an assert if needed.

    // Obsolete names
    //ImGuiSliderFlags_ClampOnInput = ImGuiSliderFlags_AlwaysClamp, // [renamed in 1.79]
}; */


struct Slider
{
    static Slider* lastactive;
    
    const char* const name;   // text displayed on/beside slider
    const char* const format; // printf-style; formatting displayed value (heldvar)
    float* const heldvar;
    const float originalValue;
    void (*Callback)() = [](){ return; }; // triggered on interaction
    const float min{0.0f};
    const float max{1.0f};
    const int sliderflags;
    
    constexpr static int defaultflags =
      ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat\
    | ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput;
    // input is disabled because it's also activated with 'Tab' (normally Ctrl+click);
    // which you're likely to hit accidentally when trying to activate mouse
    
    // xor-ing with ImGuiSliderFlags_None (0) will not change the value
    static_assert(defaultflags == (defaultflags xor ImGuiSliderFlags_None));
    
    // implicit boolean-conversions provides declaration-syntax conforming to imgui constructors (inline within if-statement)
    operator bool()   { return ImGui::SliderFloat(name, heldvar, min, max, format, sliderflags); }
    void Activate()   { lastactive = this;        Callback(); }
    void operator()() { if(this->operator bool()) Activate(); } // (implicitly) invokes the imgui constructor/object
    // manually-constructed Sliders (not using the Macros) must invoke either operator to actually create the imgui object
    
    // TODO: create the buttonText string inside function
    bool ResetButton(const char* buttonText) {
        ImGui::SameLine();
        bool returnval = ImGui::Button(buttonText);
        if(returnval) { *heldvar = originalValue; /* Callback(); */ }
        return returnval;
    }
    
    // TODO: pass callback as parameter
    Slider(const char* namestr, float* const varptr, float minp=0.0f, float maxp=1.0f, 
           const char* ffmtstr = {": %.3f"}, int xor_sliderflags=ImGuiSliderFlags_None)
    : name{namestr}, format{ffmtstr}, heldvar{varptr}, originalValue{*varptr},
      min{minp}, max{maxp}, sliderflags {defaultflags xor xor_sliderflags}
    { ; }
    
};


#endif // FLUIDSYM_SLIDER_INCLUDED
