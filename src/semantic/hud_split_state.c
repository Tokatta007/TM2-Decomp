union TM2FontDrawMode { unsigned int words[3]; unsigned char bytes[12]; };
union TM2FontSprite { unsigned int words[5]; unsigned short halves[10]; unsigned char bytes[20]; };
union TM2HudRecord { unsigned int words[475]; unsigned short halves[950]; short signed_halves[950]; unsigned char bytes[1900]; };
struct TM2HudText30 { union TM2FontDrawMode mode; union TM2FontSprite sprites[30]; };
struct TM2HudText10 { union TM2FontDrawMode mode; union TM2FontSprite sprites[10]; };
typedef char hud_record_size[(sizeof(union TM2HudRecord)==1900)?1:-1];
typedef char hud_text30_size[(sizeof(struct TM2HudText30)==612)?1:-1];
typedef char hud_text10_size[(sizeof(struct TM2HudText10)==212)?1:-1];
union TM2HudDivider { unsigned int words[6]; unsigned short halves[12]; unsigned char bytes[24]; };
typedef char hud_divider_size[(sizeof(union TM2HudDivider)==24)?1:-1];
/* @80182160..80182308: each mode has twelve bytes plus ten sprite slots. */
struct TM2HudText10 hudSplitVertLivesMode __attribute__((section(".tm2_80182160"))) = {0};
struct TM2HudText10 hudSplitHorzLivesMode __attribute__((section(".tm2_80182234"))) = {0};
/* @80183EE8..80183F00: divider coordinates, tag and color share one packet. */
union TM2HudDivider hudSplitDivider __attribute__((section(".tm2_80183EE8"))) = {0};
