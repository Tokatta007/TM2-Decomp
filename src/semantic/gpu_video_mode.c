/* @8017E0C8. Mutable SDK word, not the constant zero inferred by IDA.
 * GsGetWorkBase is the existing index label; these users consume video mode. */
int D_8017E0C8 __attribute__((section(".tm2_8017E0C8"))) = 0;

/* @80145DE8. Full-word setter returns the previous value. */
int __attribute__((section(".text_80145DE8"))) sub_80145DE8(int mode)
{
    int previous=D_8017E0C8;
    D_8017E0C8=mode;
    return previous;
}

/* @80145E00. Callers decide whether to truncate the result. */
int __attribute__((section(".text_80145E00"))) GsGetWorkBase(void)
{
    return D_8017E0C8;
}
