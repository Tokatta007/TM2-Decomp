/* Movie grid object (C++, g++ 2.x layout): constructor @8012D844
 * (menu_slider_create.cpp), virtual render @8012D908 (menu_slider_draw.cpp). */
#ifndef TM2_MOVIE_GRID_HPP
#define TM2_MOVIE_GRID_HPP

struct TM2VtableEntry {   /* g++ 2.x: entry 0 empty, then {delta, index, function} */
    short delta, index;
    void (*function)();
};

struct TM2GridLight {
    int tone;
    int w4;
    char pad8[120];
    char matrix[32];          /* +128, from RotMatrixYXZ */
    short rotation[3];        /* +160 */
    char pad166[2];
    int position[3];          /* +168 */
};

struct TM2GridCell {          /* 24 bytes */
    unsigned char kind;
    char pad1[3];
    int x, y, z;
    unsigned short h16;
    unsigned char lit, b19;
    int value;
};

struct TM2MovieGrid {
    char base0[4];
    int cell_value;           /* +4: copied to the lit cells */
    unsigned short visible;   /* +8 */
    unsigned short flat;      /* +10 */
    short tone;               /* +12, -1 = keep */
    char pad14[2];
    int scale;                /* +16: >= 0 multiplies by 8, < 0 divides 8 */
    TM2GridLight *light;      /* +20, created on first render */
    int x, y, z;              /* +24 */
    int rz, rx, ry;           /* +36 */
    TM2VtableEntry *vptr;     /* +48 */
    unsigned char light_setup[20];  /* +52 */
    unsigned char columns, rows;    /* +72 */
    char pad74[2];
    TM2GridCell *cell_ptr[16];      /* +76 */
    char pad140[4];
    TM2GridCell cell[16];           /* +144 */
    unsigned char count;            /* +528: number of lit cells */
};

#endif
