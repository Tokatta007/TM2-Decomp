/* @8012D908: movie grid render (the class's virtual function). C++ (PsyQ 3.6
 * CC1PLPSX, -fvolatile); a member function has this ABI, extern "C" keeps the
 * retail name. Every field is read through `this` each time. When the grid is
 * hidden control falls off the end: retail returns whatever v0 holds (the zero
 * `visible` value); callers ignore it. */
#include "menu_slider.hpp"

extern "C" {
TM2GridLight *csAlloc(void);
int lightGetEnv(int);
int abs(int);
void csSetModel(void *, void *);
void *RotMatrixYXZ(void *, void *);
void viewProc(int);
void viewSetFixedCamera(int, int, int);
/* renderDrawModel is void (v0 at its return is left over from its last call), but retail
 * returns its result (a void call in tail position would become a jump) here, so this unit keeps the int-returning view it was compiled with. */
int renderDrawModel(int, void *);
}

extern "C" int __attribute__((section(".text_8012D908"))) menuSliderDraw(TM2MovieGrid *grid, int context)
{
    if (grid->visible) {
        if (grid->light == 0) {
            grid->light = csAlloc();
            grid->light->w4 = lightGetEnv(0);
            csSetModel(grid->light, grid->light_setup);
        }
        for (int i = 0; i < 16; i++) {
            if (i < grid->count) {
                grid->cell[i].value = grid->cell_value;
                grid->cell[i].lit = 1;
            } else {
                grid->cell[i].value = 0;
                grid->cell[i].lit = 0;
            }
        }
        grid->light->position[0] = grid->x;
        grid->light->position[1] = grid->y;
        grid->light->position[2] = grid->z;
        grid->light->rotation[0] = grid->rx;
        grid->light->rotation[1] = grid->ry;
        grid->light->rotation[2] = grid->rz;
        RotMatrixYXZ(grid->light->rotation, grid->light->matrix);
        if (grid->flat) {
            viewProc(0);
        } else {
            if (grid->tone != -1) grid->light->tone = grid->tone;
            if (grid->scale < 0) viewSetFixedCamera(0, 8 / abs(grid->scale), 1);
            else viewSetFixedCamera(0, grid->scale * 8, 1);
        }
        return renderDrawModel(context, grid->light);
    }
}
