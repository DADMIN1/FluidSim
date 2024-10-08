#include "GradientWindow.hpp"

#include <ranges> // GradientEditor::GetRangeContents

#include <imgui.h>
#include <imgui-SFML.h>


namespace ImGui {
    // names are underscored so they don't redefine the actual ImGui function
    // Render an arrow. 'pos' is position of the arrow tip. half_sz.x is length from base to tip. half_sz.y is length on each side.
    void _RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 color) {
        switch (direction) {
            case ImGuiDir_Left:  draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, color); return;
            case ImGuiDir_Right: draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, color); return;
            case ImGuiDir_Up:    draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, color); return;
            case ImGuiDir_Down:  draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, color); return;
            case ImGuiDir_None: case ImGuiDir_COUNT: break; // Fix warnings
        }
    }
    
    // Overload for non-filled triangles
    void _RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 color, float thickness) {
        switch (direction) {
            case ImGuiDir_Left:  draw_list->AddTriangle(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, color, thickness); return;
            case ImGuiDir_Right: draw_list->AddTriangle(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, color, thickness); return;
            case ImGuiDir_Up:    draw_list->AddTriangle(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, color, thickness); return;
            case ImGuiDir_Down:  draw_list->AddTriangle(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, color, thickness); return;
            case ImGuiDir_None: case ImGuiDir_COUNT: break;
        }
    }
    
    // draws an outline for each arrow (assuming they point up)
    void RenderArrowWithOutline(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 color, ImU32 outline_color=ImGui::GetColorU32(IM_COL32(0XFF, 0xFF, 0XFF, 0xFF))) {
        ImGui::_RenderArrowPointingAt(draw_list, pos, half_sz, direction, color);
        ImGui::_RenderArrowPointingAt(draw_list, {pos.x, pos.y}, {half_sz.x, half_sz.y}, direction, outline_color, 1.f); // outline (unfilled)
    }
    
    // overload for unfilled triangles
    void RenderArrowWithOutline(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 inner_color, ImU32 outer_color, float inner_thickness, float outer_thickness=2.f) {
        ImGui::_RenderArrowPointingAt(draw_list, {pos.x, pos.y-4.f}, {half_sz.x+2.f, half_sz.y+6.f}, direction, outer_color, outer_thickness);
        ImGui::_RenderArrowPointingAt(draw_list, pos, half_sz, direction, inner_color, inner_thickness);
    }
    
    // wrapper for easier conditional-logic; toggles filled/unfilled triangles; filled has an additional toggle for outline
    void RenderArrowConditional(bool fillCondition, bool outlineCodition, ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 color, ImU32 outline_color, float thickness) {
        if (fillCondition) {
            ImGui::_RenderArrowPointingAt(draw_list, pos, half_sz, direction, color);
            if(outlineCodition) { ImGui::_RenderArrowPointingAt(draw_list, {pos.x, pos.y}, {half_sz.x, half_sz.y}, direction, outline_color, thickness); } // outline (unfilled)
        } else {
            ImGui::_RenderArrowPointingAt(draw_list, pos, half_sz, direction, color, thickness);
            //if(outlineCodition) ImGui::_RenderArrowPointingAt(draw_list, {pos.x, pos.y-4.f}, {half_sz.x+2.f, half_sz.y+6.f}, direction, outline_color, thickness*2.f);
        }
    }
    
    //imgui_internal.h: line 479
    ImVec4 ImLerp(const ImVec4& a, const ImVec4& b, float t) { return ImVec4(a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t, a.w+(b.w-a.w)*t); }
}


// from Demo Window > Examples > Custom Rendering
void GradientWindow::CustomRenderingTest()
{
    static bool gradientTestOpen{true};
    if(!gradientTestOpen) return;
    ImGui::Begin("GradientTest", &gradientTestOpen);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // [imgui/demo_window.cpp (line 8042)]
    // (note that those are currently exacerbating our sRGB/Linear issues)
    // Calling ImGui::GetColorU32() multiplies the given colors by the current Style Alpha, but you may pass the IM_COL32() directly as well..
    ImGui::Text("Gradients");
    ImVec2 gradient_size = ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeight());
    
    {
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
        ImU32 col_a = ImGui::GetColorU32(IM_COL32(0x00, 0x00, 0x00, 0xFF));
        ImU32 col_b = ImGui::GetColorU32(IM_COL32(0xFF, 0xFF, 0xFF, 0xFF));
        draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
        ImGui::InvisibleButton("##gradient1", gradient_size);
        
        static float xposition {p1.x - (gradient_size.x/2.f)}; //doesn't follow window lol
        ImGui::_RenderArrowPointingAt(draw_list, {xposition, p1.y}, {5,10}, ImGuiDir_Up, 0xFF00FFFF);
        ImGui::Spacing(); ImGui::Spacing();
        ImGui::SliderFloat("##arrowslider", &xposition, p0.x, p0.x+gradient_size.x, "%.f");
    }
    
    ImGui::Spacing();
    
    {
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
        ImU32 col_a = ImGui::GetColorU32(IM_COL32(0x00, 0xFF, 0x00, 0xFF));
        ImU32 col_b = ImGui::GetColorU32(IM_COL32(0xFF, 0x00, 0x00, 0xFF));
        draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
        ImGui::InvisibleButton("##gradient2", gradient_size);
        ImGui::_RenderArrowPointingAt(draw_list, p1, {5,10}, ImGuiDir_Up, col_b);
    }
    
    ImGui::End();
    return;
}


// ______________________________ //
// ------- GradientEditor ------- //
// ______________________________ //


int GradientEditor::Segment::nextindex{0}; // static member

/* ---- stack operations ---- */

// stores a (removed) pointer into segmentStack, roughly ordered by index
void GradientEditor::StackPush(Segment* const segptr)
{
    assert((segptr != nullptr) && "GradientEditor: StackPush called on nullptr");
    if(segmentStack.empty()) { segmentStack.emplace_back(segptr); return; }
    const Segment& front{*segmentStack.front()};
    const Segment& back {*segmentStack.back()};
    const int index = segptr->index;
    
    if (index > back.index)       segmentStack.emplace_back(segptr);
    else if (index < front.index) segmentStack.emplace_front(segptr);
    else { // when neither placement works, swap and insert into the smaller end
        const bool frontSmaller {front.index < back.index};
        Segment* const oldptr  {(frontSmaller? segmentStack.front(): segmentStack.back())};
        if (frontSmaller) {
            segmentStack.pop_front();
            segmentStack.emplace_front(segptr);
            segmentStack.emplace_front(oldptr);
        } else {
            segmentStack.pop_back();
            segmentStack.emplace_back(segptr);
            segmentStack.emplace_back(oldptr);
        }
    }
    return;
}


// re-inserts a removed point from segmentStack; pulls from front/back depending on which side
GradientEditor::SegIterT GradientEditor::StackPull(const SegIterT atIter, bool rightSide)
{
    assert(!segmentStack.empty() && "GradientEditor: StackPull called on empty stack");
    SegIterT newIter = segments.insert(atIter, (rightSide? segmentStack.back(): segmentStack.front()));
    if(rightSide) { segmentStack.pop_back(); } else { segmentStack.pop_front(); }
    return newIter;
}


/* ---- segment operations ---- */

// changes selection to right/left segment
void GradientEditor::SwitchSegment(bool right)
{
    if(!seg_range.isValid || isDraggingSegment) return;
    SegIterT selectedIter {right? seg_range.nextIter : seg_range.prevIter};
    if((*selectedIter)->index < 0) return;
    
    (*seg_range.m_iter)->isSelected = false;
    (*selectedIter)->isSelected = true;
    seg_range = SegmentRange(selectedIter);
    return;
}


// subdivides one side of the current segment by inserting a new midpoint
void GradientEditor::SplitSegment(bool right)
{
    if(!seg_range.isValid || isDraggingSegment || segmentStack.empty()) return;
    if((*seg_range.m_iter)->index < 0) return;
    
    const SegIterT outsidePoint {right? seg_range.nextIter : seg_range.prevIter};
    const int colorIndexOriginal = (*seg_range.m_iter)->color_index;
    const int colorIndexEndpoint = (*outsidePoint)->color_index;
    const int colorIndexMidpoint = (colorIndexOriginal+colorIndexEndpoint)/2;
    if(std::abs(colorIndexEndpoint-colorIndexMidpoint) <= (GradientNS::triangle_halfsz*2)) return; // not enough space
    
    const Segment* const backupPtr = *seg_range.m_iter;
    assert((backupPtr != nullptr) && "SplitSegment: seg_range is holding nullptr");
    
    seg_range.isValid = false;
    seg_hovered = segments.end();
    SegIterT insertedPoint = StackPull((right? seg_range.nextIter : seg_range.m_iter), right);
    RelocatePoint(**insertedPoint, colorIndexMidpoint);
    
    assert((backupPtr == *seg_range.m_iter) && "SplitSegment: iterator was invalidated");
    
    if (preserveSelection) {
        seg_range = SegmentRange(seg_range.m_iter);
    } else {
        (*seg_range.m_iter)->isSelected = false;
        (*insertedPoint)->isSelected = true;
        seg_range = SegmentRange(insertedPoint);
    }
    
    /*
    Segment& oldMidpoint {**(right? seg_range.prevIter : seg_range.nextIter)}; // previous midpoint becomes endpoint
    Segment& oldEndpoint {**(right? seg_range.nextIter : seg_range.prevIter)};
    Segment& newMidpoint {**seg_range.m_iter};
    
    RelocatePoint(oldMidpoint, colorIndexOriginal);
    RelocatePoint(oldEndpoint, colorIndexEndpoint);
    RelocatePoint(newMidpoint, colorIndexMidpoint);
    */
    return;
}


// removes middle segment-point, then expands to new endpoint
void GradientEditor::JoinSegment(bool right)
{
    if(!seg_range.isValid || isDraggingSegment) return;
    if((*seg_range.m_iter)->index < 0) return;
    SegIterT outsidePoint {right? seg_range.nextIter : seg_range.prevIter};
    if((*outsidePoint)->index < 0) return;
    /*if((*outsidePoint)->index < 0) {
        outsidePoint = (right? seg_range.prevIter : seg_range.nextIter); // try to flip the other way
        if((*outsidePoint)->index < 0) return;  // fail if the current segment is the only one (that isn't an endpoint)
    }*/
    
    const SegIterT selection { preserveSelection? outsidePoint : seg_range.m_iter };
    const SegIterT successor { preserveSelection? seg_range.m_iter : outsidePoint };
    const Segment* const backupPtr = *successor;
    assert((backupPtr != nullptr) && "JoinSegment: seg_range is holding nullptr");
    assert((*selection != *successor) && "JoinSegment: erased segment is current");
    
    seg_range.isValid = false;
    seg_hovered = segments.end();
    if (!preserveSelection) {
        (*selection)->isSelected = false;
        (*successor)->isSelected = true;
    }
    StackPush(*selection);
    segments.erase(selection);
    
    assert((backupPtr == *successor) && "JoinSegment: iterator was invalidated");
    seg_range = SegmentRange(successor);
    
    return;
}


/* ---- range operations ---- */

//#define DBG_PRINT_SEGMENTS
#ifdef DBG_PRINT_SEGMENTS
#include <iostream>
#include <sstream> // formatting colors as hexadecimal triplets (RGB)

std::string ColorChannelToString(sf::Uint8 sf_colorcomponent)
{
    unsigned int C {sf_colorcomponent}; //conversion is required
    std::stringstream formatbuffer{""};
    formatbuffer << std::hex << std::uppercase << C;
    std::string prefix{(C < 0x10)? "0x0" : "0x"}; //single-digit (hex) numbers need padding
    //'std::showbase' is also be affected by 'std::uppercase', resulting in "0X"; add prefix manually instead
    // also 'showbase' won't add a prefix to zero, for some reason.
    return prefix + formatbuffer.str();
}
#endif

auto GradientEditor::GetRangeIndecies(const SegmentRange& seg_range) const
{
    const int colorindex_held {seg_range.colorindex_held};
    const int colorindex_prev {seg_range.colorindex_prev};
    const int colorindex_next {seg_range.colorindex_next};
    
    // inclusive
    auto indecies = std::pair {
        std::views::iota(colorindex_prev, colorindex_held+1),
        std::views::iota(colorindex_held, colorindex_next+1),
    };
    
    // non-inclusive
    /* auto indecies = std::pair {
        std::views::iota(colorindex_prev+1, colorindex_held),
        std::views::iota(colorindex_held+1, colorindex_next),
    }; */
    
    #ifdef DBG_PRINT_SEGMENTS
    std::cout << "Indecies " << std::dec        << "\n";
    std::cout << "left   : " << colorindex_prev << "\n";
    std::cout << "right  : " << colorindex_next << "\n";
    std::cout << "center : " << colorindex_held << "\n";
    std::cout << '\n' << '\n';
    
    auto [left, right] = indecies;
    for (auto range: {left, right}) {
        for (int i: range) { std::cout << i << " "; }
        std::cout << '\n' << '\n';
    }
    std::cout << '\n' << '\n';
    #endif
    
    return indecies;
}


// isolates the colors inside the segments left/right 
auto GradientEditor::GetRangeContents(const SegmentRange& seg_range, const Gradient_T& m_gradient) const
{
    const int colorindex_held {seg_range.colorindex_held};
    const int colorindex_prev {seg_range.colorindex_prev};
    const int colorindex_next {seg_range.colorindex_next};
    
    auto filter_all   = [=](auto&& pair)->bool { const auto&[index,element]=pair; return {(index >= colorindex_prev) && (index <= colorindex_next)}; };
    auto filter_left  = [=](auto&& pair)->bool { const auto&[index,element]=pair; return {(index >= colorindex_prev) && (index <= colorindex_held)}; };
    auto filter_right = [=](auto&& pair)->bool { const auto&[index,element]=pair; return {(index >= colorindex_held) && (index <= colorindex_next)}; };
    
    auto range_all    = std::ranges::filter_view(std::views::enumerate(m_gradient.gradientdata), filter_all  );
    auto range_left   = std::ranges::filter_view(std::views::enumerate(m_gradient.gradientdata), filter_left );
    auto range_right  = std::ranges::filter_view(std::views::enumerate(m_gradient.gradientdata), filter_right);
    
    #ifdef DBG_PRINT_SEGMENTS
    std::cout << "Indecies " << std::dec        << "\n";
    std::cout << "left   : " << colorindex_prev << "\n";
    std::cout << "right  : " << colorindex_next << "\n";
    std::cout << "center : " << colorindex_held << "\n";
    
    std::cout << "\nRanges \n";
    auto PrintRange = [&](auto& range) -> void {
        for (int count{1}; const auto& [index, color]: range) { 
            //setting up left-padding for printing index
            auto oldfill = std::cout.fill('0');
            auto oldwidth = std::cout.width((colorindex_next>999)? 4:3);
            std::cout << std::dec << std::right << index << ":";
            //std::cout << '[' << index << ", " << std::hex << color.toInteger() << ']';
            std::cout.fill(oldfill); //unsetting
            std::cout.width(oldwidth);
            
            // printing the colors' components individually
            std::string R{ColorChannelToString(color.r)}, G{ColorChannelToString(color.g)}, B{ColorChannelToString(color.b)};
            std::cout << "[" << R << " " << G << " " << B << "]";
            std::cout << (((count++%5)==0)? "\n": " ");
        }
        std::cout << '\n' << '\n' << std::dec;
    };
    
    std::cout << "All   \n"; PrintRange(range_all);
    std::cout << "Left  \n"; PrintRange(range_left);
    std::cout << "Right \n"; PrintRange(range_right);
    #endif
    
    return std::tuple{range_left, range_right, range_all};
}


// allows directly modifying gradient through returned ranges
auto GradientEditor::GetRangeContentsMutable(const SegmentRange& seg_range, Gradient_T& m_gradient) const
{
    const int colorindex_held {seg_range.colorindex_held};
    const int colorindex_prev {seg_range.colorindex_prev};
    const int colorindex_next {seg_range.colorindex_next};
    auto filter_all   = [=](auto&& pair)->bool { const auto&[index,element]=pair; return {(index >= colorindex_prev) && (index <= colorindex_next)}; };
    auto filter_left  = [=](auto&& pair)->bool { const auto&[index,element]=pair; return {(index >= colorindex_prev) && (index <= colorindex_held)}; };
    auto filter_right = [=](auto&& pair)->bool { const auto&[index,element]=pair; return {(index >= colorindex_held) && (index <= colorindex_next)}; };
    auto range_all    = std::ranges::filter_view(std::views::enumerate(m_gradient.gradientdata), filter_all  );
    auto range_left   = std::ranges::filter_view(std::views::enumerate(m_gradient.gradientdata), filter_left );
    auto range_right  = std::ranges::filter_view(std::views::enumerate(m_gradient.gradientdata), filter_right);
    return std::tuple{range_left, range_right, range_all};
}


// takes and returns a range (std::views::enumerate) of {index, color}
auto InterpolateSegment(auto enumerated_segment) // not reference
{
    auto segment = std::views::values(enumerated_segment);
    auto indecies = std::views::keys(enumerated_segment);
    const auto length = std::distance(segment.begin(), segment.end()) - 1;
    const ImVec4 color_begin = ImVec4(*segment.begin());
    const ImVec4 color_final = ImVec4(*--segment.end());
    
    for (auto[offset, color]: std::views::enumerate(segment)) {
        color = sf::Color(ImGui::ImLerp(color_begin, color_final, float(offset)/float(length)));
    }
    
    return std::views::zip(indecies, segment);
}

void GradientEditor::InterpolateCurrentSegment()
{
    seg_range.wasModified = true;
    #ifdef DBG_PRINT_SEGMENTS
    GetRangeIndecies(seg_range);
    GetRangeContents(seg_range, m_gradient);
    #else
    auto[left, right, all] = GetRangeContentsMutable(seg_range, m_gradient);
    
    // extending color to gradient endpoints for edge segments
    Segment& current = **seg_range.m_iter;
    std::list<Segment*>::iterator iter = seg_range.m_iter;
    Segment& leftSeg = **--iter;  iter = seg_range.m_iter;
    Segment& rghtSeg = **++iter;
    if(leftSeg.index == Segment::Index::Head) { *leftSeg.color = *current.color;}
    if(rghtSeg.index == Segment::Index::Tail) { *rghtSeg.color = *current.color;}
    // TODO: should set defaults for endpoints instead during 'reset' actions
    
    //InterpolateSegment(all);
    InterpolateSegment(left);
    InterpolateSegment(right);
    viewWorking.RedrawTexture();
    #endif
    return;
}

#undef DBG_PRINT_SEGMENTS


void GradientEditor::RelocatePoint(Segment& segment, int target_colorindex)
{
    int dist = target_colorindex - segment.color_index;
    segment.color_index = target_colorindex;
    segment.color = &m_gradient.gradientdata[target_colorindex];
    segment.vertical_outline.move(dist, 0.f);
    segment.hitbox.move(dist, 0.f);
    return;
}


void GradientEditor::GrabSegment()
{
    seg_range.isValid = false;
    assert((seg_hovered != segments.end()) && "GradientEditor: GrabSegment called with bad seg_hovered");
    isDraggingSegment = true;
    const Segment& hovered_seg{**seg_hovered};
    if(hovered_seg.index < 0) { isDraggingSegment = false; return; }
    
    seg_range = SegmentRange(seg_hovered);
    if(!seg_range.isValid) { isDraggingSegment = false; return; }
    
    RelocatePoint(*seg_held, hovered_seg.color_index);
    /* auto held_iter = */ segments.insert(seg_hovered, seg_held);
    return;
}


void GradientEditor::ReleaseHeld()
{
    if(!isDraggingSegment) return;
    isDraggingSegment = false;
    assert((seg_hovered != segments.end()) && "GradientEditor: ReleaseHeld called with bad seg_hovered");
    RelocatePoint(**seg_hovered, seg_held->color_index); // updating color
    segments.remove(seg_held);
    
    if(seg_range.isValid) {
        seg_range.colorindex_held = seg_held->color_index;
    }
    
    return;
}


void GradientEditor::HandleMousemove(sf::Vector2f mousePosition)
{
    #ifdef DBG_GRADIENTWINDOW_DRAW_HITBOXES
    if(!inbounds.getLocalBounds().contains(mousePosition))
    { inbounds.setOutlineColor(sf::Color::Blue); return; }
    inbounds.setOutlineColor(sf::Color::Magenta);
    #else
    if((mousePosition.x > GradientNS::pixelCount) || (mousePosition.x < 0) ||
       (mousePosition.y > GradientNS::headSpace ) || (mousePosition.y < 0) ){
        return;
    }
    #endif
    
    // simply update the point if we're dragging something
    if(isDraggingSegment && seg_range.isValid) {
        int target = mousePosition.x; //restricting placement to current segment
        auto [lowerBound, upperBound] = seg_range.GetBounds();
        if (target < lowerBound) target = lowerBound;
        if (target > upperBound) target = upperBound;
        RelocatePoint(*seg_held, target); return;
    }
    
    // search for hovered_seg otherwise
    for(std::list<Segment*>::iterator segiter{segments.begin()}; segiter != segments.end(); ++segiter)
    {
        Segment* segptr{*segiter};
        if(segptr->index < 0) continue;
        segptr->hitbox.setOutlineColor(sf::Color::White);
        if(segptr->CheckHitbox(mousePosition)) {
            segptr->hitbox.setOutlineColor(sf::Color::Magenta);
            seg_hovered = segiter;
            return;
        }
    }
    
    seg_hovered = segments.end(); // invalidated
    
    return;
}


#ifdef DBG_GRADIENTWINDOW_DRAW_HITBOXES
void GradientEditor::DrawHitboxes()
{
    hitboxLayer.clear(sf::Color::Transparent);
    hitboxLayer.draw(inbounds);
    
    for (const Segment* seg: segments) {
        if(seg->index < 0) continue; // special indecies for Head/Tail/Held
        hitboxLayer.draw(seg->hitbox);
    }
    
    hitboxLayer.display();
    return;
}
#endif


void GradientWindow::DrawSegmentations()
{
    ImDrawList* drawlist = ImGui::GetForegroundDrawList(); // this can be drawn outside of any (imgui) window!!!
    
    using Segment = GradientEditor::Segment;
    GradientView& viewOverlay = Editor.viewOverlay;
    viewOverlay.m_texture.clear(sf::Color::Transparent);
    
    for (Segment* segment: Editor.segments)
    {
        bool fillCondition {segment->isSelected};
        bool outlineCondition {fillCondition};
        
        // color-variables for the triangle
        ImVec4 fillcolor = ImVec4(*segment->color);
        ImVec4 outlinecolor = ImVec4(segment->isSelected? sf::Color{0xFFFFFFCC} : sf::Color{0x00000033});
        // outlinecolor also applies to vertical-outline for non-special segments
        
        switch(segment->index)
        {
            case Segment::Held: if(!Editor.isDraggingSegment) continue; [[fallthrough]];
            case Segment::Head:
            case Segment::Tail:
                fillCondition = true;
                outlineCondition = true;
                
                // head/tail segments store the original color in outline, and is never changed (held is white)
                // the end-segments display the endpoint-color in their fill, and current-color in outlines instead
                if (segment->index != Segment::Held) {
                    outlinecolor = ImVec4(*segment->color);
                    fillcolor = ImVec4(segment->vertical_outline.getOutlineColor());
                } else {
                    // TODO: set held-segment's outline-color in the opposite way (fill=current, outlines=original-fill)
                    outlinecolor = ImVec4(segment->vertical_outline.getOutlineColor());
                }
                
                viewOverlay.m_texture.draw(segment->vertical_outline);
            break;
            
            default:
                segment->vertical_outline.setOutlineColor(outlinecolor);
                viewOverlay.m_texture.draw(segment->vertical_outline);
            break;
        }
        
        ImGui::RenderArrowConditional(fillCondition, outlineCondition,
            drawlist, 
            {segment->Xposition(), GradientNS::bandHeight*2}, // intentionally off-by-one (high; slightly pushing arrow into the gradient)
            {GradientNS::triangle_halfsz, (GradientNS::triangle_halfsz*2)}, 
            ImGuiDir_Up, 
            ImGui::GetColorU32(fillcolor),
            ImGui::GetColorU32(outlinecolor), // outline of the triangle, not the vertical box
            1.f
        );
        
    }
    
    viewOverlay.m_texture.display();
    return;
}
