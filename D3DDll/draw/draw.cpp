#include "draw.h"
#include "./imgui/imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <d3d9.h>
#include <tchar.h>
#include <cmath>

#define MY_PI 3.1415926f
// Main code
void DrawCircle(float x, float y, float r, unsigned int color)
{
    const int num_segments = 32;

    ImVec2 p[num_segments];
    for (int i = 0; i < num_segments; i++)
    {
        const float a = ((float)i) / ((float)num_segments - 1) * 2.0f * MY_PI;
        p[i].x = x + cosf(a + MY_PI) * r;
        p[i].y = y + sinf(a + MY_PI) * r;
    }
    ImGui::GetBackgroundDrawList()->AddConvexPolyFilled(p, num_segments, color);
}
void Draw_Text(float x, float y, const char* str, unsigned int color)
{
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), color, str);
}


