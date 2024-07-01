#ifndef FLUIDSIM_GRADIENTWINDOW_INCLUDED
#define FLUIDSIM_GRADIENTWINDOW_INCLUDED

#include "Gradient.hpp"

#include <array>
#include <iostream>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderWindow.hpp>


namespace GradientNS {
    constexpr int pixelCount{1024};
    constexpr int bandHeight  {32};
    constexpr int headSpace{bandHeight*2 + 1};  // vertical space reserved for the first two gradients
    constexpr int windowHeight{360};
};


struct GradientView: sf::Drawable
{
    const Gradient_T* const m_gradient;
    sf::RenderTexture m_texture{};
    sf::Sprite m_sprite{};
    const bool ownsPtr; // prevents the destructor from deleting pointers it does not own
    
    // implementing the SFML 'draw' function for this class
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override 
    { target.draw(m_sprite, states); }
    
    void Redraw(const bool useOriginal=false) // parameter selects between 'LookupDefault' and 'Lookup'
    { using namespace GradientNS;
        m_texture.clear();
        for (std::size_t i{0}; i<pixelCount; ++i) 
        {
            sf::RectangleShape colorband(sf::Vector2f{1, bandHeight});
            if (useOriginal)
                 colorband.setFillColor(m_gradient->LookupDefault(i));
            else colorband.setFillColor(m_gradient->Lookup(i));
            colorband.setPosition(i,0);
            m_texture.draw(colorband);
        }
        m_texture.display(); // must call .display before constructing sprite
    }
    
    GradientView(const Gradient_T* const gradientPtr, const bool givenOwnership)
    : sf::Drawable{}, m_gradient{gradientPtr}, ownsPtr{givenOwnership}
    { using namespace GradientNS;
        m_texture.create(pixelCount, bandHeight);
        Redraw(true);
        const int next_height{ownsPtr? 0 : bandHeight+1}; // stacking the gradients vertically, with 1-pixel gap
        m_sprite = sf::Sprite(m_texture.getTexture());
        m_sprite.setPosition(0, next_height);
    }
    
    ~GradientView() { if(ownsPtr) { delete m_gradient; } }
};


class GradientWindow: sf::RenderWindow
{
    Gradient_T* current_gradient;
    std::array<GradientView, 2> gradientViews;
    
    bool isEnabled{false};   // window visibility
    int stored_xposition{0}; // matching mainwindow's x-position
    sf::Clock clock{}; // imgui-sfml 'Update()' requires deltatime
    
    friend int main(int argc, char** argv);
    bool Initialize(int xposition);
    void Create(); // calls sf::RenderWindow.create(...) with some arguments
    void AdjustPosition() {sf::RenderWindow::setPosition({stored_xposition, 144});}
    
    public:
    void ToggleEnabled();
    void EventLoop(); // Window event-processing
    void FrameLoop(); // performs a single round of clear/draw/display and event-processing
    
    // constructors do not actually create the window; call '.create()' later
    //sf::RenderWindow(sf::VideoMode(1024, 512), "FLUIDSIM - Gradient", sf::Style::Default)
    GradientWindow(): sf::RenderWindow{}, current_gradient{new Gradient_T()}, 
      gradientViews{GradientView(current_gradient, true), GradientView(current_gradient, false)}
    { ; }
    
    //~GradientWindow() { delete current_gradient; }  // gradientView with ownership will free this in it's destructor
};


#endif
