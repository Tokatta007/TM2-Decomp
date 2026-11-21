/* gte_extra: Sony libgte routines that IDA did not list (config/US/extra_funcs.tsv,
 * note gte_asm).
 *
 * These are Sony libgte assembly routines reproduced from the retail
 * instruction sequence (bin/SCUS_94306R.EXE); no third-party source is used.
 * Nearly all are dead code in TM2. Each is file-scope assembly in its own
 * .text_<VA> section exporting sub_<VA> (same scheme as lib_r138.c and
 * model_emitter.c): ".set	noreorder" (TAB) stops maspsx from inserting nops,
 * ".set noreorder" (space, seen only by the assembler) keeps GNU as from
 * filling delay slots. GTE command words are emitted as .word with the cop2
 * operand in the comment; GTE register moves (mtc2/mfc2/ctc2/cfc2/lwc2/swc2)
 * are mnemonics. Calls are symbolic (VectorNormal core sub_8013FE60, printf
 * sub_801388A8). Data is referenced by retail address (%hi/%lo of D_XXXXXXXX:
 * sqrt tables D_801777B0 / D_80177944, matrix-stack depth D_8017E8C8, stack
 * D_8017E8CC, saved ra D_8017E8BC, error strings D_8017EB4C / D_8017EB7D);
 * stores keep a numeric low half because maspsx expands %lo(sym)(reg) stores.
 * VectorNormalS (8013FDF0) ends with a branch into the body of VectorNormalSS,
 * which lives in another section, so that one branch is a .word.
 * .ent/.end bracket each block because tools/match_unit.py treats output
 * without any .end as incomplete (this unit has no C function).
 *
 * Recipe: psyq3.3 cc1 + maspsx, gp_size 0.
 *
 * 8013FCC8 SquareRoot12, 8013FD64 InvSquareRoot, 8013FDF0 VectorNormalS, 8013FE30 VectorNormalSS, 8013FF1C MatrixNormal, 80140004 MulMatrix, 80140110 MulMatrix2, 8014021C ApplyMatrix, 8014026C ApplyMatrixSV, 801402C8 TransMatrix, 801402EC ScaleMatrix, 8014056C LocalLight, 80140590 DpqColor, 801405AC NormalColor, 801405C8 NormalColor3, 80140604 NormalColorDpq, 80140628 NormalColorDpq3, 80140690 NormalColorCol3, 801406D4 ColorDpq, 801406FC ColorCol, 80140720 NormalClip, 80140744 sub_80140744, 80140754 AverageSZ3, 80140764 AverageSZ4, 80140774 LightColor, 8014079C DpqColorLight, 801407C4 DpqColor3, 80140800 Intpl, 80140824 Square12, 8014084C Square0, 80140874 AverageZ3, 80140894 AverageZ4, 80140910 OuterProduct0, 80140968 Lzc, 80149704 CompMatrix, 80149864 MulMatrix0, 80149A54 MulRotMatrix, 80149B3C SetMulMatrix, 80149C4C ApplyMatrixLV, 80149DAC ApplyRotMatrix, 80149DDC ScaleMatrixL, 80149F00 PushMatrix, 80149FA0 PopMatrix, 8014A040 ReadRotMatrix, 8014A088 ReadLightMatrix, 8014A0D0 ReadColorMatrix, 8014A124 SetVertex0, 8014A134 SetVertex1, 8014A144 SetVertex2, 8014A154 SetVertexTri, 8014A174 SetRGBfifo, 8014A188 SetIR123, 8014A19C SetIR0, 8014A1C8 SetFarColor, 8014A1E8 SetSZfifo3, 8014A1FC SetSZfifo4, 8014A214 SetSXSYfifo, 8014A228 SetRii, 8014A23C SetMAC123, 8014A250 SetData32, 8014A280 SetDQA, 8014A28C SetDQB, 8014D62C ReadSXSYfifo, 8014D640 ReadRGBfifo, 8014D654 ReadGeomOffset, 8014D674 ReadGeomScreen */

/* SquareRoot12: square root of a 20.12 fixed-point value via the GTE leading-zero counter and the sqrt table. Sony assembly. */
__asm__(
    ".section .text_8013FCC8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8013FCC8\n"
    ".type sub_8013FCC8,@function\n"
    ".ent sub_8013FCC8\n"
    "sub_8013FCC8:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$30\n"   /* 8013FCC8: 4884f000 */
    "\tnop\n"   /* 8013FCCC: 00000000 */
    "\tnop\n"   /* 8013FCD0: 00000000 */
    "\tmfc2 $2,$31\n"   /* 8013FCD4: 4802f800 */
    "\taddiu $1,$0,32\n"   /* 8013FCD8: 24010020 */
    "\tbeq $2,$1,.L8013FCC8_8013FD54\n"   /* 8013FCDC: 1041001d */
    "\tnop\n"   /* 8013FCE0: 00000000 */
    "\tandi $8,$2,0x1\n"   /* 8013FCE4: 30480001 */
    "\taddiu $10,$0,-2\n"   /* 8013FCE8: 240afffe */
    "\tand $10,$2,$10\n"   /* 8013FCEC: 004a5024 */
    "\taddiu $9,$0,19\n"   /* 8013FCF0: 24090013 */
    "\tsub $9,$9,$10\n"   /* 8013FCF4: 012a4822 */
    "\tsra $9,$9,0x1\n"   /* 8013FCF8: 00094843 */
    "\taddi $11,$10,-24\n"   /* 8013FCFC: 214bffe8 */
    "\tbltz $11,.L8013FCC8_8013FD10\n"   /* 8013FD00: 05600003 */
    "\tnop\n"   /* 8013FD04: 00000000 */
    "\tsllv $12,$4,$11\n"   /* 8013FD08: 01646004 */
    "\tbeq $0,$0,.L8013FCC8_8013FD1C\n"   /* 8013FD0C: 10000003 */
    ".L8013FCC8_8013FD10:\n"
    "\taddiu $11,$0,24\n"   /* 8013FD10: 240b0018 */
    "\tsub $11,$11,$10\n"   /* 8013FD14: 016a5822 */
    "\tsrav $12,$4,$11\n"   /* 8013FD18: 01646007 */
    ".L8013FCC8_8013FD1C:\n"
    "\taddi $12,$12,-64\n"   /* 8013FD1C: 218cffc0 */
    "\tsll $12,$12,0x1\n"   /* 8013FD20: 000c6040 */
    "\tlui $13,%hi(D_801777B0)\n"   /* 8013FD24: 3c0d8017 */
    "\taddu $13,$13,$12\n"   /* 8013FD28: 01ac6821 */
    "\tlh $13,%lo(D_801777B0)($13)\n"   /* 8013FD2C: 85ad77b0 */
    "\tnop\n"   /* 8013FD30: 00000000 */
    "\tbltz $9,.L8013FCC8_8013FD48\n"   /* 8013FD34: 05200004 */
    "\tnop\n"   /* 8013FD38: 00000000 */
    "\tsllv $2,$13,$9\n"   /* 8013FD3C: 012d1004 */
    "\tjr $31\n"   /* 8013FD40: 03e00008 */
    "\tnop\n"   /* 8013FD44: 00000000 */
    ".L8013FCC8_8013FD48:\n"
    "\tneg $9,$9\n"   /* 8013FD48: 00094822 */
    "\tjr $31\n"   /* 8013FD4C: 03e00008 */
    "\tsrlv $2,$13,$9\n"   /* 8013FD50: 012d1006 */
    ".L8013FCC8_8013FD54:\n"
    "\tjr $31\n"   /* 8013FD54: 03e00008 */
    "\taddiu $2,$0,0\n"   /* 8013FD58: 24020000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8013FCC8\n"
    ".size sub_8013FCC8,.-sub_8013FCC8\n"
    ".text\n");

/* InvSquareRoot: inverse square root as mantissa (*a1) and shift (*a2) via LZC and the 1/sqrt table; returns 1, or -1 for 0. Sony assembly. */
__asm__(
    ".section .text_8013FD64,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8013FD64\n"
    ".type sub_8013FD64,@function\n"
    ".ent sub_8013FD64\n"
    "sub_8013FD64:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$30\n"   /* 8013FD64: 4884f000 */
    "\tnop\n"   /* 8013FD68: 00000000 */
    "\tnop\n"   /* 8013FD6C: 00000000 */
    "\tmfc2 $2,$31\n"   /* 8013FD70: 4802f800 */
    "\taddiu $1,$0,32\n"   /* 8013FD74: 24010020 */
    "\tbeq $2,$1,.L8013FD64_8013FDE8\n"   /* 8013FD78: 1041001b */
    "\tnop\n"   /* 8013FD7C: 00000000 */
    "\tbeq $2,$0,.L8013FD64_8013FDE8\n"   /* 8013FD80: 10400019 */
    "\tnop\n"   /* 8013FD84: 00000000 */
    "\tandi $8,$2,0x1\n"   /* 8013FD88: 30480001 */
    "\taddiu $10,$0,-2\n"   /* 8013FD8C: 240afffe */
    "\tand $10,$2,$10\n"   /* 8013FD90: 004a5024 */
    "\taddiu $9,$0,31\n"   /* 8013FD94: 2409001f */
    "\tsub $9,$9,$10\n"   /* 8013FD98: 012a4822 */
    "\tsra $9,$9,0x1\n"   /* 8013FD9C: 00094843 */
    "\taddi $11,$10,-24\n"   /* 8013FDA0: 214bffe8 */
    "\tbltz $11,.L8013FD64_8013FDB4\n"   /* 8013FDA4: 05600003 */
    "\tnop\n"   /* 8013FDA8: 00000000 */
    "\tsllv $12,$4,$11\n"   /* 8013FDAC: 01646004 */
    "\tbeq $0,$0,.L8013FD64_8013FDC0\n"   /* 8013FDB0: 10000003 */
    ".L8013FD64_8013FDB4:\n"
    "\taddiu $11,$0,24\n"   /* 8013FDB4: 240b0018 */
    "\tsub $11,$11,$10\n"   /* 8013FDB8: 016a5822 */
    "\tsrav $12,$4,$11\n"   /* 8013FDBC: 01646007 */
    ".L8013FD64_8013FDC0:\n"
    "\taddi $12,$12,-64\n"   /* 8013FDC0: 218cffc0 */
    "\tsll $12,$12,0x1\n"   /* 8013FDC4: 000c6040 */
    "\tlui $13,%hi(D_80177944)\n"   /* 8013FDC8: 3c0d8017 */
    "\taddu $13,$13,$12\n"   /* 8013FDCC: 01ac6821 */
    "\tlh $13,%lo(D_80177944)($13)\n"   /* 8013FDD0: 85ad7944 */
    "\tsw $9,0($6)\n"   /* 8013FDD4: acc90000 */
    "\tsw $13,0($5)\n"   /* 8013FDD8: acad0000 */
    "\taddiu $2,$0,1\n"   /* 8013FDDC: 24020001 */
    "\tjr $31\n"   /* 8013FDE0: 03e00008 */
    "\tnop\n"   /* 8013FDE4: 00000000 */
    ".L8013FD64_8013FDE8:\n"
    "\tjr $31\n"   /* 8013FDE8: 03e00008 */
    "\taddiu $2,$0,-1\n"   /* 8013FDEC: 2402ffff */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8013FD64\n"
    ".size sub_8013FD64,.-sub_8013FD64\n"
    ".text\n");

/* VectorNormalS: normalise a long VECTOR into an SVECTOR (continues in the VectorNormalSS tail). Sony assembly. */
__asm__(
    ".section .text_8013FDF0,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8013FDF0\n"
    ".type sub_8013FDF0,@function\n"
    ".ent sub_8013FDF0\n"
    "sub_8013FDF0:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 8013FDF0: 8c880000 */
    "\tlw $9,4($4)\n"   /* 8013FDF4: 8c890004 */
    "\tlw $10,8($4)\n"   /* 8013FDF8: 8c8a0008 */
    "\t.word 0x10000011\n"   /* 8013FDFC: 10000011 beq $0,$0,0x8013fe44, into VectorNormalSS */
    "\taddu $7,$31,$0\n"   /* 8013FE00: 03e03821 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8013FDF0\n"
    ".size sub_8013FDF0,.-sub_8013FDF0\n"
    ".text\n");

/* VectorNormalSS: normalise an SVECTOR into an SVECTOR via the VectorNormal core. Sony assembly. */
__asm__(
    ".section .text_8013FE30,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8013FE30\n"
    ".type sub_8013FE30,@function\n"
    ".ent sub_8013FE30\n"
    "sub_8013FE30:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlh $8,0($4)\n"   /* 8013FE30: 84880000 */
    "\tlh $9,2($4)\n"   /* 8013FE34: 84890002 */
    "\tlh $10,4($4)\n"   /* 8013FE38: 848a0004 */
    "\tnop\n"   /* 8013FE3C: 00000000 */
    "\taddu $7,$31,$0\n"   /* 8013FE40: 03e03821 */
    "\tjal sub_8013FE60\n"   /* 8013FE44: 0c04ff98 */
    "\tnop\n"   /* 8013FE48: 00000000 */
    "\taddu $31,$7,$0\n"   /* 8013FE4C: 00e0f821 */
    "\tsh $8,0($5)\n"   /* 8013FE50: a4a80000 */
    "\tsh $9,2($5)\n"   /* 8013FE54: a4a90002 */
    "\tjr $31\n"   /* 8013FE58: 03e00008 */
    "\tsh $10,4($5)\n"   /* 8013FE5C: a4aa0004 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8013FE30\n"
    ".size sub_8013FE30,.-sub_8013FE30\n"
    ".text\n");

/* MatrixNormal: re-orthonormalise a rotation matrix (outer products + VectorNormal core). Sony assembly. */
__asm__(
    ".section .text_8013FF1C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8013FF1C\n"
    ".type sub_8013FF1C,@function\n"
    ".ent sub_8013FF1C\n"
    "sub_8013FF1C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlh $8,0($4)\n"   /* 8013FF1C: 84880000 */
    "\tlh $9,2($4)\n"   /* 8013FF20: 84890002 */
    "\tlh $10,4($4)\n"   /* 8013FF24: 848a0004 */
    "\tlh $11,6($4)\n"   /* 8013FF28: 848b0006 */
    "\tlh $12,8($4)\n"   /* 8013FF2C: 848c0008 */
    "\tlh $13,10($4)\n"   /* 8013FF30: 848d000a */
    "\tcfc2 $2,$0\n"   /* 8013FF34: 48420000 */
    "\tcfc2 $3,$2\n"   /* 8013FF38: 48431000 */
    "\tcfc2 $6,$4\n"   /* 8013FF3C: 48462000 */
    "\tctc2 $8,$0\n"   /* 8013FF40: 48c80000 */
    "\tctc2 $9,$2\n"   /* 8013FF44: 48c91000 */
    "\tctc2 $10,$4\n"   /* 8013FF48: 48ca2000 */
    "\tmtc2 $13,$11\n"   /* 8013FF4C: 488d5800 */
    "\tmtc2 $11,$9\n"   /* 8013FF50: 488b4800 */
    "\tmtc2 $12,$10\n"   /* 8013FF54: 488c5000 */
    "\tnop\n"   /* 8013FF58: 00000000 */
    "\t.word 0x4b78000c\n"   /* 8013FF5C: 4b78000c cop2 0x178000c */
    "\tmfc2 $15,$25\n"   /* 8013FF60: 480fc800 */
    "\tmfc2 $24,$26\n"   /* 8013FF64: 4818d000 */
    "\tmfc2 $25,$27\n"   /* 8013FF68: 4819d800 */
    "\tctc2 $11,$0\n"   /* 8013FF6C: 48cb0000 */
    "\tctc2 $12,$2\n"   /* 8013FF70: 48cc1000 */
    "\tctc2 $13,$4\n"   /* 8013FF74: 48cd2000 */
    "\tnop\n"   /* 8013FF78: 00000000 */
    "\t.word 0x4b78000c\n"   /* 8013FF7C: 4b78000c cop2 0x178000c */
    "\tmtc2 $11,$0\n"   /* 8013FF80: 488b0000 */
    "\tmtc2 $12,$1\n"   /* 8013FF84: 488c0800 */
    "\tmtc2 $13,$2\n"   /* 8013FF88: 488d1000 */
    "\tmfc2 $8,$25\n"   /* 8013FF8C: 4808c800 */
    "\tmfc2 $9,$26\n"   /* 8013FF90: 4809d000 */
    "\tmfc2 $10,$27\n"   /* 8013FF94: 480ad800 */
    "\tctc2 $2,$0\n"   /* 8013FF98: 48c20000 */
    "\tctc2 $3,$2\n"   /* 8013FF9C: 48c31000 */
    "\tctc2 $6,$4\n"   /* 8013FFA0: 48c62000 */
    "\taddu $7,$31,$0\n"   /* 8013FFA4: 03e03821 */
    "\tjal sub_8013FE60\n"   /* 8013FFA8: 0c04ff98 */
    "\tnop\n"   /* 8013FFAC: 00000000 */
    "\tsh $8,0($5)\n"   /* 8013FFB0: a4a80000 */
    "\tsh $9,2($5)\n"   /* 8013FFB4: a4a90002 */
    "\tsh $10,4($5)\n"   /* 8013FFB8: a4aa0004 */
    "\tmfc2 $8,$0\n"   /* 8013FFBC: 48080000 */
    "\tmfc2 $9,$1\n"   /* 8013FFC0: 48090800 */
    "\tmfc2 $10,$2\n"   /* 8013FFC4: 480a1000 */
    "\tjal sub_8013FE60\n"   /* 8013FFC8: 0c04ff98 */
    "\tnop\n"   /* 8013FFCC: 00000000 */
    "\tsh $8,6($5)\n"   /* 8013FFD0: a4a80006 */
    "\tsh $9,8($5)\n"   /* 8013FFD4: a4a90008 */
    "\tsh $10,10($5)\n"   /* 8013FFD8: a4aa000a */
    "\taddu $8,$15,$0\n"   /* 8013FFDC: 01e04021 */
    "\taddu $9,$24,$0\n"   /* 8013FFE0: 03004821 */
    "\tjal sub_8013FE60\n"   /* 8013FFE4: 0c04ff98 */
    "\taddu $10,$25,$0\n"   /* 8013FFE8: 03205021 */
    "\taddu $31,$7,$0\n"   /* 8013FFEC: 00e0f821 */
    "\tsh $8,12($5)\n"   /* 8013FFF0: a4a8000c */
    "\tsh $9,14($5)\n"   /* 8013FFF4: a4a9000e */
    "\tjr $31\n"   /* 8013FFF8: 03e00008 */
    "\tsh $10,16($5)\n"   /* 8013FFFC: a4aa0010 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8013FF1C\n"
    ".size sub_8013FF1C,.-sub_8013FF1C\n"
    ".text\n");

/* MulMatrix: m0 = m0 * m1 using the GTE MVMVA on each column. Sony assembly. */
__asm__(
    ".section .text_80140004,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140004\n"
    ".type sub_80140004,@function\n"
    ".ent sub_80140004\n"
    "sub_80140004:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80140004: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80140008: 8c890004 */
    "\tlw $10,8($4)\n"   /* 8014000C: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 80140010: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 80140014: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80140018: 48c80000 */
    "\tctc2 $9,$1\n"   /* 8014001C: 48c90800 */
    "\tctc2 $10,$2\n"   /* 80140020: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 80140024: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80140028: 48cc2000 */
    "\tlhu $8,0($5)\n"   /* 8014002C: 94a80000 */
    "\tlw $9,4($5)\n"   /* 80140030: 8ca90004 */
    "\tlw $10,12($5)\n"   /* 80140034: 8caa000c */
    "\tlui $1,0xffff\n"   /* 80140038: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 8014003C: 01214824 */
    "\tor $8,$8,$9\n"   /* 80140040: 01094025 */
    "\tmtc2 $8,$0\n"   /* 80140044: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80140048: 488a0800 */
    "\tnop\n"   /* 8014004C: 00000000 */
    "\t.word 0x4a486012\n"   /* 80140050: 4a486012 cop2 0x486012 */
    "\tlhu $8,2($5)\n"   /* 80140054: 94a80002 */
    "\tlw $9,8($5)\n"   /* 80140058: 8ca90008 */
    "\tlh $10,14($5)\n"   /* 8014005C: 84aa000e */
    "\tsll $9,$9,0x10\n"   /* 80140060: 00094c00 */
    "\tor $8,$8,$9\n"   /* 80140064: 01094025 */
    "\tmfc2 $11,$9\n"   /* 80140068: 480b4800 */
    "\tmfc2 $12,$10\n"   /* 8014006C: 480c5000 */
    "\tmfc2 $13,$11\n"   /* 80140070: 480d5800 */
    "\tmtc2 $8,$0\n"   /* 80140074: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80140078: 488a0800 */
    "\tnop\n"   /* 8014007C: 00000000 */
    "\t.word 0x4a486012\n"   /* 80140080: 4a486012 cop2 0x486012 */
    "\tlhu $8,4($5)\n"   /* 80140084: 94a80004 */
    "\tlw $9,8($5)\n"   /* 80140088: 8ca90008 */
    "\tlw $10,16($5)\n"   /* 8014008C: 8caa0010 */
    "\tlui $1,0xffff\n"   /* 80140090: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 80140094: 01214824 */
    "\tor $8,$8,$9\n"   /* 80140098: 01094025 */
    "\tmfc2 $14,$9\n"   /* 8014009C: 480e4800 */
    "\tmfc2 $15,$10\n"   /* 801400A0: 480f5000 */
    "\tmfc2 $24,$11\n"   /* 801400A4: 48185800 */
    "\tmtc2 $8,$0\n"   /* 801400A8: 48880000 */
    "\tmtc2 $10,$1\n"   /* 801400AC: 488a0800 */
    "\tnop\n"   /* 801400B0: 00000000 */
    "\t.word 0x4a486012\n"   /* 801400B4: 4a486012 cop2 0x486012 */
    "\tandi $11,$11,0xffff\n"   /* 801400B8: 316bffff */
    "\tsll $14,$14,0x10\n"   /* 801400BC: 000e7400 */
    "\tor $14,$14,$11\n"   /* 801400C0: 01cb7025 */
    "\tsw $14,0($4)\n"   /* 801400C4: ac8e0000 */
    "\tandi $13,$13,0xffff\n"   /* 801400C8: 31adffff */
    "\tsll $24,$24,0x10\n"   /* 801400CC: 0018c400 */
    "\tor $24,$24,$13\n"   /* 801400D0: 030dc025 */
    "\tsw $24,12($4)\n"   /* 801400D4: ac98000c */
    "\tmfc2 $8,$9\n"   /* 801400D8: 48084800 */
    "\tmfc2 $9,$10\n"   /* 801400DC: 48095000 */
    "\tandi $8,$8,0xffff\n"   /* 801400E0: 3108ffff */
    "\tsll $12,$12,0x10\n"   /* 801400E4: 000c6400 */
    "\tor $8,$8,$12\n"   /* 801400E8: 010c4025 */
    "\tsw $8,4($4)\n"   /* 801400EC: ac880004 */
    "\tandi $15,$15,0xffff\n"   /* 801400F0: 31efffff */
    "\tsll $9,$9,0x10\n"   /* 801400F4: 00094c00 */
    "\tor $9,$9,$15\n"   /* 801400F8: 012f4825 */
    "\tsw $9,8($4)\n"   /* 801400FC: ac890008 */
    "\tswc2 $11,16($4)\n"   /* 80140100: e88b0010 */
    "\taddu $2,$4,$0\n"   /* 80140104: 00801021 */
    "\tjr $31\n"   /* 80140108: 03e00008 */
    "\tnop\n"   /* 8014010C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140004\n"
    ".size sub_80140004,.-sub_80140004\n"
    ".text\n");

/* MulMatrix2: m1 = m0 * m1 using the GTE MVMVA on each column. Sony assembly. */
__asm__(
    ".section .text_80140110,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140110\n"
    ".type sub_80140110,@function\n"
    ".ent sub_80140110\n"
    "sub_80140110:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80140110: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80140114: 8c890004 */
    "\tlw $10,8($4)\n"   /* 80140118: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 8014011C: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 80140120: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80140124: 48c80000 */
    "\tctc2 $9,$1\n"   /* 80140128: 48c90800 */
    "\tctc2 $10,$2\n"   /* 8014012C: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 80140130: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80140134: 48cc2000 */
    "\tlhu $8,0($5)\n"   /* 80140138: 94a80000 */
    "\tlw $9,4($5)\n"   /* 8014013C: 8ca90004 */
    "\tlw $10,12($5)\n"   /* 80140140: 8caa000c */
    "\tlui $1,0xffff\n"   /* 80140144: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 80140148: 01214824 */
    "\tor $8,$8,$9\n"   /* 8014014C: 01094025 */
    "\tmtc2 $8,$0\n"   /* 80140150: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80140154: 488a0800 */
    "\tnop\n"   /* 80140158: 00000000 */
    "\t.word 0x4a486012\n"   /* 8014015C: 4a486012 cop2 0x486012 */
    "\tlhu $8,2($5)\n"   /* 80140160: 94a80002 */
    "\tlw $9,8($5)\n"   /* 80140164: 8ca90008 */
    "\tlh $10,14($5)\n"   /* 80140168: 84aa000e */
    "\tsll $9,$9,0x10\n"   /* 8014016C: 00094c00 */
    "\tor $8,$8,$9\n"   /* 80140170: 01094025 */
    "\tmfc2 $11,$9\n"   /* 80140174: 480b4800 */
    "\tmfc2 $12,$10\n"   /* 80140178: 480c5000 */
    "\tmfc2 $13,$11\n"   /* 8014017C: 480d5800 */
    "\tmtc2 $8,$0\n"   /* 80140180: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80140184: 488a0800 */
    "\tnop\n"   /* 80140188: 00000000 */
    "\t.word 0x4a486012\n"   /* 8014018C: 4a486012 cop2 0x486012 */
    "\tlhu $8,4($5)\n"   /* 80140190: 94a80004 */
    "\tlw $9,8($5)\n"   /* 80140194: 8ca90008 */
    "\tlw $10,16($5)\n"   /* 80140198: 8caa0010 */
    "\tlui $1,0xffff\n"   /* 8014019C: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 801401A0: 01214824 */
    "\tor $8,$8,$9\n"   /* 801401A4: 01094025 */
    "\tmfc2 $14,$9\n"   /* 801401A8: 480e4800 */
    "\tmfc2 $15,$10\n"   /* 801401AC: 480f5000 */
    "\tmfc2 $24,$11\n"   /* 801401B0: 48185800 */
    "\tmtc2 $8,$0\n"   /* 801401B4: 48880000 */
    "\tmtc2 $10,$1\n"   /* 801401B8: 488a0800 */
    "\tnop\n"   /* 801401BC: 00000000 */
    "\t.word 0x4a486012\n"   /* 801401C0: 4a486012 cop2 0x486012 */
    "\tandi $11,$11,0xffff\n"   /* 801401C4: 316bffff */
    "\tsll $14,$14,0x10\n"   /* 801401C8: 000e7400 */
    "\tor $14,$14,$11\n"   /* 801401CC: 01cb7025 */
    "\tsw $14,0($5)\n"   /* 801401D0: acae0000 */
    "\tandi $13,$13,0xffff\n"   /* 801401D4: 31adffff */
    "\tsll $24,$24,0x10\n"   /* 801401D8: 0018c400 */
    "\tor $24,$24,$13\n"   /* 801401DC: 030dc025 */
    "\tsw $24,12($5)\n"   /* 801401E0: acb8000c */
    "\tmfc2 $8,$9\n"   /* 801401E4: 48084800 */
    "\tmfc2 $9,$10\n"   /* 801401E8: 48095000 */
    "\tandi $8,$8,0xffff\n"   /* 801401EC: 3108ffff */
    "\tsll $12,$12,0x10\n"   /* 801401F0: 000c6400 */
    "\tor $8,$8,$12\n"   /* 801401F4: 010c4025 */
    "\tsw $8,4($5)\n"   /* 801401F8: aca80004 */
    "\tandi $15,$15,0xffff\n"   /* 801401FC: 31efffff */
    "\tsll $9,$9,0x10\n"   /* 80140200: 00094c00 */
    "\tor $9,$9,$15\n"   /* 80140204: 012f4825 */
    "\tsw $9,8($5)\n"   /* 80140208: aca90008 */
    "\tswc2 $11,16($5)\n"   /* 8014020C: e8ab0010 */
    "\taddu $2,$5,$0\n"   /* 80140210: 00a01021 */
    "\tjr $31\n"   /* 80140214: 03e00008 */
    "\tnop\n"   /* 80140218: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140110\n"
    ".size sub_80140110,.-sub_80140110\n"
    ".text\n");

/* ApplyMatrix: long VECTOR = m * SVECTOR (MVMVA). Sony assembly. */
__asm__(
    ".section .text_8014021C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014021C\n"
    ".type sub_8014021C,@function\n"
    ".ent sub_8014021C\n"
    "sub_8014021C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 8014021C: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80140220: 8c890004 */
    "\tlw $10,8($4)\n"   /* 80140224: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 80140228: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 8014022C: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80140230: 48c80000 */
    "\tctc2 $9,$1\n"   /* 80140234: 48c90800 */
    "\tctc2 $10,$2\n"   /* 80140238: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 8014023C: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80140240: 48cc2000 */
    "\tlwc2 $0,0($5)\n"   /* 80140244: c8a00000 */
    "\tlwc2 $1,4($5)\n"   /* 80140248: c8a10004 */
    "\tnop\n"   /* 8014024C: 00000000 */
    "\t.word 0x4a486012\n"   /* 80140250: 4a486012 cop2 0x486012 */
    "\tswc2 $25,0($6)\n"   /* 80140254: e8d90000 */
    "\tswc2 $26,4($6)\n"   /* 80140258: e8da0004 */
    "\tswc2 $27,8($6)\n"   /* 8014025C: e8db0008 */
    "\taddu $2,$6,$0\n"   /* 80140260: 00c01021 */
    "\tjr $31\n"   /* 80140264: 03e00008 */
    "\tnop\n"   /* 80140268: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014021C\n"
    ".size sub_8014021C,.-sub_8014021C\n"
    ".text\n");

/* ApplyMatrixSV: SVECTOR = m * SVECTOR (MVMVA). Sony assembly. */
__asm__(
    ".section .text_8014026C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014026C\n"
    ".type sub_8014026C,@function\n"
    ".ent sub_8014026C\n"
    "sub_8014026C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 8014026C: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80140270: 8c890004 */
    "\tlw $10,8($4)\n"   /* 80140274: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 80140278: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 8014027C: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80140280: 48c80000 */
    "\tctc2 $9,$1\n"   /* 80140284: 48c90800 */
    "\tctc2 $10,$2\n"   /* 80140288: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 8014028C: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80140290: 48cc2000 */
    "\tlwc2 $0,0($5)\n"   /* 80140294: c8a00000 */
    "\tlwc2 $1,4($5)\n"   /* 80140298: c8a10004 */
    "\tnop\n"   /* 8014029C: 00000000 */
    "\t.word 0x4a486012\n"   /* 801402A0: 4a486012 cop2 0x486012 */
    "\tmfc2 $8,$9\n"   /* 801402A4: 48084800 */
    "\tmfc2 $9,$10\n"   /* 801402A8: 48095000 */
    "\tmfc2 $10,$11\n"   /* 801402AC: 480a5800 */
    "\tsh $8,0($6)\n"   /* 801402B0: a4c80000 */
    "\tsh $9,2($6)\n"   /* 801402B4: a4c90002 */
    "\tsh $10,4($6)\n"   /* 801402B8: a4ca0004 */
    "\taddu $2,$6,$0\n"   /* 801402BC: 00c01021 */
    "\tjr $31\n"   /* 801402C0: 03e00008 */
    "\tnop\n"   /* 801402C4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014026C\n"
    ".size sub_8014026C,.-sub_8014026C\n"
    ".text\n");

/* TransMatrix: copy a translation VECTOR into m->t. Sony assembly. */
__asm__(
    ".section .text_801402C8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_801402C8\n"
    ".type sub_801402C8,@function\n"
    ".ent sub_801402C8\n"
    "sub_801402C8:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($5)\n"   /* 801402C8: 8ca80000 */
    "\tlw $9,4($5)\n"   /* 801402CC: 8ca90004 */
    "\tlw $10,8($5)\n"   /* 801402D0: 8caa0008 */
    "\tsw $8,20($4)\n"   /* 801402D4: ac880014 */
    "\tsw $9,24($4)\n"   /* 801402D8: ac890018 */
    "\tsw $10,28($4)\n"   /* 801402DC: ac8a001c */
    "\taddu $2,$4,$0\n"   /* 801402E0: 00801021 */
    "\tjr $31\n"   /* 801402E4: 03e00008 */
    "\tnop\n"   /* 801402E8: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_801402C8\n"
    ".size sub_801402C8,.-sub_801402C8\n"
    ".text\n");

/* ScaleMatrix: scale m by a 4.12 VECTOR. Sony assembly. */
__asm__(
    ".section .text_801402EC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_801402EC\n"
    ".type sub_801402EC,@function\n"
    ".ent sub_801402EC\n"
    "sub_801402EC:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 801402EC: 8c880000 */
    "\tlw $11,0($5)\n"   /* 801402F0: 8cab0000 */
    "\tandi $9,$8,0xffff\n"   /* 801402F4: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 801402F8: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 801402FC: 00094c03 */
    "\tmultu $9,$11\n"   /* 80140300: 012b0019 */
    "\tlw $12,4($5)\n"   /* 80140304: 8cac0004 */
    "\tsra $10,$8,0x10\n"   /* 80140308: 00085403 */
    "\tlw $13,8($5)\n"   /* 8014030C: 8cad0008 */
    "\tlw $8,4($4)\n"   /* 80140310: 8c880004 */
    "\taddu $2,$4,$0\n"   /* 80140314: 00801021 */
    "\tmflo $9\n"   /* 80140318: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 8014031C: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 80140320: 3129ffff */
    "\tmultu $10,$12\n"   /* 80140324: 014c0019 */
    "\tmflo $10\n"   /* 80140328: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 8014032C: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 80140330: 000a5400 */
    "\tor $9,$9,$10\n"   /* 80140334: 012a4825 */
    "\tsw $9,0($4)\n"   /* 80140338: ac890000 */
    "\tandi $9,$8,0xffff\n"   /* 8014033C: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 80140340: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 80140344: 00094c03 */
    "\tmultu $9,$13\n"   /* 80140348: 012d0019 */
    "\tsra $10,$8,0x10\n"   /* 8014034C: 00085403 */
    "\tlw $8,8($4)\n"   /* 80140350: 8c880008 */
    "\tmflo $9\n"   /* 80140354: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80140358: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 8014035C: 3129ffff */
    "\tmultu $10,$11\n"   /* 80140360: 014b0019 */
    "\tmflo $10\n"   /* 80140364: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 80140368: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 8014036C: 000a5400 */
    "\tor $9,$9,$10\n"   /* 80140370: 012a4825 */
    "\tsw $9,4($4)\n"   /* 80140374: ac890004 */
    "\tandi $9,$8,0xffff\n"   /* 80140378: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 8014037C: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 80140380: 00094c03 */
    "\tmultu $9,$12\n"   /* 80140384: 012c0019 */
    "\tsra $10,$8,0x10\n"   /* 80140388: 00085403 */
    "\tlw $8,12($4)\n"   /* 8014038C: 8c88000c */
    "\tmflo $9\n"   /* 80140390: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80140394: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 80140398: 3129ffff */
    "\tmultu $10,$13\n"   /* 8014039C: 014d0019 */
    "\tmflo $10\n"   /* 801403A0: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 801403A4: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 801403A8: 000a5400 */
    "\tor $9,$9,$10\n"   /* 801403AC: 012a4825 */
    "\tsw $9,8($4)\n"   /* 801403B0: ac890008 */
    "\tandi $9,$8,0xffff\n"   /* 801403B4: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 801403B8: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 801403BC: 00094c03 */
    "\tmultu $9,$11\n"   /* 801403C0: 012b0019 */
    "\tsra $10,$8,0x10\n"   /* 801403C4: 00085403 */
    "\tlw $8,16($4)\n"   /* 801403C8: 8c880010 */
    "\tmflo $9\n"   /* 801403CC: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 801403D0: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 801403D4: 3129ffff */
    "\tmultu $10,$12\n"   /* 801403D8: 014c0019 */
    "\tmflo $10\n"   /* 801403DC: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 801403E0: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 801403E4: 000a5400 */
    "\tor $9,$9,$10\n"   /* 801403E8: 012a4825 */
    "\tsw $9,12($4)\n"   /* 801403EC: ac89000c */
    "\tandi $9,$8,0xffff\n"   /* 801403F0: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 801403F4: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 801403F8: 00094c03 */
    "\tmultu $9,$13\n"   /* 801403FC: 012d0019 */
    "\tmflo $9\n"   /* 80140400: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80140404: 00094b03 */
    "\tjr $31\n"   /* 80140408: 03e00008 */
    "\tsw $9,16($4)\n"   /* 8014040C: ac890010 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_801402EC\n"
    ".size sub_801402EC,.-sub_801402EC\n"
    ".text\n");

/* LocalLight: light matrix * SVECTOR (MVMVA with LLM) into a long VECTOR. Sony assembly. */
__asm__(
    ".section .text_8014056C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014056C\n"
    ".type sub_8014056C,@function\n"
    ".ent sub_8014056C\n"
    "sub_8014056C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 8014056C: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 80140570: c8810004 */
    "\tnop\n"   /* 80140574: 00000000 */
    "\t.word 0x4a4a6412\n"   /* 80140578: 4a4a6412 cop2 0x4a6412 */
    "\tswc2 $9,0($5)\n"   /* 8014057C: e8a90000 */
    "\tswc2 $10,4($5)\n"   /* 80140580: e8aa0004 */
    "\tswc2 $11,8($5)\n"   /* 80140584: e8ab0008 */
    "\tjr $31\n"   /* 80140588: 03e00008 */
    "\tnop\n"   /* 8014058C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014056C\n"
    ".size sub_8014056C,.-sub_8014056C\n"
    ".text\n");

/* DpqColor: depth-cue a CVECTOR toward the far color (DPCS). Sony assembly. */
__asm__(
    ".section .text_80140590,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140590\n"
    ".type sub_80140590,@function\n"
    ".ent sub_80140590\n"
    "sub_80140590:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $6,0($4)\n"   /* 80140590: c8860000 */
    "\tmtc2 $5,$8\n"   /* 80140594: 48854000 */
    "\tnop\n"   /* 80140598: 00000000 */
    "\t.word 0x4a780010\n"   /* 8014059C: 4a780010 cop2 0x780010 */
    "\tswc2 $22,0($6)\n"   /* 801405A0: e8d60000 */
    "\tjr $31\n"   /* 801405A4: 03e00008 */
    "\tnop\n"   /* 801405A8: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140590\n"
    ".size sub_80140590,.-sub_80140590\n"
    ".text\n");

/* NormalColor: normal to color through the light and color matrices (NCS). Sony assembly. */
__asm__(
    ".section .text_801405AC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_801405AC\n"
    ".type sub_801405AC,@function\n"
    ".ent sub_801405AC\n"
    "sub_801405AC:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 801405AC: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 801405B0: c8810004 */
    "\tnop\n"   /* 801405B4: 00000000 */
    "\t.word 0x4ac8041e\n"   /* 801405B8: 4ac8041e cop2 0xc8041e */
    "\tswc2 $22,0($5)\n"   /* 801405BC: e8b60000 */
    "\tjr $31\n"   /* 801405C0: 03e00008 */
    "\tnop\n"   /* 801405C4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_801405AC\n"
    ".size sub_801405AC,.-sub_801405AC\n"
    ".text\n");

/* NormalColor3: NormalColor for three normals (NCT). Sony assembly. */
__asm__(
    ".section .text_801405C8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_801405C8\n"
    ".type sub_801405C8,@function\n"
    ".ent sub_801405C8\n"
    "sub_801405C8:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 801405C8: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 801405CC: c8810004 */
    "\tlwc2 $2,0($5)\n"   /* 801405D0: c8a20000 */
    "\tlwc2 $3,4($5)\n"   /* 801405D4: c8a30004 */
    "\tlwc2 $4,0($6)\n"   /* 801405D8: c8c40000 */
    "\tlwc2 $5,4($6)\n"   /* 801405DC: c8c50004 */
    "\tnop\n"   /* 801405E0: 00000000 */
    "\t.word 0x4ad80420\n"   /* 801405E4: 4ad80420 cop2 0xd80420 */
    "\tlw $8,16($29)\n"   /* 801405E8: 8fa80010 */
    "\tlw $9,20($29)\n"   /* 801405EC: 8fa90014 */
    "\tswc2 $20,0($7)\n"   /* 801405F0: e8f40000 */
    "\tswc2 $21,0($8)\n"   /* 801405F4: e9150000 */
    "\tswc2 $22,0($9)\n"   /* 801405F8: e9360000 */
    "\tjr $31\n"   /* 801405FC: 03e00008 */
    "\tnop\n"   /* 80140600: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_801405C8\n"
    ".size sub_801405C8,.-sub_801405C8\n"
    ".text\n");

/* NormalColorDpq: normal to depth-cued color (NCDS). Sony assembly. */
__asm__(
    ".section .text_80140604,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140604\n"
    ".type sub_80140604,@function\n"
    ".ent sub_80140604\n"
    "sub_80140604:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 80140604: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 80140608: c8810004 */
    "\tlwc2 $6,0($5)\n"   /* 8014060C: c8a60000 */
    "\tmtc2 $6,$8\n"   /* 80140610: 48864000 */
    "\tnop\n"   /* 80140614: 00000000 */
    "\t.word 0x4ae80413\n"   /* 80140618: 4ae80413 cop2 0xe80413 */
    "\tswc2 $22,0($7)\n"   /* 8014061C: e8f60000 */
    "\tjr $31\n"   /* 80140620: 03e00008 */
    "\tnop\n"   /* 80140624: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140604\n"
    ".size sub_80140604,.-sub_80140604\n"
    ".text\n");

/* NormalColorDpq3: NormalColorDpq for three normals (NCDT). Sony assembly. */
__asm__(
    ".section .text_80140628,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140628\n"
    ".type sub_80140628,@function\n"
    ".ent sub_80140628\n"
    "sub_80140628:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 80140628: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 8014062C: c8810004 */
    "\tlwc2 $2,0($5)\n"   /* 80140630: c8a20000 */
    "\tlwc2 $3,4($5)\n"   /* 80140634: c8a30004 */
    "\tlwc2 $4,0($6)\n"   /* 80140638: c8c40000 */
    "\tlwc2 $5,4($6)\n"   /* 8014063C: c8c50004 */
    "\tlwc2 $6,0($7)\n"   /* 80140640: c8e60000 */
    "\tlwc2 $8,16($29)\n"   /* 80140644: cba80010 */
    "\tnop\n"   /* 80140648: 00000000 */
    "\t.word 0x4af80416\n"   /* 8014064C: 4af80416 cop2 0xf80416 */
    "\tlw $8,20($29)\n"   /* 80140650: 8fa80014 */
    "\tlw $9,24($29)\n"   /* 80140654: 8fa90018 */
    "\tlw $10,28($29)\n"   /* 80140658: 8faa001c */
    "\tswc2 $20,0($8)\n"   /* 8014065C: e9140000 */
    "\tswc2 $21,0($9)\n"   /* 80140660: e9350000 */
    "\tswc2 $22,0($10)\n"   /* 80140664: e9560000 */
    "\tjr $31\n"   /* 80140668: 03e00008 */
    "\tnop\n"   /* 8014066C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140628\n"
    ".size sub_80140628,.-sub_80140628\n"
    ".text\n");

/* NormalColorCol3: normal to color times primary color, three normals (NCCT). Sony assembly. */
__asm__(
    ".section .text_80140690,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140690\n"
    ".type sub_80140690,@function\n"
    ".ent sub_80140690\n"
    "sub_80140690:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 80140690: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 80140694: c8810004 */
    "\tlwc2 $2,0($5)\n"   /* 80140698: c8a20000 */
    "\tlwc2 $3,4($5)\n"   /* 8014069C: c8a30004 */
    "\tlwc2 $4,0($6)\n"   /* 801406A0: c8c40000 */
    "\tlwc2 $5,4($6)\n"   /* 801406A4: c8c50004 */
    "\tlwc2 $6,0($7)\n"   /* 801406A8: c8e60000 */
    "\tnop\n"   /* 801406AC: 00000000 */
    "\t.word 0x4b18043f\n"   /* 801406B0: 4b18043f cop2 0x118043f */
    "\tlw $8,16($29)\n"   /* 801406B4: 8fa80010 */
    "\tlw $9,20($29)\n"   /* 801406B8: 8fa90014 */
    "\tlw $10,24($29)\n"   /* 801406BC: 8faa0018 */
    "\tswc2 $20,0($8)\n"   /* 801406C0: e9140000 */
    "\tswc2 $21,0($9)\n"   /* 801406C4: e9350000 */
    "\tswc2 $22,0($10)\n"   /* 801406C8: e9560000 */
    "\tjr $31\n"   /* 801406CC: 03e00008 */
    "\tnop\n"   /* 801406D0: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140690\n"
    ".size sub_80140690,.-sub_80140690\n"
    ".text\n");

/* ColorDpq: light vector to depth-cued color (CDP). Sony assembly. */
__asm__(
    ".section .text_801406D4,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_801406D4\n"
    ".type sub_801406D4,@function\n"
    ".ent sub_801406D4\n"
    "sub_801406D4:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $9,0($4)\n"   /* 801406D4: c8890000 */
    "\tlwc2 $10,4($4)\n"   /* 801406D8: c88a0004 */
    "\tlwc2 $11,8($4)\n"   /* 801406DC: c88b0008 */
    "\tlwc2 $6,0($5)\n"   /* 801406E0: c8a60000 */
    "\tmtc2 $6,$8\n"   /* 801406E4: 48864000 */
    "\tnop\n"   /* 801406E8: 00000000 */
    "\t.word 0x4b280414\n"   /* 801406EC: 4b280414 cop2 0x1280414 */
    "\tswc2 $22,0($7)\n"   /* 801406F0: e8f60000 */
    "\tjr $31\n"   /* 801406F4: 03e00008 */
    "\tnop\n"   /* 801406F8: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_801406D4\n"
    ".size sub_801406D4,.-sub_801406D4\n"
    ".text\n");

/* ColorCol: light vector to color times primary color (CC). Sony assembly. */
__asm__(
    ".section .text_801406FC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_801406FC\n"
    ".type sub_801406FC,@function\n"
    ".ent sub_801406FC\n"
    "sub_801406FC:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $9,0($4)\n"   /* 801406FC: c8890000 */
    "\tlwc2 $10,4($4)\n"   /* 80140700: c88a0004 */
    "\tlwc2 $11,8($4)\n"   /* 80140704: c88b0008 */
    "\tlwc2 $6,0($5)\n"   /* 80140708: c8a60000 */
    "\tnop\n"   /* 8014070C: 00000000 */
    "\t.word 0x4b38041c\n"   /* 80140710: 4b38041c cop2 0x138041c */
    "\tswc2 $22,0($6)\n"   /* 80140714: e8d60000 */
    "\tjr $31\n"   /* 80140718: 03e00008 */
    "\tnop\n"   /* 8014071C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_801406FC\n"
    ".size sub_801406FC,.-sub_801406FC\n"
    ".text\n");

/* NormalClip: load three screen XYs and return the NCLIP outer product (MAC0). Sony assembly. */
__asm__(
    ".section .text_80140720,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140720\n"
    ".type sub_80140720,@function\n"
    ".ent sub_80140720\n"
    "sub_80140720:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$12\n"   /* 80140720: 48846000 */
    "\tmtc2 $6,$14\n"   /* 80140724: 48867000 */
    "\tmtc2 $5,$13\n"   /* 80140728: 48856800 */
    "\tnop\n"   /* 8014072C: 00000000 */
    "\tnop\n"   /* 80140730: 00000000 */
    "\t.word 0x4b400006\n"   /* 80140734: 4b400006 cop2 0x1400006 */
    "\tmfc2 $2,$24\n"   /* 80140738: 4802c000 */
    "\tjr $31\n"   /* 8014073C: 03e00008 */
    "\tnop\n"   /* 80140740: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140720\n"
    ".size sub_80140720,.-sub_80140720\n"
    ".text\n");

/* unnamed NCLIP helper: NCLIP on the SXY FIFO already loaded, returns MAC0. Sony assembly. */
__asm__(
    ".section .text_80140744,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140744\n"
    ".type sub_80140744,@function\n"
    ".ent sub_80140744\n"
    "sub_80140744:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\t.word 0x4b400006\n"   /* 80140744: 4b400006 cop2 0x1400006 */
    "\tmfc2 $2,$24\n"   /* 80140748: 4802c000 */
    "\tjr $31\n"   /* 8014074C: 03e00008 */
    "\tnop\n"   /* 80140750: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140744\n"
    ".size sub_80140744,.-sub_80140744\n"
    ".text\n");

/* AverageSZ3: AVSZ3 of the SZ FIFO, returns OTZ. Sony assembly. */
__asm__(
    ".section .text_80140754,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140754\n"
    ".type sub_80140754,@function\n"
    ".ent sub_80140754\n"
    "sub_80140754:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\t.word 0x4b58002d\n"   /* 80140754: 4b58002d cop2 0x158002d */
    "\tmfc2 $2,$7\n"   /* 80140758: 48023800 */
    "\tjr $31\n"   /* 8014075C: 03e00008 */
    "\tnop\n"   /* 80140760: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140754\n"
    ".size sub_80140754,.-sub_80140754\n"
    ".text\n");

/* AverageSZ4: AVSZ4 of the SZ FIFO, returns OTZ. Sony assembly. */
__asm__(
    ".section .text_80140764,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140764\n"
    ".type sub_80140764,@function\n"
    ".ent sub_80140764\n"
    "sub_80140764:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\t.word 0x4b68002e\n"   /* 80140764: 4b68002e cop2 0x168002e */
    "\tmfc2 $2,$7\n"   /* 80140768: 48023800 */
    "\tjr $31\n"   /* 8014076C: 03e00008 */
    "\tnop\n"   /* 80140770: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140764\n"
    ".size sub_80140764,.-sub_80140764\n"
    ".text\n");

/* LightColor: light-color matrix * VECTOR (MVMVA with LCM). Sony assembly. */
__asm__(
    ".section .text_80140774,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140774\n"
    ".type sub_80140774,@function\n"
    ".ent sub_80140774\n"
    "sub_80140774:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $9,0($4)\n"   /* 80140774: c8890000 */
    "\tlwc2 $10,4($4)\n"   /* 80140778: c88a0004 */
    "\tlwc2 $11,8($4)\n"   /* 8014077C: c88b0008 */
    "\tnop\n"   /* 80140780: 00000000 */
    "\t.word 0x4a4da412\n"   /* 80140784: 4a4da412 cop2 0x4da412 */
    "\tswc2 $9,0($5)\n"   /* 80140788: e8a90000 */
    "\tswc2 $10,4($5)\n"   /* 8014078C: e8aa0004 */
    "\tswc2 $11,8($5)\n"   /* 80140790: e8ab0008 */
    "\tjr $31\n"   /* 80140794: 03e00008 */
    "\tnop\n"   /* 80140798: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140774\n"
    ".size sub_80140774,.-sub_80140774\n"
    ".text\n");

/* DpqColorLight: light vector plus color, depth-cued (DPCL). Sony assembly. */
__asm__(
    ".section .text_8014079C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014079C\n"
    ".type sub_8014079C,@function\n"
    ".ent sub_8014079C\n"
    "sub_8014079C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $9,0($4)\n"   /* 8014079C: c8890000 */
    "\tlwc2 $10,4($4)\n"   /* 801407A0: c88a0004 */
    "\tlwc2 $11,8($4)\n"   /* 801407A4: c88b0008 */
    "\tlwc2 $6,0($5)\n"   /* 801407A8: c8a60000 */
    "\tmtc2 $6,$8\n"   /* 801407AC: 48864000 */
    "\tnop\n"   /* 801407B0: 00000000 */
    "\t.word 0x4a680029\n"   /* 801407B4: 4a680029 cop2 0x680029 */
    "\tswc2 $22,0($7)\n"   /* 801407B8: e8f60000 */
    "\tjr $31\n"   /* 801407BC: 03e00008 */
    "\tnop\n"   /* 801407C0: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014079C\n"
    ".size sub_8014079C,.-sub_8014079C\n"
    ".text\n");

/* DpqColor3: DpqColor for three colors (DPCT). Sony assembly. */
__asm__(
    ".section .text_801407C4,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_801407C4\n"
    ".type sub_801407C4,@function\n"
    ".ent sub_801407C4\n"
    "sub_801407C4:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $20,0($4)\n"   /* 801407C4: c8940000 */
    "\tlwc2 $21,0($5)\n"   /* 801407C8: c8b50000 */
    "\tlwc2 $22,0($6)\n"   /* 801407CC: c8d60000 */
    "\tlwc2 $6,0($6)\n"   /* 801407D0: c8c60000 */
    "\tmtc2 $7,$8\n"   /* 801407D4: 48874000 */
    "\tnop\n"   /* 801407D8: 00000000 */
    "\t.word 0x4af8002a\n"   /* 801407DC: 4af8002a cop2 0xf8002a */
    "\tlw $8,16($29)\n"   /* 801407E0: 8fa80010 */
    "\tlw $9,20($29)\n"   /* 801407E4: 8fa90014 */
    "\tlw $10,24($29)\n"   /* 801407E8: 8faa0018 */
    "\tswc2 $20,0($8)\n"   /* 801407EC: e9140000 */
    "\tswc2 $21,0($9)\n"   /* 801407F0: e9350000 */
    "\tswc2 $22,0($10)\n"   /* 801407F4: e9560000 */
    "\tjr $31\n"   /* 801407F8: 03e00008 */
    "\tnop\n"   /* 801407FC: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_801407C4\n"
    ".size sub_801407C4,.-sub_801407C4\n"
    ".text\n");

/* Intpl: interpolate a VECTOR toward the far color by p (INTPL). Sony assembly. */
__asm__(
    ".section .text_80140800,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140800\n"
    ".type sub_80140800,@function\n"
    ".ent sub_80140800\n"
    "sub_80140800:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $9,0($4)\n"   /* 80140800: c8890000 */
    "\tlwc2 $10,4($4)\n"   /* 80140804: c88a0004 */
    "\tlwc2 $11,8($4)\n"   /* 80140808: c88b0008 */
    "\tmtc2 $5,$8\n"   /* 8014080C: 48854000 */
    "\tnop\n"   /* 80140810: 00000000 */
    "\t.word 0x4a980011\n"   /* 80140814: 4a980011 cop2 0x980011 */
    "\tswc2 $22,0($6)\n"   /* 80140818: e8d60000 */
    "\tjr $31\n"   /* 8014081C: 03e00008 */
    "\tnop\n"   /* 80140820: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140800\n"
    ".size sub_80140800,.-sub_80140800\n"
    ".text\n");

/* Square12: per-component square of a VECTOR, >> 12 (SQR sf=1); returns the output pointer. Sony assembly. */
__asm__(
    ".section .text_80140824,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140824\n"
    ".type sub_80140824,@function\n"
    ".ent sub_80140824\n"
    "sub_80140824:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $9,0($4)\n"   /* 80140824: c8890000 */
    "\tlwc2 $10,4($4)\n"   /* 80140828: c88a0004 */
    "\tlwc2 $11,8($4)\n"   /* 8014082C: c88b0008 */
    "\tnop\n"   /* 80140830: 00000000 */
    "\t.word 0x4aa80428\n"   /* 80140834: 4aa80428 cop2 0xa80428 */
    "\tswc2 $25,0($5)\n"   /* 80140838: e8b90000 */
    "\tswc2 $26,4($5)\n"   /* 8014083C: e8ba0004 */
    "\tswc2 $27,8($5)\n"   /* 80140840: e8bb0008 */
    "\tjr $31\n"   /* 80140844: 03e00008 */
    "\taddu $2,$5,$0\n"   /* 80140848: 00a01021 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140824\n"
    ".size sub_80140824,.-sub_80140824\n"
    ".text\n");

/* Square0: per-component square of a VECTOR, unshifted (SQR sf=0); returns the output pointer. Sony assembly. */
__asm__(
    ".section .text_8014084C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014084C\n"
    ".type sub_8014084C,@function\n"
    ".ent sub_8014084C\n"
    "sub_8014084C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $9,0($4)\n"   /* 8014084C: c8890000 */
    "\tlwc2 $10,4($4)\n"   /* 80140850: c88a0004 */
    "\tlwc2 $11,8($4)\n"   /* 80140854: c88b0008 */
    "\tnop\n"   /* 80140858: 00000000 */
    "\t.word 0x4aa00428\n"   /* 8014085C: 4aa00428 cop2 0xa00428 */
    "\tswc2 $25,0($5)\n"   /* 80140860: e8b90000 */
    "\tswc2 $26,4($5)\n"   /* 80140864: e8ba0004 */
    "\tswc2 $27,8($5)\n"   /* 80140868: e8bb0008 */
    "\tjr $31\n"   /* 8014086C: 03e00008 */
    "\taddu $2,$5,$0\n"   /* 80140870: 00a01021 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014084C\n"
    ".size sub_8014084C,.-sub_8014084C\n"
    ".text\n");

/* AverageZ3: average of three Z values via the SZ FIFO and AVSZ3. Sony assembly. */
__asm__(
    ".section .text_80140874,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140874\n"
    ".type sub_80140874,@function\n"
    ".ent sub_80140874\n"
    "sub_80140874:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$17\n"   /* 80140874: 48848800 */
    "\tmtc2 $5,$18\n"   /* 80140878: 48859000 */
    "\tmtc2 $6,$19\n"   /* 8014087C: 48869800 */
    "\tnop\n"   /* 80140880: 00000000 */
    "\t.word 0x4b58002d\n"   /* 80140884: 4b58002d cop2 0x158002d */
    "\tmfc2 $2,$7\n"   /* 80140888: 48023800 */
    "\tjr $31\n"   /* 8014088C: 03e00008 */
    "\tnop\n"   /* 80140890: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140874\n"
    ".size sub_80140874,.-sub_80140874\n"
    ".text\n");

/* AverageZ4: average of four Z values via the SZ FIFO and AVSZ4. Sony assembly. */
__asm__(
    ".section .text_80140894,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140894\n"
    ".type sub_80140894,@function\n"
    ".ent sub_80140894\n"
    "sub_80140894:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$16\n"   /* 80140894: 48848000 */
    "\tmtc2 $5,$17\n"   /* 80140898: 48858800 */
    "\tmtc2 $6,$18\n"   /* 8014089C: 48869000 */
    "\tmtc2 $7,$19\n"   /* 801408A0: 48879800 */
    "\tnop\n"   /* 801408A4: 00000000 */
    "\t.word 0x4b68002e\n"   /* 801408A8: 4b68002e cop2 0x168002e */
    "\tmfc2 $2,$7\n"   /* 801408AC: 48023800 */
    "\tjr $31\n"   /* 801408B0: 03e00008 */
    "\tnop\n"   /* 801408B4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140894\n"
    ".size sub_80140894,.-sub_80140894\n"
    ".text\n");

/* OuterProduct0: cross product of two VECTORs (OP sf=0), restoring the rotation diagonal afterwards. Sony assembly. */
__asm__(
    ".section .text_80140910,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140910\n"
    ".type sub_80140910,@function\n"
    ".ent sub_80140910\n"
    "sub_80140910:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tcfc2 $13,$0\n"   /* 80140910: 484d0000 */
    "\tcfc2 $14,$2\n"   /* 80140914: 484e1000 */
    "\tcfc2 $15,$4\n"   /* 80140918: 484f2000 */
    "\tlw $8,0($4)\n"   /* 8014091C: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80140920: 8c890004 */
    "\tlw $10,8($4)\n"   /* 80140924: 8c8a0008 */
    "\tctc2 $8,$0\n"   /* 80140928: 48c80000 */
    "\tctc2 $9,$2\n"   /* 8014092C: 48c91000 */
    "\tctc2 $10,$4\n"   /* 80140930: 48ca2000 */
    "\tlwc2 $11,8($5)\n"   /* 80140934: c8ab0008 */
    "\tlwc2 $9,0($5)\n"   /* 80140938: c8a90000 */
    "\tlwc2 $10,4($5)\n"   /* 8014093C: c8aa0004 */
    "\tnop\n"   /* 80140940: 00000000 */
    "\t.word 0x4b70000c\n"   /* 80140944: 4b70000c cop2 0x170000c */
    "\tswc2 $25,0($6)\n"   /* 80140948: e8d90000 */
    "\tswc2 $26,4($6)\n"   /* 8014094C: e8da0004 */
    "\tswc2 $27,8($6)\n"   /* 80140950: e8db0008 */
    "\tctc2 $13,$0\n"   /* 80140954: 48cd0000 */
    "\tctc2 $14,$2\n"   /* 80140958: 48ce1000 */
    "\tctc2 $15,$4\n"   /* 8014095C: 48cf2000 */
    "\tjr $31\n"   /* 80140960: 03e00008 */
    "\tnop\n"   /* 80140964: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140910\n"
    ".size sub_80140910,.-sub_80140910\n"
    ".text\n");

/* Lzc: leading-zero/one count of a 32-bit value (GTE LZCS/LZCR). Sony assembly. */
__asm__(
    ".section .text_80140968,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80140968\n"
    ".type sub_80140968,@function\n"
    ".ent sub_80140968\n"
    "sub_80140968:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$30\n"   /* 80140968: 4884f000 */
    "\tnop\n"   /* 8014096C: 00000000 */
    "\tnop\n"   /* 80140970: 00000000 */
    "\tmfc2 $2,$31\n"   /* 80140974: 4802f800 */
    "\tjr $31\n"   /* 80140978: 03e00008 */
    "\tnop\n"   /* 8014097C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80140968\n"
    ".size sub_80140968,.-sub_80140968\n"
    ".text\n");

/* CompMatrix: composite transform m2 = m0 * m1 including translation. Sony assembly. */
__asm__(
    ".section .text_80149704,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149704\n"
    ".type sub_80149704,@function\n"
    ".ent sub_80149704\n"
    "sub_80149704:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80149704: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80149708: 8c890004 */
    "\tlw $10,8($4)\n"   /* 8014970C: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 80149710: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 80149714: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80149718: 48c80000 */
    "\tctc2 $9,$1\n"   /* 8014971C: 48c90800 */
    "\tctc2 $10,$2\n"   /* 80149720: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 80149724: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80149728: 48cc2000 */
    "\tlhu $8,0($5)\n"   /* 8014972C: 94a80000 */
    "\tlw $9,4($5)\n"   /* 80149730: 8ca90004 */
    "\tlw $10,12($5)\n"   /* 80149734: 8caa000c */
    "\tlui $1,0xffff\n"   /* 80149738: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 8014973C: 01214824 */
    "\tor $8,$8,$9\n"   /* 80149740: 01094025 */
    "\tmtc2 $8,$0\n"   /* 80149744: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149748: 488a0800 */
    "\tnop\n"   /* 8014974C: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149750: 4a486012 cop2 0x486012 */
    "\tlhu $8,2($5)\n"   /* 80149754: 94a80002 */
    "\tlw $9,8($5)\n"   /* 80149758: 8ca90008 */
    "\tlh $10,14($5)\n"   /* 8014975C: 84aa000e */
    "\tsll $9,$9,0x10\n"   /* 80149760: 00094c00 */
    "\tor $8,$8,$9\n"   /* 80149764: 01094025 */
    "\tmfc2 $11,$9\n"   /* 80149768: 480b4800 */
    "\tmfc2 $12,$10\n"   /* 8014976C: 480c5000 */
    "\tmfc2 $13,$11\n"   /* 80149770: 480d5800 */
    "\tmtc2 $8,$0\n"   /* 80149774: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149778: 488a0800 */
    "\tnop\n"   /* 8014977C: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149780: 4a486012 cop2 0x486012 */
    "\tlhu $8,4($5)\n"   /* 80149784: 94a80004 */
    "\tlw $9,8($5)\n"   /* 80149788: 8ca90008 */
    "\tlw $10,16($5)\n"   /* 8014978C: 8caa0010 */
    "\tlui $1,0xffff\n"   /* 80149790: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 80149794: 01214824 */
    "\tor $8,$8,$9\n"   /* 80149798: 01094025 */
    "\tmfc2 $14,$9\n"   /* 8014979C: 480e4800 */
    "\tmfc2 $15,$10\n"   /* 801497A0: 480f5000 */
    "\tmfc2 $24,$11\n"   /* 801497A4: 48185800 */
    "\tmtc2 $8,$0\n"   /* 801497A8: 48880000 */
    "\tmtc2 $10,$1\n"   /* 801497AC: 488a0800 */
    "\tnop\n"   /* 801497B0: 00000000 */
    "\t.word 0x4a486012\n"   /* 801497B4: 4a486012 cop2 0x486012 */
    "\tandi $11,$11,0xffff\n"   /* 801497B8: 316bffff */
    "\tsll $14,$14,0x10\n"   /* 801497BC: 000e7400 */
    "\tor $14,$14,$11\n"   /* 801497C0: 01cb7025 */
    "\tsw $14,0($6)\n"   /* 801497C4: acce0000 */
    "\tandi $13,$13,0xffff\n"   /* 801497C8: 31adffff */
    "\tsll $24,$24,0x10\n"   /* 801497CC: 0018c400 */
    "\tor $24,$24,$13\n"   /* 801497D0: 030dc025 */
    "\tsw $24,12($6)\n"   /* 801497D4: acd8000c */
    "\tmfc2 $8,$9\n"   /* 801497D8: 48084800 */
    "\tmfc2 $9,$10\n"   /* 801497DC: 48095000 */
    "\tswc2 $11,16($6)\n"   /* 801497E0: e8cb0010 */
    "\tlhu $13,20($5)\n"   /* 801497E4: 94ad0014 */
    "\tlw $14,24($5)\n"   /* 801497E8: 8cae0018 */
    "\tlw $10,28($5)\n"   /* 801497EC: 8caa001c */
    "\tsll $14,$14,0x10\n"   /* 801497F0: 000e7400 */
    "\tor $13,$13,$14\n"   /* 801497F4: 01ae6825 */
    "\tmtc2 $13,$0\n"   /* 801497F8: 488d0000 */
    "\tmtc2 $10,$1\n"   /* 801497FC: 488a0800 */
    "\tnop\n"   /* 80149800: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149804: 4a486012 cop2 0x486012 */
    "\tsll $12,$12,0x10\n"   /* 80149808: 000c6400 */
    "\tandi $8,$8,0xffff\n"   /* 8014980C: 3108ffff */
    "\tor $8,$8,$12\n"   /* 80149810: 010c4025 */
    "\tsw $8,4($6)\n"   /* 80149814: acc80004 */
    "\tandi $15,$15,0xffff\n"   /* 80149818: 31efffff */
    "\tsll $9,$9,0x10\n"   /* 8014981C: 00094c00 */
    "\tor $9,$9,$15\n"   /* 80149820: 012f4825 */
    "\tsw $9,8($6)\n"   /* 80149824: acc90008 */
    "\tmfc2 $8,$25\n"   /* 80149828: 4808c800 */
    "\tmfc2 $9,$26\n"   /* 8014982C: 4809d000 */
    "\tmfc2 $10,$27\n"   /* 80149830: 480ad800 */
    "\tlw $11,20($4)\n"   /* 80149834: 8c8b0014 */
    "\tlw $12,24($4)\n"   /* 80149838: 8c8c0018 */
    "\tlw $13,28($4)\n"   /* 8014983C: 8c8d001c */
    "\tadd $8,$8,$11\n"   /* 80149840: 010b4020 */
    "\tadd $9,$9,$12\n"   /* 80149844: 012c4820 */
    "\tadd $10,$10,$13\n"   /* 80149848: 014d5020 */
    "\tsw $8,20($6)\n"   /* 8014984C: acc80014 */
    "\tsw $9,24($6)\n"   /* 80149850: acc90018 */
    "\tsw $10,28($6)\n"   /* 80149854: acca001c */
    "\taddu $2,$6,$0\n"   /* 80149858: 00c01021 */
    "\tjr $31\n"   /* 8014985C: 03e00008 */
    "\tnop\n"   /* 80149860: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149704\n"
    ".size sub_80149704,.-sub_80149704\n"
    ".text\n");

/* MulMatrix0: m2 = m0 * m1. Sony assembly. */
__asm__(
    ".section .text_80149864,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149864\n"
    ".type sub_80149864,@function\n"
    ".ent sub_80149864\n"
    "sub_80149864:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80149864: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80149868: 8c890004 */
    "\tlw $10,8($4)\n"   /* 8014986C: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 80149870: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 80149874: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80149878: 48c80000 */
    "\tctc2 $9,$1\n"   /* 8014987C: 48c90800 */
    "\tctc2 $10,$2\n"   /* 80149880: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 80149884: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80149888: 48cc2000 */
    "\tlhu $8,0($5)\n"   /* 8014988C: 94a80000 */
    "\tlw $9,4($5)\n"   /* 80149890: 8ca90004 */
    "\tlw $10,12($5)\n"   /* 80149894: 8caa000c */
    "\tlui $1,0xffff\n"   /* 80149898: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 8014989C: 01214824 */
    "\tor $8,$8,$9\n"   /* 801498A0: 01094025 */
    "\tmtc2 $8,$0\n"   /* 801498A4: 48880000 */
    "\tmtc2 $10,$1\n"   /* 801498A8: 488a0800 */
    "\tnop\n"   /* 801498AC: 00000000 */
    "\t.word 0x4a486012\n"   /* 801498B0: 4a486012 cop2 0x486012 */
    "\tlhu $8,2($5)\n"   /* 801498B4: 94a80002 */
    "\tlw $9,8($5)\n"   /* 801498B8: 8ca90008 */
    "\tlh $10,14($5)\n"   /* 801498BC: 84aa000e */
    "\tsll $9,$9,0x10\n"   /* 801498C0: 00094c00 */
    "\tor $8,$8,$9\n"   /* 801498C4: 01094025 */
    "\tmfc2 $11,$9\n"   /* 801498C8: 480b4800 */
    "\tmfc2 $12,$10\n"   /* 801498CC: 480c5000 */
    "\tmfc2 $13,$11\n"   /* 801498D0: 480d5800 */
    "\tmtc2 $8,$0\n"   /* 801498D4: 48880000 */
    "\tmtc2 $10,$1\n"   /* 801498D8: 488a0800 */
    "\tnop\n"   /* 801498DC: 00000000 */
    "\t.word 0x4a486012\n"   /* 801498E0: 4a486012 cop2 0x486012 */
    "\tlhu $8,4($5)\n"   /* 801498E4: 94a80004 */
    "\tlw $9,8($5)\n"   /* 801498E8: 8ca90008 */
    "\tlw $10,16($5)\n"   /* 801498EC: 8caa0010 */
    "\tlui $1,0xffff\n"   /* 801498F0: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 801498F4: 01214824 */
    "\tor $8,$8,$9\n"   /* 801498F8: 01094025 */
    "\tmfc2 $14,$9\n"   /* 801498FC: 480e4800 */
    "\tmfc2 $15,$10\n"   /* 80149900: 480f5000 */
    "\tmfc2 $24,$11\n"   /* 80149904: 48185800 */
    "\tmtc2 $8,$0\n"   /* 80149908: 48880000 */
    "\tmtc2 $10,$1\n"   /* 8014990C: 488a0800 */
    "\tnop\n"   /* 80149910: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149914: 4a486012 cop2 0x486012 */
    "\tandi $11,$11,0xffff\n"   /* 80149918: 316bffff */
    "\tsll $14,$14,0x10\n"   /* 8014991C: 000e7400 */
    "\tor $14,$14,$11\n"   /* 80149920: 01cb7025 */
    "\tsw $14,0($6)\n"   /* 80149924: acce0000 */
    "\tandi $13,$13,0xffff\n"   /* 80149928: 31adffff */
    "\tsll $24,$24,0x10\n"   /* 8014992C: 0018c400 */
    "\tor $24,$24,$13\n"   /* 80149930: 030dc025 */
    "\tsw $24,12($6)\n"   /* 80149934: acd8000c */
    "\tmfc2 $8,$9\n"   /* 80149938: 48084800 */
    "\tmfc2 $9,$10\n"   /* 8014993C: 48095000 */
    "\tandi $8,$8,0xffff\n"   /* 80149940: 3108ffff */
    "\tsll $12,$12,0x10\n"   /* 80149944: 000c6400 */
    "\tor $8,$8,$12\n"   /* 80149948: 010c4025 */
    "\tsw $8,4($6)\n"   /* 8014994C: acc80004 */
    "\tandi $15,$15,0xffff\n"   /* 80149950: 31efffff */
    "\tsll $9,$9,0x10\n"   /* 80149954: 00094c00 */
    "\tor $9,$9,$15\n"   /* 80149958: 012f4825 */
    "\tsw $9,8($6)\n"   /* 8014995C: acc90008 */
    "\tswc2 $11,16($6)\n"   /* 80149960: e8cb0010 */
    "\taddu $2,$6,$0\n"   /* 80149964: 00c01021 */
    "\tjr $31\n"   /* 80149968: 03e00008 */
    "\tnop\n"   /* 8014996C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149864\n"
    ".size sub_80149864,.-sub_80149864\n"
    ".text\n");

/* MulRotMatrix: m0 = GTE rotation matrix * m0. Sony assembly. */
__asm__(
    ".section .text_80149A54,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149A54\n"
    ".type sub_80149A54,@function\n"
    ".ent sub_80149A54\n"
    "sub_80149A54:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80149A54: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80149A58: 8c890004 */
    "\tlw $10,12($4)\n"   /* 80149A5C: 8c8a000c */
    "\tandi $8,$8,0xffff\n"   /* 80149A60: 3108ffff */
    "\tlui $1,0xffff\n"   /* 80149A64: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 80149A68: 01214824 */
    "\tor $8,$8,$9\n"   /* 80149A6C: 01094025 */
    "\tmtc2 $8,$0\n"   /* 80149A70: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149A74: 488a0800 */
    "\tnop\n"   /* 80149A78: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149A7C: 4a486012 cop2 0x486012 */
    "\tlhu $8,2($4)\n"   /* 80149A80: 94880002 */
    "\tlw $9,8($4)\n"   /* 80149A84: 8c890008 */
    "\tlh $10,14($4)\n"   /* 80149A88: 848a000e */
    "\tsll $9,$9,0x10\n"   /* 80149A8C: 00094c00 */
    "\tor $8,$8,$9\n"   /* 80149A90: 01094025 */
    "\tmfc2 $11,$9\n"   /* 80149A94: 480b4800 */
    "\tmfc2 $12,$10\n"   /* 80149A98: 480c5000 */
    "\tmfc2 $13,$11\n"   /* 80149A9C: 480d5800 */
    "\tmtc2 $8,$0\n"   /* 80149AA0: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149AA4: 488a0800 */
    "\tnop\n"   /* 80149AA8: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149AAC: 4a486012 cop2 0x486012 */
    "\tlhu $8,4($4)\n"   /* 80149AB0: 94880004 */
    "\tlw $9,8($4)\n"   /* 80149AB4: 8c890008 */
    "\tlw $10,16($4)\n"   /* 80149AB8: 8c8a0010 */
    "\tlui $1,0xffff\n"   /* 80149ABC: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 80149AC0: 01214824 */
    "\tor $8,$8,$9\n"   /* 80149AC4: 01094025 */
    "\tmfc2 $14,$9\n"   /* 80149AC8: 480e4800 */
    "\tmfc2 $15,$10\n"   /* 80149ACC: 480f5000 */
    "\tmfc2 $24,$11\n"   /* 80149AD0: 48185800 */
    "\tmtc2 $8,$0\n"   /* 80149AD4: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149AD8: 488a0800 */
    "\tnop\n"   /* 80149ADC: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149AE0: 4a486012 cop2 0x486012 */
    "\tandi $11,$11,0xffff\n"   /* 80149AE4: 316bffff */
    "\tsll $14,$14,0x10\n"   /* 80149AE8: 000e7400 */
    "\tor $14,$14,$11\n"   /* 80149AEC: 01cb7025 */
    "\tsw $14,0($4)\n"   /* 80149AF0: ac8e0000 */
    "\tandi $13,$13,0xffff\n"   /* 80149AF4: 31adffff */
    "\tsll $24,$24,0x10\n"   /* 80149AF8: 0018c400 */
    "\tor $24,$24,$13\n"   /* 80149AFC: 030dc025 */
    "\tsw $24,12($4)\n"   /* 80149B00: ac98000c */
    "\tmfc2 $8,$9\n"   /* 80149B04: 48084800 */
    "\tmfc2 $9,$10\n"   /* 80149B08: 48095000 */
    "\tandi $8,$8,0xffff\n"   /* 80149B0C: 3108ffff */
    "\tsll $12,$12,0x10\n"   /* 80149B10: 000c6400 */
    "\tor $8,$8,$12\n"   /* 80149B14: 010c4025 */
    "\tsw $8,4($4)\n"   /* 80149B18: ac880004 */
    "\tandi $15,$15,0xffff\n"   /* 80149B1C: 31efffff */
    "\tsll $9,$9,0x10\n"   /* 80149B20: 00094c00 */
    "\tor $9,$9,$15\n"   /* 80149B24: 012f4825 */
    "\tsw $9,8($4)\n"   /* 80149B28: ac890008 */
    "\tswc2 $11,16($4)\n"   /* 80149B2C: e88b0010 */
    "\taddu $2,$4,$0\n"   /* 80149B30: 00801021 */
    "\tjr $31\n"   /* 80149B34: 03e00008 */
    "\tnop\n"   /* 80149B38: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149A54\n"
    ".size sub_80149A54,.-sub_80149A54\n"
    ".text\n");

/* SetMulMatrix: load the GTE rotation matrix with m0 * m1. Sony assembly. */
__asm__(
    ".section .text_80149B3C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149B3C\n"
    ".type sub_80149B3C,@function\n"
    ".ent sub_80149B3C\n"
    "sub_80149B3C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80149B3C: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80149B40: 8c890004 */
    "\tlw $10,8($4)\n"   /* 80149B44: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 80149B48: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 80149B4C: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80149B50: 48c80000 */
    "\tctc2 $9,$1\n"   /* 80149B54: 48c90800 */
    "\tctc2 $10,$2\n"   /* 80149B58: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 80149B5C: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80149B60: 48cc2000 */
    "\tlhu $8,0($5)\n"   /* 80149B64: 94a80000 */
    "\tlw $9,4($5)\n"   /* 80149B68: 8ca90004 */
    "\tlw $10,12($5)\n"   /* 80149B6C: 8caa000c */
    "\tlui $1,0xffff\n"   /* 80149B70: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 80149B74: 01214824 */
    "\tor $8,$8,$9\n"   /* 80149B78: 01094025 */
    "\tmtc2 $8,$0\n"   /* 80149B7C: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149B80: 488a0800 */
    "\tnop\n"   /* 80149B84: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149B88: 4a486012 cop2 0x486012 */
    "\tlhu $8,2($5)\n"   /* 80149B8C: 94a80002 */
    "\tlw $9,8($5)\n"   /* 80149B90: 8ca90008 */
    "\tlh $10,14($5)\n"   /* 80149B94: 84aa000e */
    "\tsll $9,$9,0x10\n"   /* 80149B98: 00094c00 */
    "\tor $8,$8,$9\n"   /* 80149B9C: 01094025 */
    "\tmfc2 $11,$9\n"   /* 80149BA0: 480b4800 */
    "\tmfc2 $12,$10\n"   /* 80149BA4: 480c5000 */
    "\tmfc2 $13,$11\n"   /* 80149BA8: 480d5800 */
    "\tmtc2 $8,$0\n"   /* 80149BAC: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149BB0: 488a0800 */
    "\tnop\n"   /* 80149BB4: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149BB8: 4a486012 cop2 0x486012 */
    "\tlhu $8,4($5)\n"   /* 80149BBC: 94a80004 */
    "\tlw $9,8($5)\n"   /* 80149BC0: 8ca90008 */
    "\tlw $10,16($5)\n"   /* 80149BC4: 8caa0010 */
    "\tlui $1,0xffff\n"   /* 80149BC8: 3c01ffff */
    "\tand $9,$9,$1\n"   /* 80149BCC: 01214824 */
    "\tor $8,$8,$9\n"   /* 80149BD0: 01094025 */
    "\tmfc2 $14,$9\n"   /* 80149BD4: 480e4800 */
    "\tmfc2 $15,$10\n"   /* 80149BD8: 480f5000 */
    "\tmfc2 $24,$11\n"   /* 80149BDC: 48185800 */
    "\tmtc2 $8,$0\n"   /* 80149BE0: 48880000 */
    "\tmtc2 $10,$1\n"   /* 80149BE4: 488a0800 */
    "\tnop\n"   /* 80149BE8: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149BEC: 4a486012 cop2 0x486012 */
    "\tandi $11,$11,0xffff\n"   /* 80149BF0: 316bffff */
    "\tsll $14,$14,0x10\n"   /* 80149BF4: 000e7400 */
    "\tor $14,$14,$11\n"   /* 80149BF8: 01cb7025 */
    "\tandi $13,$13,0xffff\n"   /* 80149BFC: 31adffff */
    "\tsll $24,$24,0x10\n"   /* 80149C00: 0018c400 */
    "\tor $24,$24,$13\n"   /* 80149C04: 030dc025 */
    "\tmfc2 $8,$9\n"   /* 80149C08: 48084800 */
    "\tmfc2 $9,$10\n"   /* 80149C0C: 48095000 */
    "\tmfc2 $10,$11\n"   /* 80149C10: 480a5800 */
    "\tandi $8,$8,0xffff\n"   /* 80149C14: 3108ffff */
    "\tsll $12,$12,0x10\n"   /* 80149C18: 000c6400 */
    "\tor $8,$8,$12\n"   /* 80149C1C: 010c4025 */
    "\tandi $15,$15,0xffff\n"   /* 80149C20: 31efffff */
    "\tsll $9,$9,0x10\n"   /* 80149C24: 00094c00 */
    "\tor $9,$9,$15\n"   /* 80149C28: 012f4825 */
    "\tctc2 $14,$0\n"   /* 80149C2C: 48ce0000 */
    "\tctc2 $8,$1\n"   /* 80149C30: 48c80800 */
    "\tctc2 $9,$2\n"   /* 80149C34: 48c91000 */
    "\tctc2 $24,$3\n"   /* 80149C38: 48d81800 */
    "\tctc2 $10,$4\n"   /* 80149C3C: 48ca2000 */
    "\taddu $2,$4,$0\n"   /* 80149C40: 00801021 */
    "\tjr $31\n"   /* 80149C44: 03e00008 */
    "\tnop\n"   /* 80149C48: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149B3C\n"
    ".size sub_80149B3C,.-sub_80149B3C\n"
    ".text\n");

/* ApplyMatrixLV: long VECTOR = m * long VECTOR (split into high and low halves). Sony assembly. */
__asm__(
    ".section .text_80149C4C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149C4C\n"
    ".type sub_80149C4C,@function\n"
    ".ent sub_80149C4C\n"
    "sub_80149C4C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80149C4C: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80149C50: 8c890004 */
    "\tlw $10,8($4)\n"   /* 80149C54: 8c8a0008 */
    "\tlw $11,12($4)\n"   /* 80149C58: 8c8b000c */
    "\tlw $12,16($4)\n"   /* 80149C5C: 8c8c0010 */
    "\tctc2 $8,$0\n"   /* 80149C60: 48c80000 */
    "\tctc2 $9,$1\n"   /* 80149C64: 48c90800 */
    "\tctc2 $10,$2\n"   /* 80149C68: 48ca1000 */
    "\tctc2 $11,$3\n"   /* 80149C6C: 48cb1800 */
    "\tctc2 $12,$4\n"   /* 80149C70: 48cc2000 */
    "\tlw $8,0($5)\n"   /* 80149C74: 8ca80000 */
    "\tlw $9,4($5)\n"   /* 80149C78: 8ca90004 */
    "\tlw $10,8($5)\n"   /* 80149C7C: 8caa0008 */
    "\tbgez $8,.L80149C4C_80149CA4\n"   /* 80149C80: 05010008 */
    "\tsra $11,$8,0xf\n"   /* 80149C84: 00085bc3 */
    "\tnegu $8,$8\n"   /* 80149C88: 00084023 */
    "\tsra $11,$8,0xf\n"   /* 80149C8C: 00085bc3 */
    "\tandi $8,$8,0x7fff\n"   /* 80149C90: 31087fff */
    "\tnegu $11,$11\n"   /* 80149C94: 000b5823 */
    "\tbeq $0,$0,.L80149C4C_80149CA8\n"   /* 80149C98: 10000003 */
    "\tnegu $8,$8\n"   /* 80149C9C: 00084023 */
    "\tsra $11,$8,0xf\n"   /* 80149CA0: 00085bc3 */
    ".L80149C4C_80149CA4:\n"
    "\tandi $8,$8,0x7fff\n"   /* 80149CA4: 31087fff */
    ".L80149C4C_80149CA8:\n"
    "\tbgez $9,.L80149C4C_80149CCC\n"   /* 80149CA8: 05210008 */
    "\tsra $12,$9,0xf\n"   /* 80149CAC: 000963c3 */
    "\tnegu $9,$9\n"   /* 80149CB0: 00094823 */
    "\tsra $12,$9,0xf\n"   /* 80149CB4: 000963c3 */
    "\tandi $9,$9,0x7fff\n"   /* 80149CB8: 31297fff */
    "\tnegu $12,$12\n"   /* 80149CBC: 000c6023 */
    "\tbeq $0,$0,.L80149C4C_80149CD0\n"   /* 80149CC0: 10000003 */
    "\tnegu $9,$9\n"   /* 80149CC4: 00094823 */
    "\tsra $12,$9,0xf\n"   /* 80149CC8: 000963c3 */
    ".L80149C4C_80149CCC:\n"
    "\tandi $9,$9,0x7fff\n"   /* 80149CCC: 31297fff */
    ".L80149C4C_80149CD0:\n"
    "\tbgez $10,.L80149C4C_80149CF4\n"   /* 80149CD0: 05410008 */
    "\tsra $13,$10,0xf\n"   /* 80149CD4: 000a6bc3 */
    "\tnegu $10,$10\n"   /* 80149CD8: 000a5023 */
    "\tsra $13,$10,0xf\n"   /* 80149CDC: 000a6bc3 */
    "\tandi $10,$10,0x7fff\n"   /* 80149CE0: 314a7fff */
    "\tnegu $13,$13\n"   /* 80149CE4: 000d6823 */
    "\tbeq $0,$0,.L80149C4C_80149CF8\n"   /* 80149CE8: 10000003 */
    "\tnegu $10,$10\n"   /* 80149CEC: 000a5023 */
    "\tsra $13,$10,0xf\n"   /* 80149CF0: 000a6bc3 */
    ".L80149C4C_80149CF4:\n"
    "\tandi $10,$10,0x7fff\n"   /* 80149CF4: 314a7fff */
    ".L80149C4C_80149CF8:\n"
    "\tmtc2 $11,$9\n"   /* 80149CF8: 488b4800 */
    "\tmtc2 $12,$10\n"   /* 80149CFC: 488c5000 */
    "\tmtc2 $13,$11\n"   /* 80149D00: 488d5800 */
    "\tnop\n"   /* 80149D04: 00000000 */
    "\t.word 0x4a41e012\n"   /* 80149D08: 4a41e012 cop2 0x41e012 */
    "\tmfc2 $11,$25\n"   /* 80149D0C: 480bc800 */
    "\tmfc2 $12,$26\n"   /* 80149D10: 480cd000 */
    "\tmfc2 $13,$27\n"   /* 80149D14: 480dd800 */
    "\tmtc2 $8,$9\n"   /* 80149D18: 48884800 */
    "\tmtc2 $9,$10\n"   /* 80149D1C: 48895000 */
    "\tmtc2 $10,$11\n"   /* 80149D20: 488a5800 */
    "\tnop\n"   /* 80149D24: 00000000 */
    "\t.word 0x4a49e012\n"   /* 80149D28: 4a49e012 cop2 0x49e012 */
    "\tbgez $11,.L80149C4C_80149D44\n"   /* 80149D2C: 05610005 */
    "\tnop\n"   /* 80149D30: 00000000 */
    "\tnegu $11,$11\n"   /* 80149D34: 000b5823 */
    "\tsll $11,$11,0x3\n"   /* 80149D38: 000b58c0 */
    "\tbeq $0,$0,.L80149C4C_80149D48\n"   /* 80149D3C: 10000002 */
    "\tnegu $11,$11\n"   /* 80149D40: 000b5823 */
    ".L80149C4C_80149D44:\n"
    "\tsll $11,$11,0x3\n"   /* 80149D44: 000b58c0 */
    ".L80149C4C_80149D48:\n"
    "\tbgez $12,.L80149C4C_80149D60\n"   /* 80149D48: 05810005 */
    "\tnop\n"   /* 80149D4C: 00000000 */
    "\tnegu $12,$12\n"   /* 80149D50: 000c6023 */
    "\tsll $12,$12,0x3\n"   /* 80149D54: 000c60c0 */
    "\tbeq $0,$0,.L80149C4C_80149D64\n"   /* 80149D58: 10000002 */
    "\tnegu $12,$12\n"   /* 80149D5C: 000c6023 */
    ".L80149C4C_80149D60:\n"
    "\tsll $12,$12,0x3\n"   /* 80149D60: 000c60c0 */
    ".L80149C4C_80149D64:\n"
    "\tbgez $13,.L80149C4C_80149D7C\n"   /* 80149D64: 05a10005 */
    "\tnop\n"   /* 80149D68: 00000000 */
    "\tnegu $13,$13\n"   /* 80149D6C: 000d6823 */
    "\tsll $13,$13,0x3\n"   /* 80149D70: 000d68c0 */
    "\tbeq $0,$0,.L80149C4C_80149D80\n"   /* 80149D74: 10000002 */
    "\tnegu $13,$13\n"   /* 80149D78: 000d6823 */
    ".L80149C4C_80149D7C:\n"
    "\tsll $13,$13,0x3\n"   /* 80149D7C: 000d68c0 */
    ".L80149C4C_80149D80:\n"
    "\tmfc2 $8,$25\n"   /* 80149D80: 4808c800 */
    "\tmfc2 $9,$26\n"   /* 80149D84: 4809d000 */
    "\tmfc2 $10,$27\n"   /* 80149D88: 480ad800 */
    "\taddu $8,$8,$11\n"   /* 80149D8C: 010b4021 */
    "\taddu $9,$9,$12\n"   /* 80149D90: 012c4821 */
    "\taddu $10,$10,$13\n"   /* 80149D94: 014d5021 */
    "\tsw $8,0($6)\n"   /* 80149D98: acc80000 */
    "\tsw $9,4($6)\n"   /* 80149D9C: acc90004 */
    "\tsw $10,8($6)\n"   /* 80149DA0: acca0008 */
    "\tjr $31\n"   /* 80149DA4: 03e00008 */
    "\taddu $2,$6,$0\n"   /* 80149DA8: 00c01021 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149C4C\n"
    ".size sub_80149C4C,.-sub_80149C4C\n"
    ".text\n");

/* ApplyRotMatrix: long VECTOR = GTE rotation matrix * SVECTOR; returns the output pointer. Sony assembly. */
__asm__(
    ".section .text_80149DAC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149DAC\n"
    ".type sub_80149DAC,@function\n"
    ".ent sub_80149DAC\n"
    "sub_80149DAC:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80149DAC: 8c880000 */
    "\tlw $9,4($4)\n"   /* 80149DB0: 8c890004 */
    "\tmtc2 $8,$0\n"   /* 80149DB4: 48880000 */
    "\tmtc2 $9,$1\n"   /* 80149DB8: 48890800 */
    "\tnop\n"   /* 80149DBC: 00000000 */
    "\t.word 0x4a486012\n"   /* 80149DC0: 4a486012 cop2 0x486012 */
    "\tswc2 $9,0($5)\n"   /* 80149DC4: e8a90000 */
    "\tswc2 $10,4($5)\n"   /* 80149DC8: e8aa0004 */
    "\tswc2 $11,8($5)\n"   /* 80149DCC: e8ab0008 */
    "\taddu $2,$6,$0\n"   /* 80149DD0: 00c01021 */
    "\tjr $31\n"   /* 80149DD4: 03e00008 */
    "\tnop\n"   /* 80149DD8: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149DAC\n"
    ".size sub_80149DAC,.-sub_80149DAC\n"
    ".text\n");

/* ScaleMatrixL: scale m by a 4.12 VECTOR (other axis than ScaleMatrix). Sony assembly. */
__asm__(
    ".section .text_80149DDC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149DDC\n"
    ".type sub_80149DDC,@function\n"
    ".ent sub_80149DDC\n"
    "sub_80149DDC:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlw $8,0($4)\n"   /* 80149DDC: 8c880000 */
    "\tlw $11,0($5)\n"   /* 80149DE0: 8cab0000 */
    "\tandi $9,$8,0xffff\n"   /* 80149DE4: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 80149DE8: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 80149DEC: 00094c03 */
    "\tmultu $9,$11\n"   /* 80149DF0: 012b0019 */
    "\tsra $10,$8,0x10\n"   /* 80149DF4: 00085403 */
    "\tlw $12,4($5)\n"   /* 80149DF8: 8cac0004 */
    "\tlw $13,8($5)\n"   /* 80149DFC: 8cad0008 */
    "\tlw $8,4($4)\n"   /* 80149E00: 8c880004 */
    "\taddu $2,$4,$0\n"   /* 80149E04: 00801021 */
    "\tmflo $9\n"   /* 80149E08: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80149E0C: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 80149E10: 3129ffff */
    "\tmultu $10,$11\n"   /* 80149E14: 014b0019 */
    "\tmflo $10\n"   /* 80149E18: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 80149E1C: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 80149E20: 000a5400 */
    "\tor $9,$9,$10\n"   /* 80149E24: 012a4825 */
    "\tsw $9,0($4)\n"   /* 80149E28: ac890000 */
    "\tandi $9,$8,0xffff\n"   /* 80149E2C: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 80149E30: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 80149E34: 00094c03 */
    "\tmultu $9,$11\n"   /* 80149E38: 012b0019 */
    "\tsra $10,$8,0x10\n"   /* 80149E3C: 00085403 */
    "\tlw $8,8($4)\n"   /* 80149E40: 8c880008 */
    "\tmflo $9\n"   /* 80149E44: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80149E48: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 80149E4C: 3129ffff */
    "\tmultu $10,$12\n"   /* 80149E50: 014c0019 */
    "\tmflo $10\n"   /* 80149E54: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 80149E58: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 80149E5C: 000a5400 */
    "\tor $9,$9,$10\n"   /* 80149E60: 012a4825 */
    "\tsw $9,4($4)\n"   /* 80149E64: ac890004 */
    "\tandi $9,$8,0xffff\n"   /* 80149E68: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 80149E6C: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 80149E70: 00094c03 */
    "\tmultu $9,$12\n"   /* 80149E74: 012c0019 */
    "\tsra $10,$8,0x10\n"   /* 80149E78: 00085403 */
    "\tlw $8,12($4)\n"   /* 80149E7C: 8c88000c */
    "\tmflo $9\n"   /* 80149E80: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80149E84: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 80149E88: 3129ffff */
    "\tmultu $10,$12\n"   /* 80149E8C: 014c0019 */
    "\tmflo $10\n"   /* 80149E90: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 80149E94: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 80149E98: 000a5400 */
    "\tor $9,$9,$10\n"   /* 80149E9C: 012a4825 */
    "\tsw $9,8($4)\n"   /* 80149EA0: ac890008 */
    "\tandi $9,$8,0xffff\n"   /* 80149EA4: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 80149EA8: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 80149EAC: 00094c03 */
    "\tmultu $9,$13\n"   /* 80149EB0: 012d0019 */
    "\tsra $10,$8,0x10\n"   /* 80149EB4: 00085403 */
    "\tlw $8,16($4)\n"   /* 80149EB8: 8c880010 */
    "\tmflo $9\n"   /* 80149EBC: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80149EC0: 00094b03 */
    "\tandi $9,$9,0xffff\n"   /* 80149EC4: 3129ffff */
    "\tmultu $10,$13\n"   /* 80149EC8: 014d0019 */
    "\tmflo $10\n"   /* 80149ECC: 00005012 */
    "\tsra $10,$10,0xc\n"   /* 80149ED0: 000a5303 */
    "\tsll $10,$10,0x10\n"   /* 80149ED4: 000a5400 */
    "\tor $9,$9,$10\n"   /* 80149ED8: 012a4825 */
    "\tsw $9,12($4)\n"   /* 80149EDC: ac89000c */
    "\tandi $9,$8,0xffff\n"   /* 80149EE0: 3109ffff */
    "\tsll $9,$9,0x10\n"   /* 80149EE4: 00094c00 */
    "\tsra $9,$9,0x10\n"   /* 80149EE8: 00094c03 */
    "\tmultu $9,$13\n"   /* 80149EEC: 012d0019 */
    "\tmflo $9\n"   /* 80149EF0: 00004812 */
    "\tsra $9,$9,0xc\n"   /* 80149EF4: 00094b03 */
    "\tjr $31\n"   /* 80149EF8: 03e00008 */
    "\tsw $9,16($4)\n"   /* 80149EFC: ac890010 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149DDC\n"
    ".size sub_80149DDC,.-sub_80149DDC\n"
    ".text\n");

/* PushMatrix: push the GTE rotation matrix and translation onto the library matrix stack (prints an error when 20 deep). Sony assembly. */
__asm__(
    ".section .text_80149F00,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149F00\n"
    ".type sub_80149F00,@function\n"
    ".ent sub_80149F00\n"
    "sub_80149F00:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlui $14,%hi(D_8017E8C8)\n"   /* 80149F00: 3c0e8018 */
    "\tlw $14,%lo(D_8017E8C8)($14)\n"   /* 80149F04: 8dcee8c8 */
    "\tnop\n"   /* 80149F08: 00000000 */
    "\tslti $1,$14,640\n"   /* 80149F0C: 29c10280 */
    "\tbne $1,$0,.L80149F00_80149F3C\n"   /* 80149F10: 1420000a */
    "\tlui $1,%hi(D_8017E8BC)\n"   /* 80149F14: 3c018018 */
    "\tsw $31,%lo(D_8017E8BC)($1)\n"   /* 80149F18: ac3fe8bc */
    "\tlui $4,%hi(D_8017EB4C)\n"   /* 80149F1C: 3c048018 */
    "\tjal sub_801388A8\n"   /* 80149F20: 0c04e22a */
    "\taddiu $4,$4,%lo(D_8017EB4C)\n"   /* 80149F24: 2484eb4c */
    "\tlui $31,%hi(D_8017E8BC)\n"   /* 80149F28: 3c1f8018 */
    "\tlw $31,%lo(D_8017E8BC)($31)\n"   /* 80149F2C: 8fffe8bc */
    "\tnop\n"   /* 80149F30: 00000000 */
    "\tjr $31\n"   /* 80149F34: 03e00008 */
    "\tnop\n"   /* 80149F38: 00000000 */
    ".L80149F00_80149F3C:\n"
    "\tlui $15,%hi(D_8017E8CC)\n"   /* 80149F3C: 3c0f8018 */
    "\taddu $15,$15,$14\n"   /* 80149F40: 01ee7821 */
    "\taddiu $15,$15,%lo(D_8017E8CC)\n"   /* 80149F44: 25efe8cc */
    "\tcfc2 $8,$0\n"   /* 80149F48: 48480000 */
    "\tcfc2 $9,$1\n"   /* 80149F4C: 48490800 */
    "\tsw $8,0($15)\n"   /* 80149F50: ade80000 */
    "\tsw $9,4($15)\n"   /* 80149F54: ade90004 */
    "\tcfc2 $8,$2\n"   /* 80149F58: 48481000 */
    "\tcfc2 $9,$3\n"   /* 80149F5C: 48491800 */
    "\tsw $8,8($15)\n"   /* 80149F60: ade80008 */
    "\tsw $9,12($15)\n"   /* 80149F64: ade9000c */
    "\tcfc2 $8,$4\n"   /* 80149F68: 48482000 */
    "\tnop\n"   /* 80149F6C: 00000000 */
    "\tsw $8,16($15)\n"   /* 80149F70: ade80010 */
    "\tcfc2 $8,$5\n"   /* 80149F74: 48482800 */
    "\tcfc2 $9,$6\n"   /* 80149F78: 48493000 */
    "\tcfc2 $10,$7\n"   /* 80149F7C: 484a3800 */
    "\tsw $8,20($15)\n"   /* 80149F80: ade80014 */
    "\tsw $9,24($15)\n"   /* 80149F84: ade90018 */
    "\tsw $10,28($15)\n"   /* 80149F88: adea001c */
    "\taddi $14,$14,32\n"   /* 80149F8C: 21ce0020 */
    "\tlui $1,%hi(D_8017E8C8)\n"   /* 80149F90: 3c018018 */
    "\tsw $14,%lo(D_8017E8C8)($1)\n"   /* 80149F94: ac2ee8c8 */
    "\tjr $31\n"   /* 80149F98: 03e00008 */
    "\tnop\n"   /* 80149F9C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149F00\n"
    ".size sub_80149F00,.-sub_80149F00\n"
    ".text\n");

/* PopMatrix: pop the GTE rotation matrix and translation from the library matrix stack (prints an error when empty). Sony assembly. */
__asm__(
    ".section .text_80149FA0,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80149FA0\n"
    ".type sub_80149FA0,@function\n"
    ".ent sub_80149FA0\n"
    "sub_80149FA0:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlui $14,%hi(D_8017E8C8)\n"   /* 80149FA0: 3c0e8018 */
    "\tlw $14,%lo(D_8017E8C8)($14)\n"   /* 80149FA4: 8dcee8c8 */
    "\tnop\n"   /* 80149FA8: 00000000 */
    "\tbgtz $14,.L80149FA0_80149FD8\n"   /* 80149FAC: 1dc0000a */
    "\tlui $1,%hi(D_8017E8BC)\n"   /* 80149FB0: 3c018018 */
    "\tsw $31,%lo(D_8017E8BC)($1)\n"   /* 80149FB4: ac3fe8bc */
    "\tlui $4,%hi(D_8017EB7D)\n"   /* 80149FB8: 3c048018 */
    "\tjal sub_801388A8\n"   /* 80149FBC: 0c04e22a */
    "\taddiu $4,$4,%lo(D_8017EB7D)\n"   /* 80149FC0: 2484eb7d */
    "\tlui $31,%hi(D_8017E8BC)\n"   /* 80149FC4: 3c1f8018 */
    "\tlw $31,%lo(D_8017E8BC)($31)\n"   /* 80149FC8: 8fffe8bc */
    "\tnop\n"   /* 80149FCC: 00000000 */
    "\tjr $31\n"   /* 80149FD0: 03e00008 */
    "\tnop\n"   /* 80149FD4: 00000000 */
    ".L80149FA0_80149FD8:\n"
    "\taddi $14,$14,-32\n"   /* 80149FD8: 21ceffe0 */
    "\tlui $1,%hi(D_8017E8C8)\n"   /* 80149FDC: 3c018018 */
    "\tsw $14,%lo(D_8017E8C8)($1)\n"   /* 80149FE0: ac2ee8c8 */
    "\tlui $15,%hi(D_8017E8CC)\n"   /* 80149FE4: 3c0f8018 */
    "\taddu $15,$15,$14\n"   /* 80149FE8: 01ee7821 */
    "\taddiu $15,$15,%lo(D_8017E8CC)\n"   /* 80149FEC: 25efe8cc */
    "\tlw $8,0($15)\n"   /* 80149FF0: 8de80000 */
    "\tlw $9,4($15)\n"   /* 80149FF4: 8de90004 */
    "\tctc2 $8,$0\n"   /* 80149FF8: 48c80000 */
    "\tctc2 $9,$1\n"   /* 80149FFC: 48c90800 */
    "\tlw $8,8($15)\n"   /* 8014A000: 8de80008 */
    "\tlw $9,12($15)\n"   /* 8014A004: 8de9000c */
    "\tctc2 $8,$2\n"   /* 8014A008: 48c81000 */
    "\tctc2 $9,$3\n"   /* 8014A00C: 48c91800 */
    "\tlw $8,16($15)\n"   /* 8014A010: 8de80010 */
    "\tnop\n"   /* 8014A014: 00000000 */
    "\tctc2 $8,$4\n"   /* 8014A018: 48c82000 */
    "\tnop\n"   /* 8014A01C: 00000000 */
    "\tlw $8,20($15)\n"   /* 8014A020: 8de80014 */
    "\tlw $9,24($15)\n"   /* 8014A024: 8de90018 */
    "\tlw $10,28($15)\n"   /* 8014A028: 8dea001c */
    "\tctc2 $8,$5\n"   /* 8014A02C: 48c82800 */
    "\tctc2 $9,$6\n"   /* 8014A030: 48c93000 */
    "\tctc2 $10,$7\n"   /* 8014A034: 48ca3800 */
    "\tjr $31\n"   /* 8014A038: 03e00008 */
    "\tnop\n"   /* 8014A03C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80149FA0\n"
    ".size sub_80149FA0,.-sub_80149FA0\n"
    ".text\n");

/* ReadRotMatrix: read the GTE rotation matrix and translation into m. Sony assembly. */
__asm__(
    ".section .text_8014A040,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A040\n"
    ".type sub_8014A040,@function\n"
    ".ent sub_8014A040\n"
    "sub_8014A040:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tcfc2 $8,$0\n"   /* 8014A040: 48480000 */
    "\tcfc2 $9,$1\n"   /* 8014A044: 48490800 */
    "\tcfc2 $10,$2\n"   /* 8014A048: 484a1000 */
    "\tcfc2 $11,$3\n"   /* 8014A04C: 484b1800 */
    "\tcfc2 $12,$4\n"   /* 8014A050: 484c2000 */
    "\tsw $8,0($4)\n"   /* 8014A054: ac880000 */
    "\tsw $9,4($4)\n"   /* 8014A058: ac890004 */
    "\tsw $10,8($4)\n"   /* 8014A05C: ac8a0008 */
    "\tsw $11,12($4)\n"   /* 8014A060: ac8b000c */
    "\tsw $12,16($4)\n"   /* 8014A064: ac8c0010 */
    "\tcfc2 $8,$5\n"   /* 8014A068: 48482800 */
    "\tcfc2 $9,$6\n"   /* 8014A06C: 48493000 */
    "\tcfc2 $10,$7\n"   /* 8014A070: 484a3800 */
    "\tsw $8,20($4)\n"   /* 8014A074: ac880014 */
    "\tsw $9,24($4)\n"   /* 8014A078: ac890018 */
    "\tsw $10,28($4)\n"   /* 8014A07C: ac8a001c */
    "\tjr $31\n"   /* 8014A080: 03e00008 */
    "\tnop\n"   /* 8014A084: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A040\n"
    ".size sub_8014A040,.-sub_8014A040\n"
    ".text\n");

/* ReadLightMatrix: read the GTE light matrix and back color into m. Sony assembly. */
__asm__(
    ".section .text_8014A088,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A088\n"
    ".type sub_8014A088,@function\n"
    ".ent sub_8014A088\n"
    "sub_8014A088:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tcfc2 $8,$8\n"   /* 8014A088: 48484000 */
    "\tcfc2 $9,$9\n"   /* 8014A08C: 48494800 */
    "\tcfc2 $10,$10\n"   /* 8014A090: 484a5000 */
    "\tcfc2 $11,$11\n"   /* 8014A094: 484b5800 */
    "\tcfc2 $12,$12\n"   /* 8014A098: 484c6000 */
    "\tsw $8,0($4)\n"   /* 8014A09C: ac880000 */
    "\tsw $9,4($4)\n"   /* 8014A0A0: ac890004 */
    "\tsw $10,8($4)\n"   /* 8014A0A4: ac8a0008 */
    "\tsw $11,12($4)\n"   /* 8014A0A8: ac8b000c */
    "\tsw $12,16($4)\n"   /* 8014A0AC: ac8c0010 */
    "\tcfc2 $8,$13\n"   /* 8014A0B0: 48486800 */
    "\tcfc2 $9,$14\n"   /* 8014A0B4: 48497000 */
    "\tcfc2 $10,$15\n"   /* 8014A0B8: 484a7800 */
    "\tsw $8,20($4)\n"   /* 8014A0BC: ac880014 */
    "\tsw $9,24($4)\n"   /* 8014A0C0: ac890018 */
    "\tsw $10,28($4)\n"   /* 8014A0C4: ac8a001c */
    "\tjr $31\n"   /* 8014A0C8: 03e00008 */
    "\tnop\n"   /* 8014A0CC: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A088\n"
    ".size sub_8014A088,.-sub_8014A088\n"
    ".text\n");

/* ReadColorMatrix: read the GTE light-color matrix and far color into m. Sony assembly. */
__asm__(
    ".section .text_8014A0D0,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A0D0\n"
    ".type sub_8014A0D0,@function\n"
    ".ent sub_8014A0D0\n"
    "sub_8014A0D0:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tcfc2 $8,$16\n"   /* 8014A0D0: 48488000 */
    "\tcfc2 $9,$17\n"   /* 8014A0D4: 48498800 */
    "\tcfc2 $10,$18\n"   /* 8014A0D8: 484a9000 */
    "\tcfc2 $11,$19\n"   /* 8014A0DC: 484b9800 */
    "\tcfc2 $12,$20\n"   /* 8014A0E0: 484ca000 */
    "\tsw $8,0($4)\n"   /* 8014A0E4: ac880000 */
    "\tsw $9,4($4)\n"   /* 8014A0E8: ac890004 */
    "\tsw $10,8($4)\n"   /* 8014A0EC: ac8a0008 */
    "\tsw $11,12($4)\n"   /* 8014A0F0: ac8b000c */
    "\tsw $12,16($4)\n"   /* 8014A0F4: ac8c0010 */
    "\tcfc2 $8,$21\n"   /* 8014A0F8: 4848a800 */
    "\tcfc2 $9,$22\n"   /* 8014A0FC: 4849b000 */
    "\tcfc2 $10,$23\n"   /* 8014A100: 484ab800 */
    "\tsw $8,20($4)\n"   /* 8014A104: ac880014 */
    "\tsw $9,24($4)\n"   /* 8014A108: ac890018 */
    "\tsw $10,28($4)\n"   /* 8014A10C: ac8a001c */
    "\tjr $31\n"   /* 8014A110: 03e00008 */
    "\tnop\n"   /* 8014A114: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A0D0\n"
    ".size sub_8014A0D0,.-sub_8014A0D0\n"
    ".text\n");

/* SetVertex0: load an SVECTOR into GTE V0. Sony assembly. */
__asm__(
    ".section .text_8014A124,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A124\n"
    ".type sub_8014A124,@function\n"
    ".ent sub_8014A124\n"
    "sub_8014A124:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 8014A124: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 8014A128: c8810004 */
    "\tjr $31\n"   /* 8014A12C: 03e00008 */
    "\tnop\n"   /* 8014A130: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A124\n"
    ".size sub_8014A124,.-sub_8014A124\n"
    ".text\n");

/* SetVertex1: load an SVECTOR into GTE V1. Sony assembly. */
__asm__(
    ".section .text_8014A134,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A134\n"
    ".type sub_8014A134,@function\n"
    ".ent sub_8014A134\n"
    "sub_8014A134:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $2,0($4)\n"   /* 8014A134: c8820000 */
    "\tlwc2 $3,4($4)\n"   /* 8014A138: c8830004 */
    "\tjr $31\n"   /* 8014A13C: 03e00008 */
    "\tnop\n"   /* 8014A140: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A134\n"
    ".size sub_8014A134,.-sub_8014A134\n"
    ".text\n");

/* SetVertex2: load an SVECTOR into GTE V2. Sony assembly. */
__asm__(
    ".section .text_8014A144,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A144\n"
    ".type sub_8014A144,@function\n"
    ".ent sub_8014A144\n"
    "sub_8014A144:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $4,0($4)\n"   /* 8014A144: c8840000 */
    "\tlwc2 $5,4($4)\n"   /* 8014A148: c8850004 */
    "\tjr $31\n"   /* 8014A14C: 03e00008 */
    "\tnop\n"   /* 8014A150: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A144\n"
    ".size sub_8014A144,.-sub_8014A144\n"
    ".text\n");

/* SetVertexTri: load three SVECTORs into GTE V0..V2. Sony assembly. */
__asm__(
    ".section .text_8014A154,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A154\n"
    ".type sub_8014A154,@function\n"
    ".ent sub_8014A154\n"
    "sub_8014A154:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $0,0($4)\n"   /* 8014A154: c8800000 */
    "\tlwc2 $1,4($4)\n"   /* 8014A158: c8810004 */
    "\tlwc2 $2,0($5)\n"   /* 8014A15C: c8a20000 */
    "\tlwc2 $3,4($5)\n"   /* 8014A160: c8a30004 */
    "\tlwc2 $4,0($6)\n"   /* 8014A164: c8c40000 */
    "\tlwc2 $5,4($6)\n"   /* 8014A168: c8c50004 */
    "\tjr $31\n"   /* 8014A16C: 03e00008 */
    "\tnop\n"   /* 8014A170: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A154\n"
    ".size sub_8014A154,.-sub_8014A154\n"
    ".text\n");

/* SetRGBfifo: load three CVECTORs into the RGB FIFO. Sony assembly. */
__asm__(
    ".section .text_8014A174,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A174\n"
    ".type sub_8014A174,@function\n"
    ".ent sub_8014A174\n"
    "sub_8014A174:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tlwc2 $20,0($4)\n"   /* 8014A174: c8940000 */
    "\tlwc2 $21,0($5)\n"   /* 8014A178: c8b50000 */
    "\tlwc2 $22,0($6)\n"   /* 8014A17C: c8d60000 */
    "\tjr $31\n"   /* 8014A180: 03e00008 */
    "\tnop\n"   /* 8014A184: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A174\n"
    ".size sub_8014A174,.-sub_8014A174\n"
    ".text\n");

/* SetIR123: load IR1..IR3. Sony assembly. */
__asm__(
    ".section .text_8014A188,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A188\n"
    ".type sub_8014A188,@function\n"
    ".ent sub_8014A188\n"
    "sub_8014A188:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$9\n"   /* 8014A188: 48844800 */
    "\tmtc2 $5,$10\n"   /* 8014A18C: 48855000 */
    "\tmtc2 $6,$11\n"   /* 8014A190: 48865800 */
    "\tjr $31\n"   /* 8014A194: 03e00008 */
    "\tnop\n"   /* 8014A198: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A188\n"
    ".size sub_8014A188,.-sub_8014A188\n"
    ".text\n");

/* SetIR0: load IR0. Sony assembly. */
__asm__(
    ".section .text_8014A19C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A19C\n"
    ".type sub_8014A19C,@function\n"
    ".ent sub_8014A19C\n"
    "sub_8014A19C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$8\n"   /* 8014A19C: 48844000 */
    "\tjr $31\n"   /* 8014A1A0: 03e00008 */
    "\tnop\n"   /* 8014A1A4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A19C\n"
    ".size sub_8014A19C,.-sub_8014A19C\n"
    ".text\n");

/* SetFarColor: set the far color RFC/GFC/BFC (components << 4). Sony assembly. */
__asm__(
    ".section .text_8014A1C8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A1C8\n"
    ".type sub_8014A1C8,@function\n"
    ".ent sub_8014A1C8\n"
    "sub_8014A1C8:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tsll $4,$4,0x4\n"   /* 8014A1C8: 00042100 */
    "\tsll $5,$5,0x4\n"   /* 8014A1CC: 00052900 */
    "\tsll $6,$6,0x4\n"   /* 8014A1D0: 00063100 */
    "\tctc2 $4,$21\n"   /* 8014A1D4: 48c4a800 */
    "\tctc2 $5,$22\n"   /* 8014A1D8: 48c5b000 */
    "\tctc2 $6,$23\n"   /* 8014A1DC: 48c6b800 */
    "\tjr $31\n"   /* 8014A1E0: 03e00008 */
    "\tnop\n"   /* 8014A1E4: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A1C8\n"
    ".size sub_8014A1C8,.-sub_8014A1C8\n"
    ".text\n");

/* SetSZfifo3: load SZ1..SZ3. Sony assembly. */
__asm__(
    ".section .text_8014A1E8,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A1E8\n"
    ".type sub_8014A1E8,@function\n"
    ".ent sub_8014A1E8\n"
    "sub_8014A1E8:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$17\n"   /* 8014A1E8: 48848800 */
    "\tmtc2 $5,$18\n"   /* 8014A1EC: 48859000 */
    "\tmtc2 $6,$19\n"   /* 8014A1F0: 48869800 */
    "\tjr $31\n"   /* 8014A1F4: 03e00008 */
    "\tnop\n"   /* 8014A1F8: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A1E8\n"
    ".size sub_8014A1E8,.-sub_8014A1E8\n"
    ".text\n");

/* SetSZfifo4: load SZ0..SZ3. Sony assembly. */
__asm__(
    ".section .text_8014A1FC,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A1FC\n"
    ".type sub_8014A1FC,@function\n"
    ".ent sub_8014A1FC\n"
    "sub_8014A1FC:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$16\n"   /* 8014A1FC: 48848000 */
    "\tmtc2 $5,$17\n"   /* 8014A200: 48858800 */
    "\tmtc2 $6,$18\n"   /* 8014A204: 48869000 */
    "\tmtc2 $7,$19\n"   /* 8014A208: 48879800 */
    "\tjr $31\n"   /* 8014A20C: 03e00008 */
    "\tnop\n"   /* 8014A210: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A1FC\n"
    ".size sub_8014A1FC,.-sub_8014A1FC\n"
    ".text\n");

/* SetSXSYfifo: load SXY0..SXY2. Sony assembly. */
__asm__(
    ".section .text_8014A214,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A214\n"
    ".type sub_8014A214,@function\n"
    ".ent sub_8014A214\n"
    "sub_8014A214:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$12\n"   /* 8014A214: 48846000 */
    "\tmtc2 $5,$13\n"   /* 8014A218: 48856800 */
    "\tmtc2 $6,$14\n"   /* 8014A21C: 48867000 */
    "\tjr $31\n"   /* 8014A220: 03e00008 */
    "\tnop\n"   /* 8014A224: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A214\n"
    ".size sub_8014A214,.-sub_8014A214\n"
    ".text\n");

/* SetRii: load the rotation-matrix control words holding the diagonal (R11R12, R22R23, R33). Sony assembly. */
__asm__(
    ".section .text_8014A228,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A228\n"
    ".type sub_8014A228,@function\n"
    ".ent sub_8014A228\n"
    "sub_8014A228:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tctc2 $4,$0\n"   /* 8014A228: 48c40000 */
    "\tctc2 $5,$2\n"   /* 8014A22C: 48c51000 */
    "\tctc2 $6,$4\n"   /* 8014A230: 48c62000 */
    "\tjr $31\n"   /* 8014A234: 03e00008 */
    "\tnop\n"   /* 8014A238: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A228\n"
    ".size sub_8014A228,.-sub_8014A228\n"
    ".text\n");

/* SetMAC123: load MAC1..MAC3. Sony assembly. */
__asm__(
    ".section .text_8014A23C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A23C\n"
    ".type sub_8014A23C,@function\n"
    ".ent sub_8014A23C\n"
    "sub_8014A23C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$25\n"   /* 8014A23C: 4884c800 */
    "\tmtc2 $5,$26\n"   /* 8014A240: 4885d000 */
    "\tmtc2 $6,$27\n"   /* 8014A244: 4886d800 */
    "\tjr $31\n"   /* 8014A248: 03e00008 */
    "\tnop\n"   /* 8014A24C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A23C\n"
    ".size sub_8014A23C,.-sub_8014A23C\n"
    ".text\n");

/* SetData32: write LZCS. Sony assembly. */
__asm__(
    ".section .text_8014A250,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A250\n"
    ".type sub_8014A250,@function\n"
    ".ent sub_8014A250\n"
    "sub_8014A250:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tmtc2 $4,$30\n"   /* 8014A250: 4884f000 */
    "\tjr $31\n"   /* 8014A254: 03e00008 */
    "\tnop\n"   /* 8014A258: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A250\n"
    ".size sub_8014A250,.-sub_8014A250\n"
    ".text\n");

/* SetDQA: set the depth-cue coefficient DQA. Sony assembly. */
__asm__(
    ".section .text_8014A280,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A280\n"
    ".type sub_8014A280,@function\n"
    ".ent sub_8014A280\n"
    "sub_8014A280:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tctc2 $4,$27\n"   /* 8014A280: 48c4d800 */
    "\tjr $31\n"   /* 8014A284: 03e00008 */
    "\tnop\n"   /* 8014A288: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A280\n"
    ".size sub_8014A280,.-sub_8014A280\n"
    ".text\n");

/* SetDQB: set the depth-cue offset DQB. Sony assembly. */
__asm__(
    ".section .text_8014A28C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014A28C\n"
    ".type sub_8014A28C,@function\n"
    ".ent sub_8014A28C\n"
    "sub_8014A28C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tctc2 $4,$28\n"   /* 8014A28C: 48c4e000 */
    "\tjr $31\n"   /* 8014A290: 03e00008 */
    "\tnop\n"   /* 8014A294: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014A28C\n"
    ".size sub_8014A28C,.-sub_8014A28C\n"
    ".text\n");

/* ReadSXSYfifo: read SXY0..SXY2. Sony assembly. */
__asm__(
    ".section .text_8014D62C,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014D62C\n"
    ".type sub_8014D62C,@function\n"
    ".ent sub_8014D62C\n"
    "sub_8014D62C:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tswc2 $12,0($4)\n"   /* 8014D62C: e88c0000 */
    "\tswc2 $13,0($5)\n"   /* 8014D630: e8ad0000 */
    "\tswc2 $14,0($6)\n"   /* 8014D634: e8ce0000 */
    "\tjr $31\n"   /* 8014D638: 03e00008 */
    "\tnop\n"   /* 8014D63C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014D62C\n"
    ".size sub_8014D62C,.-sub_8014D62C\n"
    ".text\n");

/* ReadRGBfifo: read RGB0..RGB2. Sony assembly. */
__asm__(
    ".section .text_8014D640,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014D640\n"
    ".type sub_8014D640,@function\n"
    ".ent sub_8014D640\n"
    "sub_8014D640:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tswc2 $20,0($4)\n"   /* 8014D640: e8940000 */
    "\tswc2 $21,0($5)\n"   /* 8014D644: e8b50000 */
    "\tswc2 $22,0($6)\n"   /* 8014D648: e8d60000 */
    "\tjr $31\n"   /* 8014D64C: 03e00008 */
    "\tnop\n"   /* 8014D650: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014D640\n"
    ".size sub_8014D640,.-sub_8014D640\n"
    ".text\n");

/* ReadGeomOffset: read the screen offset (OFX, OFY >> 16). Sony assembly. */
__asm__(
    ".section .text_8014D654,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014D654\n"
    ".type sub_8014D654,@function\n"
    ".ent sub_8014D654\n"
    "sub_8014D654:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tcfc2 $8,$24\n"   /* 8014D654: 4848c000 */
    "\tcfc2 $9,$25\n"   /* 8014D658: 4849c800 */
    "\tsra $8,$8,0x10\n"   /* 8014D65C: 00084403 */
    "\tsra $9,$9,0x10\n"   /* 8014D660: 00094c03 */
    "\tsw $8,0($4)\n"   /* 8014D664: ac880000 */
    "\tsw $9,0($5)\n"   /* 8014D668: aca90000 */
    "\tjr $31\n"   /* 8014D66C: 03e00008 */
    "\tnop\n"   /* 8014D670: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014D654\n"
    ".size sub_8014D654,.-sub_8014D654\n"
    ".text\n");

/* ReadGeomScreen: read the projection distance H. Sony assembly. */
__asm__(
    ".section .text_8014D674,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_8014D674\n"
    ".type sub_8014D674,@function\n"
    ".ent sub_8014D674\n"
    "sub_8014D674:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    ".set noat\n"
    "\tcfc2 $2,$26\n"   /* 8014D674: 4842d000 */
    "\tjr $31\n"   /* 8014D678: 03e00008 */
    "\tnop\n"   /* 8014D67C: 00000000 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_8014D674\n"
    ".size sub_8014D674,.-sub_8014D674\n"
    ".text\n");
