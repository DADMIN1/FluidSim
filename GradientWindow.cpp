#include "GradientWindow.hpp"

#include <SFML/Window.hpp>  // defines sf::Event
#include <imgui.h>
#include <imgui-SFML.h>



/* ------------------------------------- GradientWindow ------------------------------------- */


// Main.cpp
extern bool usingVsync;
extern int framerateCap;


bool GradientWindow::Initialize(int xposition)
{
    if(isOpen()) { return false; } //should not call initialize twice
    stored_xposition = xposition;
    AdjustPosition();
    
    if(!ImGui::SFML::Init(*this)) return false;
    ImGuiIO& imguiIO = ImGui::GetIO();
    
    // imguiIO.ConfigInputTrickleEventQueue      = false; // spreads interactions (like simultaneous keypresses) across multiple frames
    imguiIO.ConfigInputTextEnterKeepActive    = true ; // keep input focused after hitting enter
    imguiIO.ConfigWindowsResizeFromEdges      = false; // can be annoying, and it requires BackendFlags_HasMouseCursors anyway
    imguiIO.ConfigWindowsMoveFromTitleBarOnly = true ;
    
    imguiIO.ConfigFlags = ImGuiConfigFlags {
        ImGuiConfigFlags_None
        //| ImGuiConfigFlags_NavEnableKeyboard     // Master keyboard navigation enable flag. Enable full Tabbing + directional arrows + space/enter to activate.
        | ImGuiConfigFlags_NavNoCaptureKeyboard  // Instruct navigation to not set the io.WantCaptureKeyboard flag when io.NavActive is set.
        | ImGuiConfigFlags_NoMouseCursorChange   // Prevent imgui from altering mouse visibility/shape
        //| ImGuiConfigFlags_IsSRGB // Application is SRGB-aware. NOT used by core Dear ImGui (only used by backends, maybe)
    };
    
    //imguiIO.BackendFlags = ImGuiBackendFlags_HasMouseCursors;  // required for 'ResizeFromEdges'
    imguiIO.IniFilename = NULL; // disable autosaving of the 'imgui.ini' config file (just stores window states)
    
    return true;
}


void GradientWindow::Create()
{
    if(isOpen()) { return; }
    constexpr auto m_style = sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close;
    sf::RenderWindow::create(sf::VideoMode(GradientNS::windowWidth, GradientNS::windowHeight), "GradientWindow [FLUIDSIM]", m_style);
    setVerticalSyncEnabled(usingVsync);
    setFramerateLimit(framerateCap);
    AdjustPosition();
    setVisible(isEnabled);
    clock.restart();
    return;
}

// only used by the keybind in main, and in the eventloop below
void GradientWindow::ToggleEnabled()
{
    if (!sf::RenderWindow::isOpen()) {
        GradientWindow::Create();
        isEnabled = false; // gets toggled back to true
    }
    isEnabled = !isEnabled;
    sf::RenderWindow::setVisible(isEnabled);
    AdjustPosition();
    return;
}


void GradientWindow::EventLoop()
{
    sf::Event gw_event;
    while (sf::RenderWindow::pollEvent(gw_event)) 
    {
        ImGui::SFML::ProcessEvent(*this, gw_event);
        switch (gw_event.type) 
        {
            case sf::Event::Closed:
                isEnabled = false;
                sf::RenderWindow::close();
            break;
            
            case sf::Event::KeyPressed:
            {
                switch (gw_event.key.code) {
                    case sf::Keyboard::Key::Escape:
                    case sf::Keyboard::Key::Q:
                    case sf::Keyboard::F2:
                        isEnabled = false;
                        sf::RenderWindow::close();
                    break;
                    
                    /* case sf::Keyboard::F2:
                        ToggleEnabled();
                    break; */
                    
                    default: break;
                }
            }
            break;
            
            case sf::Event::MouseMoved:
            {
                const sf::RenderWindow& rw_ref {*this};
                const sf::Vector2f mousePosition {static_cast<float>(sf::Mouse::getPosition(rw_ref).x), static_cast<float>(sf::Mouse::getPosition(rw_ref).y)};
                Editor.HandleMousemove(mousePosition);
            }
            break;
            
            case sf::Event::MouseButtonPressed:
            {
                const sf::RenderWindow& rw_ref {*this};
                const sf::Vector2f mousePosition {static_cast<float>(sf::Mouse::getPosition(rw_ref).x), static_cast<float>(sf::Mouse::getPosition(rw_ref).y)};
                
                // early-exit if not inbounds (of GradientEditor)
                #ifdef DBG_GRADIENTWINDOW_DRAW_HITBOXES
                if(!Editor.inbounds.getLocalBounds().contains(mousePosition)) break;
                #else
                if((mousePosition.x > GradientNS::pixelCount) || (mousePosition.x < 0) ||
                   (mousePosition.y > GradientNS::headSpace ) || (mousePosition.y < 0) ){
                    break;
                }
                #endif
                
                if(gw_event.mouseButton.button == sf::Mouse::Button::Left)
                {
                    GradientEditor::Segment* hovered_seg{*Editor.seg_hovered};
                    if(hovered_seg->CheckHitbox(mousePosition))
                    {
                        static GradientEditor::Segment* last_selected {nullptr};
                        if(last_selected) { last_selected->isSelected = false; }
                        last_selected = hovered_seg;
                        
                        hovered_seg->isSelected = true;
                        hovered_seg->hitbox.setFillColor(sf::Color{0xFF, 0xFF, 0xFF, 0xCC});
                        Editor.isDraggingSegment = true;
                        Editor.GrabSegment();
                        break;
                    }
                    hovered_seg->isSelected = false;
                    hovered_seg->hitbox.setFillColor(sf::Color::Transparent);
                }
                else if(gw_event.mouseButton.button == sf::Mouse::Button::Right)
                { // deselect all points/segments
                    Editor.ReleaseHeld();
                    Editor.isDraggingSegment = false;
                    Editor.seg_range.isValid = false;
                    for (auto& segment: Editor.segments) {
                        segment->isSelected = false;
                        segment->hitbox.setFillColor(sf::Color::Transparent);
                    }
                }
            }
            break;
            
            case sf::Event::MouseButtonReleased:
                if(gw_event.mouseButton.button == sf::Mouse::Button::Left)
                {
                    if(Editor.seg_range.isValid && Editor.isDraggingSegment) {
                        Editor.ReleaseHeld();
                        heldSegmentWasChanged = true;
                    }
                }
            break;
            
            // prevents contents from scaling/moving on resize
            case sf::Event::Resized:
            {
                auto [newwidth, newheight] = gw_event.size;
                sf::View newview {sf::RenderWindow::getView()};
                newview.setViewport({0, 0, float{float(GradientNS::windowWidth)/newwidth}, float{float(GradientNS::windowHeight)/newheight}});
                sf::RenderWindow::setView(newview);
            }
            break;
            
            default: break;
        }
    }
    
    return;
}


void GradientWindow::FrameLoop()
{
    if(!isEnabled || !isOpen()) { return; }
    
    sf::RenderWindow::setActive(); // required; otherwise closing the window completely blacks-out the mainwindow
    sf::RenderWindow::clear(sf::Color::Transparent);
    ImGui::SFML::Update(*this, clock.restart()); // calls ImGui::NewFrame()
    EventLoop();
    
    DrawSegmentations(); // GradientView.cpp
    sf::RenderWindow::draw(Editor.viewCurrent);
    sf::RenderWindow::draw(Editor.viewWorking);
    sf::RenderWindow::draw(Editor.viewOverlay);
    #ifdef DBG_GRADIENTWINDOW_DRAW_HITBOXES
    Editor.DrawHitboxes();
    static const sf::Sprite hitboxSprite{Editor.hitboxLayer.getTexture()};
    draw(hitboxSprite);
    #endif
    
    DisplayTestWindows();   // GradientTestWindow.cpp
    //CustomRenderingTest();  // GradientView.cpp
    //DisplaySelectionInfo(); // GradientView.cpp
    
    if(Editor.seg_range.isValid && Editor.seg_range.wasModified) {
        MasterGradient.gradientdata = Editor.m_gradient.gradientdata;
        Editor.seg_range.wasModified = false;
    }
    
    ImGui::SFML::Render(*this);
    sf::RenderWindow::display();
    return;
}
