#ifndef __MARIO_UI_H
#define __MARIO_UI_H

#include "mario_sprites.h"
#include <stdint.h>

#define UI_SCREEN_W 320
#define UI_TILE_SZ  16

typedef struct {
    char    ch;
    uint8_t rows[5]; /* 5 rows, 4 LSB bits each (MSB = col 0) */
} Font4x5Glyph;

static const Font4x5Glyph FONT4X5[] = {
    {' ', {0x0, 0x0, 0x0, 0x0, 0x0}},
    {'0', {0x6, 0x9, 0x9, 0x9, 0x6}},
    {'1', {0x2, 0x6, 0x2, 0x2, 0x7}},
    {'2', {0xE, 0x1, 0x6, 0x8, 0xF}},
    {'3', {0xE, 0x1, 0x6, 0x1, 0xE}},
    {'4', {0x9, 0x9, 0xF, 0x1, 0x1}},
    {'5', {0xF, 0x8, 0xE, 0x1, 0xE}},
    {'6', {0x6, 0x8, 0xE, 0x9, 0x6}},
    {'7', {0xF, 0x1, 0x2, 0x4, 0x4}},
    {'8', {0x6, 0x9, 0x6, 0x9, 0x6}},
    {'9', {0x6, 0x9, 0x7, 0x1, 0x6}},
    {'A', {0x6, 0x9, 0xF, 0x9, 0x9}},
    {'B', {0xE, 0x9, 0xE, 0x9, 0xE}},
    {'C', {0x7, 0x8, 0x8, 0x8, 0x7}},
    {'D', {0xE, 0x9, 0x9, 0x9, 0xE}},
    {'E', {0xF, 0x8, 0xE, 0x8, 0xF}},
    {'F', {0xF, 0x8, 0xE, 0x8, 0x8}},
    {'G', {0x7, 0x8, 0xB, 0x9, 0x7}},
    {'H', {0x9, 0x9, 0xF, 0x9, 0x9}},
    {'I', {0x7, 0x2, 0x2, 0x2, 0x7}},
    {'J', {0x3, 0x1, 0x1, 0x9, 0x6}},
    {'K', {0x9, 0xA, 0xC, 0xA, 0x9}},
    {'L', {0x8, 0x8, 0x8, 0x8, 0xF}},
    {'M', {0x9, 0xF, 0xF, 0x9, 0x9}},
    {'N', {0x9, 0xD, 0xB, 0x9, 0x9}},
    {'O', {0x6, 0x9, 0x9, 0x9, 0x6}},
    {'P', {0xE, 0x9, 0xE, 0x8, 0x8}},
    {'R', {0xE, 0x9, 0xE, 0xA, 0x9}},
    {'S', {0x7, 0x8, 0x6, 0x1, 0xE}},
    {'T', {0xF, 0x4, 0x4, 0x4, 0x4}},
    {'U', {0x9, 0x9, 0x9, 0x9, 0x6}},
    {'V', {0x9, 0x9, 0x9, 0x6, 0x6}},
    {'W', {0x9, 0x9, 0xF, 0xF, 0x9}},
    {'X', {0x9, 0x6, 0x6, 0x6, 0x9}},
    {'Y', {0x9, 0x9, 0x6, 0x2, 0x2}},
    {'-', {0x0, 0x0, 0xE, 0x0, 0x0}},
    {':', {0x0, 0x4, 0x0, 0x4, 0x0}},
    {'*', {0x0, 0xA, 0x4, 0xA, 0x0}},
    {'!', {0x4, 0x4, 0x4, 0x0, 0x4}},
    {'/', {0x1, 0x2, 0x4, 0x4, 0x8}},
    {'+', {0x0, 0x4, 0xE, 0x4, 0x0}},
};

static inline const uint8_t *ui_find_glyph4x5(char ch)
{
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }
    for (unsigned i = 0; i < (unsigned)(sizeof(FONT4X5) / sizeof(FONT4X5[0])); i++) {
        if (FONT4X5[i].ch == ch) {
            return FONT4X5[i].rows;
        }
    }
    return FONT4X5[0].rows;
}

static inline void ui_draw_text4x5(uint16_t *buf, int16_t x0, int16_t y0, const char *s, uint16_t fg)
{
    int16_t cx = x0;
    while (*s) {
        const uint8_t *rows = ui_find_glyph4x5(*s++);
        for (int r = 0; r < 5; r++) {
            int16_t py = (int16_t)(y0 + r);
            if (py < 0 || py >= UI_TILE_SZ) {
                continue;
            }
            uint8_t bits = rows[r];
            for (int c = 0; c < 4; c++) {
                if (bits & (1U << (3 - c))) {
                    int16_t px = (int16_t)(cx + c);
                    if (px >= 0 && px < UI_SCREEN_W) {
                        buf[py * UI_SCREEN_W + px] = fg;
                    }
                }
            }
        }
        cx = (int16_t)(cx + 5);
    }
}

/* 8x8 mini HUD icons stored as strings ('.'=transparent) */
static const char ICON_COIN_8[8][9] = {
    "..YYYY..",
    ".YWYYYY.",
    "YWYYKYYy",
    "YWYYKYYy",
    "YWYYKYYy",
    "YWYYKYYy",
    ".YYYYYy.",
    "..yyyy.."
};

static const char ICON_HEART_8[8][9] = {
    ".RR..RR.",
    "RWWWRRRR",
    "RWWRRRRR",
    "RRRRRRRR",
    ".RRRRRR.",
    "..RRRR..",
    "...RR...",
    "........"
};

static const char ICON_HAT_8[8][9] = {
    "..RRRR..",
    ".RRRRRRR",
    ".NNSSKS.",
    "NSNSSKSS",
    ".NSSSSKK",
    "..SSSS..",
    ".RRBRRR.",
    "..BBBB.."
};

static const char ICON_SPK_ON_8[8][9] = {
    "....WW.W",
    "...WWW..",
    ".WWWWW.W",
    "WWWWWW..",
    "WWWWWW..",
    ".WWWWW.W",
    "...WWW..",
    "....WW.W"
};

static const char ICON_SPK_OFF_8[8][9] = {
    "R...AA..",
    ".R.AAA..",
    ".RWAAA..",
    "AAAWR...",
    "AAARWA..",
    ".AAA.R..",
    "...AA.R.",
    "......R."
};

static inline void ui_draw_icon8(uint16_t *buf, int16_t x0, int16_t y0, const char icon[8][9])
{
    for (int r = 0; r < 8; r++) {
        int16_t py = (int16_t)(y0 + r);
        if (py < 0 || py >= UI_TILE_SZ) {
            continue;
        }
        for (int c = 0; c < 8; c++) {
            char ch = icon[r][c];
            if (ch == '.') {
                continue;
            }
            int16_t px = (int16_t)(x0 + c);
            if (px >= 0 && px < UI_SCREEN_W) {
                buf[py * UI_SCREEN_W + px] = sprite_palette(ch);
            }
        }
    }
}

/* Rounded NES-like cloud & hill silhouette helper for rows 2..3 (clouds) and 10..11 (hills/bushes)
 * sitting cleanly on row 12 ground! Zero floating gaps, theme-aware muted night palette. */
static inline void ui_draw_scenery_band(uint16_t *buf, int16_t row, int16_t cam_x,
                                          uint8_t area_type, uint8_t night, uint8_t snow,
                                          uint8_t bg_scenery, uint16_t bg_col)
{
    /* Fast 32-bit solid fill */
    uint32_t bg32 = ((uint32_t)bg_col << 16) | bg_col;
    uint32_t *dst32 = (uint32_t *)buf;
    for (int i = 0; i < (UI_SCREEN_W * UI_TILE_SZ) / 2; i++) {
        dst32[i] = bg32;
    }

    /* Night stars in upper rows */
    if (area_type == 1U && night && row >= 1 && row <= 6) {
        static const uint16_t STAR_X[10] = {22, 68, 114, 149, 193, 237, 281, 305, 44, 259};
        static const uint8_t  STAR_R[10] = { 2,  5,   3,   9,   4,  12,   2,   7, 13,  10};
        for (int i = 0; i < 10; i++) {
            int16_t sx = (int16_t)((STAR_X[i] - (cam_x >> 3)) % UI_SCREEN_W);
            if (sx < 0) sx += UI_SCREEN_W;
            if ((i % 6) + 1 == row) {
                int16_t ry = STAR_R[i];
                buf[ry * UI_SCREEN_W + sx] = (i & 1) ? 0xFE60U : 0xCE7FU;
            }
        }
    }

    if (area_type == 0U) {
        /* Underwater subtle rising bubbles */
        if (row >= 2 && row <= 10) {
            int16_t bx = (int16_t)(((row * 73) - (cam_x >> 1)) % UI_SCREEN_W);
            if (bx < 0) bx += UI_SCREEN_W;
            if (bx >= 2 && bx < UI_SCREEN_W - 2) {
                buf[6 * UI_SCREEN_W + bx] = 0x5EFFU;
                buf[7 * UI_SCREEN_W + bx - 1] = 0x5EFFU;
                buf[7 * UI_SCREEN_W + bx + 1] = 0x5EFFU;
                buf[8 * UI_SCREEN_W + bx] = 0x5EFFU;
            }
        }
        return;
    }

    if (area_type != 1U || bg_scenery == 0U) {
        return; /* Underground/Castle stay clean dark */
    }

    uint16_t cl_top = night ? 0x4A69U : (snow ? 0xFFFFU : 0xFFFFU);
    uint16_t cl_sh  = night ? 0x2965U : (snow ? 0xBDD7U : 0xB6FFU);
    uint16_t cl_rim = night ? 0x18E3U : 0x39DFU;
    uint16_t hl_lt  = night ? 0x0361U : (snow ? 0x54E8U : COL_HILL_LT);
    uint16_t hl_dk  = night ? 0x01C0U : (snow ? 0x2B44U : COL_HILL_DK);
    uint16_t hl_rim = night ? 0x00C0U : 0x0260U;

    /* Puffy rounded clouds in rows 2..3 (smooth pixel silhouette) */
    if (row == 2 || row == 3) {
        static const uint8_t CLOUD_HW[19] = {
            7, 10, 12, 14, 15, 17, 20, 22, 23, 24, 24, 24, 24, 23, 22, 20, 17, 13, 8
        };
        int16_t scroll = (int16_t)((cam_x / 3) % 144);
        for (int k = -1; k < 4; k++) {
            int16_t cx = (int16_t)(k * 144 - scroll + 36);
            int16_t y0 = (row == 2) ? 7 : 0;
            for (int r = y0; r < UI_TILE_SZ; r++) {
                int16_t dy = (row == 2) ? (r - 7) : (r + 9);
                if (dy >= 19) {
                    continue;
                }
                int16_t half_w = CLOUD_HW[dy];
                int16_t xL = (int16_t)(cx - half_w);
                int16_t xR = (int16_t)(cx + half_w);
                for (int16_t x = xL; x <= xR; x++) {
                    if (x >= 0 && x < UI_SCREEN_W) {
                        if (x == xL || x == xR || dy == 0 || dy == 18) {
                            buf[r * UI_SCREEN_W + x] = cl_rim;
                        } else if (dy >= 15) {
                            buf[r * UI_SCREEN_W + x] = cl_sh;
                        } else {
                            buf[r * UI_SCREEN_W + x] = cl_top;
                        }
                    }
                }
            }
        }
    }

    /* Rounded hills (rows 10..11, base touching row 12 ground!) & bushes (row 11) */
    if (row == 10 || row == 11) {
        int16_t scroll = (int16_t)((cam_x / 2) % 192);
        for (int k = -1; k < 3; k++) {
            int16_t hx = (int16_t)(k * 192 - scroll + 36);
            for (int r = 0; r < UI_TILE_SZ; r++) {
                int16_t dy = (row == 10) ? r : (r + 16);
                if (dy < 4) {
                    continue; /* starts cleanly at y=4 of row 10 */
                }
                int16_t h = (int16_t)(dy - 4); /* 0..27 */
                int16_t span = (h < 4) ? (int16_t)(4 + h * 2) : (int16_t)(10 + h);
                int16_t xL = (int16_t)(hx - span);
                int16_t xR = (int16_t)(hx + span);
                for (int16_t x = xL; x <= xR; x++) {
                    if (x >= 0 && x < UI_SCREEN_W) {
                        uint16_t col = hl_lt;
                        if (x <= xL + 1 || x >= xR - 1 || h == 0) {
                            col = hl_rim;
                        } else if ((h >= 10 && h <= 14 && x >= hx - 7 && x <= hx - 3) ||
                                   (h >= 19 && h <= 23 && x >= hx + 5 && x <= hx + 9) ||
                                   (x > hx + span - 5)) {
                            col = hl_dk;
                        }
                        buf[r * UI_SCREEN_W + x] = col;
                    }
                }
            }

            /* Bush sitting cleanly on row 12 ground (drawn in row 11) */
            if (row == 11 && bg_scenery != 3U) {
                int16_t bx = (int16_t)(hx + 88);
                for (int r = 5; r < UI_TILE_SZ; r++) {
                    int16_t dy = (int16_t)(r - 5);
                    int16_t span = (dy < 3) ? (int16_t)(7 + dy * 3) : 16;
                    for (int16_t x = (int16_t)(bx - span); x <= (int16_t)(bx + span); x++) {
                        if (x >= 0 && x < UI_SCREEN_W) {
                            buf[r * UI_SCREEN_W + x] = (dy == 0 || x == bx - span || x == bx + span) ? hl_rim :
                                                       (dy >= 8 ? hl_dk : hl_lt);
                        }
                    }
                }
            }
        }
    }
}

#endif
