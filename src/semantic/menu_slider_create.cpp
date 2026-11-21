/* @8012D844: constructor of the movie grid object (16 cells of 24 bytes).
 * C++ (PsyQ 3.6 CC1PLPSX, -fvolatile). The base constructor runs first, then
 * the vtable pointer at +48 is set to this class's table. The table stays an
 * explicit retail-placed object; extern "C" keeps the retail symbol names. */
#include "menu_slider.hpp"

extern "C" int menuSliderDraw(void *grid, int context);
extern "C" TM2VtableEntry menuSliderVtbl[2] __attribute__((section(".tm2_800D02CC"))) = {
    {0, 0, 0}, {0, 0, (void (*)(void))menuSliderDraw}};

extern "C" void *sub_80123E98(TM2MovieGrid *, unsigned short first, unsigned short second);

extern "C" TM2MovieGrid * __attribute__((section(".text_8012D844")))
menuSliderCreate(TM2MovieGrid *grid, unsigned short first, unsigned short second)
{
    sub_80123E98(grid, first, second);
    grid->vptr = menuSliderVtbl;
    grid->cell_value = 0;
    grid->light = 0;
    grid->light_setup[0] = 1;
    grid->columns = 16;
    grid->rows = 4;
    int x = 0;
    for (int i = 0; i < 16; i++) {
        grid->cell_ptr[i] = &grid->cell[i];
        grid->cell[i].kind = 4;
        grid->cell[i].h16 = 0;
        grid->cell[i].lit = 0;
        grid->cell[i].b19 = 0;
        grid->cell[i].value = 0;
        grid->cell[i].x = x;
        x += 64;
        grid->cell[i].y = 0;
        grid->cell[i].z = 0;
    }
    grid->count = 8;
    return grid;
}
