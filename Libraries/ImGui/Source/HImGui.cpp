#include "HImGui.h"

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"

#include "backends/imgui_impl_win32.cpp"
#include "backends/imgui_impl_dx12.cpp"


bool ImGui::SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderDouble2(const char* label, double v[2], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarN(label, ImGuiDataType_Double, v, 2, &v_min, &v_max, format, flags);
}

bool ImGui::SliderDouble3(const char* label, double v[3], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarN(label, ImGuiDataType_Double, v, 3, &v_min, &v_max, format, flags);
}

bool ImGui::SliderDouble4(const char* label, double v[4], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalarN(label, ImGuiDataType_Double, v, 4, &v_min, &v_max, format, flags);
}