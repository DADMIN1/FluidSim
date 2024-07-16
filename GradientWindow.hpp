#ifndef FLUIDSIM_GRADIENTWINDOW_INCLUDED
#define FLUIDSIM_GRADIENTWINDOW_INCLUDED

#include "Gradient.hpp"

#include <array>
#include <list>  // GradientEditor::segments

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderWindow.hpp>


namespace GradientNS {
    constexpr int pixelCount{1024};
    constexpr int bandHeight  {64};
    constexpr int triangle_halfsz{8}; // parameter for segment-slider handles
    // vertical space reserved for the first two gradients, plus segment-sliders
    constexpr int headSpace{(bandHeight*2 + 1) + triangle_halfsz*3};
    constexpr int windowWidth{1536};
    //constexpr int windowHeight{360};
    constexpr int windowHeight{640};
    
    constexpr int segmentCap{8};
    constexpr int segmentLength{pixelCount/segmentCap};
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
    sf::RenderTexture hitboxLayer{};
    sf::RectangleShape hitbox{{GradientNS::pixelCount, GradientNS::headSpace}}; // for editor region
    void DrawHitboxes();
    
    struct Segment 
    {
        bool isSelected{false};
        enum Index: int { Head = -1, Tail = -2, Held = -3 } const index;
        static int nextindex; //only used for constructor's default arg
        int  color_index; //into gradient
        sf::Color* color;
        sf::RectangleShape vertical_outline;
        sf::RectangleShape hitbox;
        
        float Xposition() const { return static_cast<float>(color_index); }
        
        Segment(int pNum = nextindex++):index{pNum},
          color_index{ (nextindex*GradientNS::segmentLength) - (GradientNS::segmentLength/2) },
          color{nullptr}, vertical_outline{sf::RectangleShape({2.0f, GradientNS::bandHeight})}
        { 
            sf::Color outlineColor {sf::Color::Black};
            if (index < 0) { color_index=0; }
            switch(index) {
              case Segment::Held: outlineColor=sf::Color{0xFFFFFFFF};  goto fallthrough;
              case Segment::Tail: color_index = GradientNS::pixelCount-1; [[fallthrough]];
              case Segment::Head: outlineColor=sf::Color{0x00000000};
              fallthrough: [[fallthrough]];
              default:
                  vertical_outline.setFillColor(sf::Color::Transparent);
                  vertical_outline.setOutlineColor(outlineColor);
                  vertical_outline.setOutlineThickness(1.f);
                  vertical_outline.setPosition(Xposition()-1.f, 0.f); // -1 because rectangle's-position is aligned to left edge
                  // don't set Y-position (always 0 relative to viewOverlay's rendertexture)
                  // hitbox covering only the triangle
                  //hitbox = sf::RectangleShape{{GradientNS::triangle_halfsz*2, GradientNS::triangle_halfsz*2}};
                  //hitbox.setOrigin(GradientNS::triangle_halfsz, GradientNS::triangle_halfsz);
                  //hitbox.setPosition(Xposition(), (GradientNS::bandHeight*2) + GradientNS::triangle_halfsz);
                  hitbox = sf::RectangleShape{{GradientNS::triangle_halfsz*2, GradientNS::headSpace-(GradientNS::bandHeight+GradientNS::triangle_halfsz)}};
                  hitbox.setOrigin(GradientNS::triangle_halfsz, 0);
                  hitbox.setFillColor(sf::Color::Transparent);
                  hitbox.setOutlineColor(sf::Color::White);
                  hitbox.setOutlineThickness(2.f);
                  hitbox.setPosition(Xposition(), GradientNS::bandHeight+1);
              break;
            }
        }
    };
    
    std::array<Segment, GradientNS::segmentCap> segstore;
    std::list<Segment*> segments;
    Segment* seg_held;
    std::list<Segment*>::iterator seg_hovered;
    
    GradientEditor(): m_gradient{}, viewCurrent{m_gradient, true}, viewWorking{m_gradient, true}, viewOverlay{m_gradient, true},
      segstore{}, segments{ new Segment{Segment::Head}, new Segment{Segment::Tail} }, seg_held{ new Segment{Segment::Held} }, seg_hovered{segments.begin()}
    {
        viewOverlay.m_texture.clear(sf::Color::Transparent);
        viewWorking.m_sprite.move(0, GradientNS::bandHeight+1);
        viewOverlay.m_sprite.move(0, GradientNS::bandHeight+1);
        
        hitboxLayer.create(GradientNS::pixelCount, GradientNS::headSpace);
        hitboxLayer.clear(sf::Color::Transparent);
        
        hitbox.setFillColor(sf::Color::Transparent);
        hitbox.setOutlineThickness(-2.f);
        hitbox.setOutlineColor(sf::Color::Blue);
        
        seg_held->color = &m_gradient.gradientdata[0];
        for(Segment* seg : segments) { seg->color = &m_gradient.gradientdata[seg->color_index]; }
        for(Segment& segr: segstore) { segr.color = &m_gradient.gradientdata[segr.color_index]; }
        auto insertpoint = ++segments.cbegin();
        for(int S{GradientNS::segmentCap-1}; S >= 0; --S) { //looping backwards to match iterator order with (array) indecies
            insertpoint = segments.insert(insertpoint, &segstore[S]);
        }
    }
    
    ~GradientEditor() {
        Segment* segfront = segments.front();
        Segment* seg_back = segments.back();
        if(segfront) delete segfront;
        if(seg_back) delete seg_back;
        if(seg_held) delete seg_held;
    }
};


class GradientWindow: sf::RenderWindow
{
    const Gradient_T MasterGradient{};
    GradientEditor Editor{};
    
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
