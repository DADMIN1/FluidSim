#ifndef FLUIDSIM_GRADIENTWINDOW_INCLUDED
#define FLUIDSIM_GRADIENTWINDOW_INCLUDED

#include "Gradient.hpp"

#include <array>
#include <list>  // GradientView::segments
//#include <memory> //smart-ptr
//#include <iterator>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderWindow.hpp>


namespace GradientNS {
    constexpr int pixelCount{1024};
    constexpr int bandHeight  {32};
    constexpr int headSpace{bandHeight*2 + 1};  // vertical space reserved for the first two gradients
    constexpr int windowWidth{1536};
    //constexpr int windowHeight{360};
    constexpr int windowHeight{640};
};


struct GradientView: sf::Drawable
{
    const Gradient_T& sourceRef;
    sf::RenderTexture m_texture;
    sf::Sprite m_sprite;
    
    // implementing the SFML 'draw' function for this class
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override 
    { target.draw(m_sprite, states); }
    
    void RedrawTexture(bool useOriginal=false) // parameter selects between 'LookupDefault' and 'Lookup'
    {
        m_texture.clear(sf::Color::Transparent);
        sf::RectangleShape colorband(sf::Vector2f{1, GradientNS::bandHeight});
        for (std::size_t i{0}; i<GradientNS::pixelCount; ++i) {
            colorband.setFillColor(useOriginal? sourceRef.LookupDefault(i) : sourceRef.Lookup(i));
            colorband.setPosition(i,0);
            m_texture.draw(colorband);
        }
        m_texture.display(); // must call .display before constructing sprite
        return;
    }
    
    GradientView(const Gradient_T& gradientRef, bool useOriginal=false)
        : sf::Drawable{}, sourceRef{gradientRef}, m_texture{}, m_sprite{}
    {
        m_texture.create(GradientNS::pixelCount, GradientNS::bandHeight);
        RedrawTexture(useOriginal);
        m_sprite = sf::Sprite(m_texture.getTexture());
    }
};


// TODO: implement draggable-interaction
// TODO: color-modifying controls
// TODO:    logic for splitting / joining segments
// TODO: controls for splitting / joining segments

class GradientEditor
{
    friend class GradientWindow;
    
    Gradient_T m_gradient{};
    GradientView viewCurrent{m_gradient};
    GradientView viewWorking{m_gradient};
    GradientView viewOverlay{m_gradient};
    
    struct Segment 
    {
        enum Index: int { Head = -1, Tail = -2, Held = -3 } const segNum;
        int  color_index; //into gradient
        sf::Color* color;
        sf::RectangleShape vertical_outline;
        
        float Xposition() const { return static_cast<float>(color_index); }
        
        // TODO: retry incrementing segNum through default arg
        Segment(int pNum, int indexp, sf::Color& colorp):
          segNum{pNum}, color_index{indexp}, color{&colorp}, 
          vertical_outline{sf::RectangleShape({2.f, GradientNS::bandHeight})}
        { 
            sf::Color outlineColor {sf::Color::Black};
            switch(segNum) {
              case Segment::Held: outlineColor=sf::Color{0xFFFFFFFF}; goto fallthrough;
              case Segment::Head:
              case Segment::Tail: /* outlineColor=sf::Color{0x000000}; */
              fallthrough: [[fallthrough]];
              default:
                  vertical_outline.setFillColor(sf::Color::Transparent);
                  vertical_outline.setOutlineColor(outlineColor);
                  vertical_outline.setOutlineThickness(1.f);
                  vertical_outline.setPosition(Xposition()-1.f, 0.f);
                  // don't set Y-position (always 0 relative to vlines_texture)
              break;
            }
        }
    };
    
    //std::array<>
    std::list<Segment*> segments;
    // this works without heap allocations, but the plan is to store them outside of the list
    // so that the ordering of indecies can be made independent from the list-order
    
    #define last GradientNS::pixelCount-1
    GradientEditor() : m_gradient{}, viewCurrent{m_gradient, true}, viewWorking{m_gradient, true}, viewOverlay{m_gradient, true},
      segments{ new Segment{Segment::Head,    0, m_gradient.gradientdata[0]   },
                new Segment{            1,  512, m_gradient.gradientdata[512] },
                new Segment{Segment::Tail, last, m_gradient.gradientdata[last]}}
    {
        viewOverlay.m_texture.clear(sf::Color::Transparent);
        viewWorking.m_sprite.move(0, GradientNS::bandHeight+1);
        viewOverlay.m_sprite.move(0, GradientNS::bandHeight+1);
        //DrawSegmentations();  //Do not call within constructor! (requires active ImGui context; not initialized yet)
    }
    #undef last
    
    //~GradientEditor() { delete segments; }  // TODO: stop leaking memory
};


class GradientWindow: sf::RenderWindow
{
    const Gradient_T MasterGradient{};
    GradientEditor Editor;
    
    bool isEnabled{false};   // window visibility
    int stored_xposition{0}; // matching mainwindow's x-position  // TODO: try storing a ref to the window position instead? (probably a bad idea)
    sf::Clock clock{}; // imgui-sfml 'Update()' requires deltatime
    
    
    bool Initialize(int xposition);
    void Create(); // calls sf::RenderWindow.create(...) with some arguments
    void AdjustPosition() {sf::RenderWindow::setPosition({stored_xposition, 144});}
    
    // Internal draw functions
    void DrawSegmentations(); // visual guides for the definition-points of the gradient
    void DisplayTestWindows();   // GradientTestWindow.cpp
    void CustomRenderingTest();  // GradientView.cpp
    
    
    public:
    friend int main(int argc, char** argv);
    void ToggleEnabled();
    void EventLoop(); // Window event-processing
    void FrameLoop(); // performs a single round of clear/draw/display and event-processing
    
    // constructors do not actually create the window; call '.create()' later
    //sf::RenderWindow(sf::VideoMode(1024, 512), "FLUIDSIM - Gradient", sf::Style::Default)
    GradientWindow(): sf::RenderWindow{}
    { ; }
};


#endif
