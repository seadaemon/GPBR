/* imgui_util.h
 *
 * Provides methods to modify the primary ImGui context.
 *
 */
#include "imgui.h"
#include <string>

namespace util
{
// Changes the look and feel of ImGui.
void set_imgui_theme(ImGuiStyle& style, std::string theme_name);
} // namespace util