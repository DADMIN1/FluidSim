#include "GradientWindow.hpp"

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
    void RenderArrowWithOutline(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 color) {
        ImGui::_RenderArrowPointingAt(draw_list, pos, half_sz, direction, color);
        ImGui::_RenderArrowPointingAt(draw_list, {pos.x, pos.y}, {half_sz.x, half_sz.y}, direction, ImGui::GetColorU32(IM_COL32(0XFF, 0xFF, 0XFF, 0xFF)), 1.f);
    }
    
    void RenderArrowWithOutline(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 color, float thickness) {
        ImGui::_RenderArrowPointingAt(draw_list, {pos.x, pos.y-4.f}, {half_sz.x+2.f, half_sz.y+6.f}, direction, ImGui::GetColorU32(IM_COL32(0X00, 0x00, 0X00, 0xFF)), 2.f);
        ImGui::_RenderArrowPointingAt(draw_list, pos, half_sz, direction, color, thickness);
    }
}


// from Demo Window > Examples > Custom Rendering
void GradientWindow::CustomRenderingTest()
{
    ImGui::Begin("GradientTest");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // [imgui/demo_window.cpp (line 8042)]
    // (note that those are currently exacerbating our sRGB/Linear issues)
    // Calling ImGui::GetColorU32() multiplies the given colors by the current Style Alpha, but you may pass the IM_COL32() directly as well..
    ImGui::Text("Gradients");
    ImVec2 gradient_size = ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeight());
    
    {
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
        ImU32 col_a = ImGui::GetColorU32(IM_COL32(0, 0, 0, 255));
        ImU32 col_b = ImGui::GetColorU32(IM_COL32(255, 255, 255, 255));
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
        ImU32 col_a = ImGui::GetColorU32(IM_COL32(0, 255, 0, 255));
        ImU32 col_b = ImGui::GetColorU32(IM_COL32(255, 0, 0, 255));
        draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
        ImGui::InvisibleButton("##gradient2", gradient_size);
        ImGui::_RenderArrowPointingAt(draw_list, p1, {5,10}, ImGuiDir_Up, col_b);
    }
    
    ImGui::End();
    return;
}


void GradientWindow::DrawSegmentations()
{
    ImDrawList* drawlist = ImGui::GetForegroundDrawList(); // this can be drawn outside of any (imgui) window!!!
    
    using Segment = GradientEditor::Segment;
    GradientView& viewOverlay = Editor.viewOverlay;
    viewOverlay.m_texture.clear(sf::Color::Transparent);
    
    for (Segment* segmentPtr: Editor.segments)
    {
        if(!segmentPtr) continue;
        Segment& segment {*segmentPtr};
        ImGui::RenderArrowWithOutline(
            drawlist, 
            {segment.Xposition(), GradientNS::bandHeight*2}, // intentionally off-by-one (high; slightly pushing arrow into the gradient)
            {8,16}, 
            ImGuiDir_Up, 
            ImGui::GetColorU32(ImVec4(*segment.color)) // required conversion, for some reason
        );
        
        // these get drawn UNDER the arrows, even though the arrows are drawn first
        // because imgui.render is called at the very end of the frameloop
        viewOverlay.m_texture.draw(segment.vertical_outline);
    }
    
    viewOverlay.m_texture.display();
    return;
}
