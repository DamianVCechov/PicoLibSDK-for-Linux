// Soubor: main.cpp
// Popis: 3D vizualizace funkce sinc(d) = sin(d)/d pro knihovnu PicoLibSDK.
// Ovládání pro PicoPad:
//   Šipky  - Otáčení scény
//   Tlačítko A - Přiblížení (Zoom In)
//   Tlačítko B - Oddálení (Zoom Out)
//   Tlačítko Y - Konec programu

#include "../include.h"

struct Point3D { float x, y, z; };
struct Point2D { int x, y; };

const float AXIS_INDICATOR_SCALE = 25.0f; // Měřítko pro ukazatel os
const int AXIS_INDICATOR_X = 40;          // X pozice ukazatele os
const int AXIS_INDICATOR_Y = 200;         // Y pozice ukazatele os

const int GRID_SIZE = 41;
Point2D screen_points[GRID_SIZE][GRID_SIZE];

Point3D rotate_point(Point3D p, float angle_yaw, float angle_pitch) {
    Point3D rotated_p;

    float cosY = cosf(angle_yaw);
    float sinY = sinf(angle_yaw);
    float tempX = p.x * cosY + p.z * sinY;
    float tempZ = -p.x * sinY + p.z * cosY;

    float cosX = cosf(angle_pitch);
    float sinX = sinf(angle_pitch);
    rotated_p.x = tempX;
    rotated_p.y = p.y * cosX - tempZ * sinX;
    rotated_p.z = p.y * sinX + tempZ * cosX;

    return rotated_p;
}

// --- Funkce pro projekci 3D bodu na 2D obrazovku ---
Point2D project_point(Point3D p, float scale) {
    int sx = (int)((WIDTH / 2.0f) + p.x * scale);
    int sy = (int)((HEIGHT / 2.0f) - p.y * scale);
    return {sx, sy};
}

// --- Funkce pro projekci os ukazatele ---
Point2D project_axis(Point3D p) {
    int sx = (int)(AXIS_INDICATOR_X + p.x * AXIS_INDICATOR_SCALE);
    int sy = (int)(AXIS_INDICATOR_Y - p.y * AXIS_INDICATOR_SCALE);
    return {sx, sy};
}

// --- Hlavní funkce pro vykreslení celé scény ---
void render_scene(float angle_yaw, float angle_pitch, float scale) {
    DrawClearCol(COLOR(20, 20, 30));

    const int grid_range = 20;
    const int grid_step = 5;

    for (int i = -grid_range; i <= grid_range; i += grid_step) {
        COLTYPE grid_color = (i == 0) ? COL_GRAY : COL_DKGRAY;

        Point3D start_z = {(float)i, 0, (float)-grid_range};
        Point3D end_z = {(float)i, 0, (float)grid_range};
        Point2D p_start_z = project_point(rotate_point(start_z, angle_yaw, angle_pitch), scale);
        Point2D p_end_z = project_point(rotate_point(end_z, angle_yaw, angle_pitch), scale);
        DrawLine(p_start_z.x, p_start_z.y, p_end_z.x, p_end_z.y, grid_color);

        Point3D start_x = {(float)-grid_range, 0, (float)i};
        Point3D end_x = {(float)grid_range, 0, (float)i};
        Point2D p_start_x = project_point(rotate_point(start_x, angle_yaw, angle_pitch), scale);
        Point2D p_end_x = project_point(rotate_point(end_x, angle_yaw, angle_pitch), scale);
        DrawLine(p_start_x.x, p_start_x.y, p_end_x.x, p_end_x.y, grid_color);
    }
    
    const float step = 1.0f; // Krok mřížky pro graf
    const float range = (GRID_SIZE - 1) / 2.0f * step;
    
    int i = 0;
    for (float grid_x = -range; grid_x <= range; grid_x += step) {
        int j = 0;
        for (float grid_y = -range; grid_y <= range; grid_y += step) {
            float d = sqrtf(grid_x * grid_x + grid_y * grid_y);
            float height = (d == 0.0f) ? 5.0f : sinf(d) / d * 5.0f; // Zvětšení 

            Point3D p3d = {grid_x, height, grid_y};
            Point3D rotated_p = rotate_point(p3d, angle_yaw, angle_pitch);
            screen_points[i][j] = project_point(rotated_p, scale);
            j++;
        }
        i++;
    }

    // --- Vykreslení drátového modelu grafu ---
    for (i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            // Určení barvy podle výšky (souřadnice Y na obrazovce)
            int r = 128 + (int)(screen_points[i][j].y / 2.0f);
            if (r < 0) r = 0; if (r > 255) r = 255;
            int g = 50;
            int b = 128 - (int)(screen_points[i][j].y / 2.0f);
            if (b < 0) b = 0; if (b > 255) b = 255;
            COLTYPE line_color = COLOR(r, g, b);
            
            // Spojnice se sousedními body
            if (j + 1 < GRID_SIZE) 
                DrawLine(screen_points[i][j].x, screen_points[i][j].y, screen_points[i][j+1].x, screen_points[i][j+1].y, line_color);
            if (i + 1 < GRID_SIZE) 
                DrawLine(screen_points[i][j].x, screen_points[i][j].y, screen_points[i+1][j].x, screen_points[i+1][j].y, line_color);
        }
    }

    Point3D x_axis = {1, 0, 0}, y_axis = {0, 1, 0}, z_axis = {0, 0, 1};

    Point3D rot_x_axis = rotate_point(x_axis, angle_yaw, angle_pitch);
    Point3D rot_y_axis = rotate_point(y_axis, angle_yaw, angle_pitch);
    Point3D rot_z_axis = rotate_point(z_axis, angle_yaw, angle_pitch);
    
    Point2D origin_2d = project_axis({0,0,0}); // Počátek je vždy v centru
    Point2D x_axis_2d = project_axis(rot_x_axis);
    Point2D y_axis_2d = project_axis(rot_y_axis);
    Point2D z_axis_2d = project_axis(rot_z_axis);

    DrawLine(origin_2d.x, origin_2d.y, x_axis_2d.x, x_axis_2d.y, COL_RED);
    DrawLine(origin_2d.x, origin_2d.y, y_axis_2d.x, y_axis_2d.y, COL_GREEN);
    DrawLine(origin_2d.x, origin_2d.y, z_axis_2d.x, z_axis_2d.y, COL_BLUE);
}

// --- Hlavní program ---
int main() {
    // Inicializace proměnných pro rotaci a zoom
    float angle_yaw = 0.6f;
    float angle_pitch = 0.5f;
    float scale = 7.0f;
    const float rotation_speed = 0.05f;
    const float zoom_speed = 0.5f;

    // Hlavní smyčka
    while (true) {
        // Vykreslení scény
        render_scene(angle_yaw, angle_pitch, scale);

        // Zobrazení nápovědy
        DrawText("Sipky=otaceni, A/B=zoom, Y=exit", 5, 5, COL_WHITE);

        DispUpdate();

        // Zpracování vstupu od uživatele
        switch (KeyGet()) {
            case KEY_UP:    angle_pitch -= rotation_speed; break;
            case KEY_DOWN:  angle_pitch += rotation_speed; break;
            case KEY_LEFT:  angle_yaw   += rotation_speed; break;
            case KEY_RIGHT: angle_yaw   -= rotation_speed; break;
            case KEY_A:     scale += zoom_speed; break;
            case KEY_B:     if (scale > zoom_speed) scale -= zoom_speed; break;
            case KEY_Y:     ResetToBootLoader(); break;
        }
    }
}
