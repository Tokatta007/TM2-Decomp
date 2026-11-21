/* 8015DAF8: an empty function that retail compiled without optimisation
 * (frame pointer, two jumps to the epilogue). It sits among the libspu modules
 * but matches no PsyQ 3.3/3.5 module (config/US/sdk_modules.tsv); probably a
 * 3.4 library static. Recipe: psyq3.3 cc1 with -O0 (manifest "opt"). The two
 * `return;` statements give retail's two jumps to the epilogue. */

void __attribute__((section(".text_8015DAF8"))) sub_8015DAF8(void)
{
    return;
    return;
}
