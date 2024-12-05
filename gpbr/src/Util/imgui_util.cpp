#include "gpbr/Util/imgui_util.h"
#include <unordered_map>
#include <glm/glm.hpp>
namespace util
{

// currently using an anonymous namespace to hold each theme in a "private" method
// consider using PIMPL idiom? https://stackoverflow.com/questions/8972588/is-the-pimpl-idiom-really-used-in-practice
namespace
{
    // Custom theme (unfinished).
    void set_theme_gpbr(ImGuiStyle& style)
    {
        glm::vec3 base_col = glm::vec3(0.2980392f, 0.3450980f, 0.2666667f);
        glm::vec3 dark_col = glm::vec3(0.2470588f, 0.2784314f, 0.2196078f);
        glm::vec3 text_col = glm::vec3(0.5882353f, 0.5294118f, 0.19607843f);

        style.Alpha          = 1.0f;
        style.WindowRounding = 3.0f;
        style.ChildRounding  = 3.0f;
        style.GrabRounding   = 1.0f;
        style.GrabMinSize    = 20.0f;
        style.FrameRounding  = 3.0f;

        style.Colors[ImGuiCol_Text]             = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_WindowBg]         = ImVec4(base_col.x, base_col.y, base_col.z, 0.9f);
        style.Colors[ImGuiCol_ChildBg]          = ImVec4(base_col.x, base_col.y, base_col.z, 1.0f);
        style.Colors[ImGuiCol_Border]           = ImVec4(text_col.x, text_col.y, text_col.z, 0.65f);
        style.Colors[ImGuiCol_BorderShadow]     = ImVec4(text_col.x, text_col.y, text_col.z, 1.00f);
        style.Colors[ImGuiCol_TitleBg]          = ImVec4(dark_col.x, dark_col.y, dark_col.z, 0.50f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(dark_col.x, dark_col.y, dark_col.z, 0.30f);
        style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(dark_col.x, dark_col.y, dark_col.z, 1.00f);
        style.Colors[ImGuiCol_FrameBg]          = ImVec4(dark_col.x, dark_col.y, dark_col.z, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(dark_col.x, dark_col.y, dark_col.z, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(dark_col.x, dark_col.y, dark_col.z, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg]        = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PopupBg]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab]       = ImVec4(text_col.x, text_col.y, text_col.z, 0.80f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(text_col.x, text_col.y, text_col.z, 1.00f);
        style.Colors[ImGuiCol_Button]           = ImVec4(text_col.x, text_col.y * 0.65, text_col.z * 0.65, 0.46f);
        style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(text_col.x, text_col.y, text_col.z, 0.43f);
        style.Colors[ImGuiCol_ButtonActive]     = ImVec4(text_col.x, text_col.y, text_col.z, 1.00f);
        //  style.Colors[ImGuiCol_Header]           = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        //   style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
        //   style.Colors[ImGuiCol_HeaderActive]     = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
        // style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.04f, 0.10f, 0.09f, 0.51f);
    }

    // Lifted from https://gist.github.com/enemymouse/c8aa24e247a1d7b9fc33d45091cbb8f0
    // and tweaked to support ImGui 1.91.4
    void set_theme_enemymouse(ImGuiStyle& style)
    {
        style.Alpha          = 1.0;
        style.ChildRounding  = 3;
        style.WindowRounding = 3;
        style.GrabRounding   = 1;
        style.GrabMinSize    = 20;
        style.FrameRounding  = 3;

        style.Colors[ImGuiCol_Text]                 = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
        style.Colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_Border]               = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
        style.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg]              = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
        style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
        style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
        style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.14f, 0.18f, 0.21f, 0.73f);
        style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
        style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.00f, 1.00f, 1.00f, 0.27f);
        style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
        style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
        style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.16f, 0.24f, 0.22f, 0.60f);
        style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
        style.Colors[ImGuiCol_SliderGrab]           = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
        style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
        style.Colors[ImGuiCol_Button]               = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
        style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
        style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
        style.Colors[ImGuiCol_Header]               = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
        style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
        style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
        style.Colors[ImGuiCol_Separator]            = ImVec4(0.00f, 0.50f, 0.50f, 0.33f);
        style.Colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.00f, 0.50f, 0.50f, 0.47f);
        style.Colors[ImGuiCol_SeparatorActive]      = ImVec4(0.00f, 0.70f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
        style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
        style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
        style.Colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.04f, 0.10f, 0.09f, 0.51f);
    }
} // namespace

void set_imgui_theme(ImGuiStyle& style, std::string theme_name)
{
    static const std::unordered_map<std::string, void (*)(ImGuiStyle&)> themes{
        {      "gpbr",       set_theme_gpbr},
        {"enemymouse", set_theme_enemymouse}
    };

    if (auto it = themes.find(theme_name); it != themes.end())
    {
        it->second(style);
    }
}
} // namespace util