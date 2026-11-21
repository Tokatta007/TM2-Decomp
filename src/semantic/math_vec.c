typedef char math_vec_widths[(sizeof(int) == 4 && sizeof(short) == 2) ? 1 : -1];

/* One output component: three halfword-matrix by word-vector products,
 * summed and scaled down by 4096 (signed division toward zero). The input is
 * re-read per row, so input/output aliasing behaves as in retail. */
#define MAT_ROW(m, a, b, c, input) (((input)[0] * (m)[a] + (input)[1] * (m)[b] + (input)[2] * (m)[c]) / 4096)

/* @80153FE4: row-major 3x3 signed-halfword matrix, three signed words in/out. */
void __attribute__((section(".text_80153FE4"))) mathMulMatVec(short *m, int *input, int *output)
{
    output[0] = MAT_ROW(m, 0, 1, 2, input);
    output[1] = MAT_ROW(m, 3, 4, 5, input);
    output[2] = MAT_ROW(m, 6, 7, 8, input);   /* the value stays in v0 as the result */
}

/* @80154118: the same arithmetic with transposed matrix access. */
void __attribute__((section(".text_80154118"))) mathMulTransVec(short *m, int *input, int *output)
{
    output[0] = MAT_ROW(m, 0, 3, 6, input);
    output[1] = MAT_ROW(m, 1, 4, 7, input);
    output[2] = MAT_ROW(m, 2, 5, 8, input);   /* the value stays in v0 as the result */
}
