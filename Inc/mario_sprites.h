#ifndef __MARIO_SPRITES_H
#define __MARIO_SPRITES_H

#include <stdint.h>

/* RGB565 Palette colors */
#define COL_TRANSP   0x0001U /* Magic transparent key */
#define COL_SKY_DAY  0x5C9FU /* Classic NES sky blue */
#define COL_SKY_NGT  0x10A5U /* Level 2 twilight sky */
#define COL_HUD_BG   0x0000U
#define COL_WHITE    0xFFFFU
#define COL_BLACK    0x0000U
#define COL_RED      0xF800U
#define COL_DARKRED  0xA000U
#define COL_SKIN     0xFD52U
#define COL_BLUE     0x02BFU
#define COL_BROWN    0x9A60U
#define COL_BRICK    0xC244U
#define COL_BRICK_DK 0x7121U
#define COL_GROUND   0xCA43U
#define COL_GRASS    0x2E24U
#define COL_GOLD     0xFE60U
#define COL_GOLD_DK  0xC440U
#define COL_PIPE_LT  0x4FE6U
#define COL_PIPE_MD  0x05C0U
#define COL_PIPE_DK  0x0380U
#define COL_GRAY     0x8410U
#define COL_DARKGRAY 0x4208U
#define COL_CLOUD    0xFFFFU
#define COL_CLOUD_SH 0xCE7FU
#define COL_HILL_LT  0x2EC6U
#define COL_HILL_DK  0x04A2U
#define COL_P2_LT    0x0660U /* 2P Luigi green */
#define COL_P2_DK    0x0380U
#define COL_P3_LT    0xFE60U /* 3P Gold Mario */
#define COL_P3_DK    0xC440U
#define COL_P4_LT    0x05FFU /* 4P Cyan Mario */
#define COL_P4_DK    0x02BFU
#define COL_UG_BRK   0x1A9FU /* Underground blue brick */
#define COL_UG_DK    0x0155U
#define COL_CAST_WL  0x8410U /* Castle stone wall */
#define COL_CAST_DK  0x4208U
#define COL_UW_BG    0x0235U /* Underwater deep sea blue */
#define COL_WATER_LT 0x5EFFU /* Water surface cyan */
#define COL_WATER_DK 0x02BFU
#define COL_LAVA_LT  0xFC00U /* Castle lava orange */
#define COL_LAVA_DK  0xC000U
#define COL_PINK     0xFCDFU /* Princess Peach pink */

/* 16x16 sprite stored inline in Flash (0 RAM pointer table) */
typedef struct {
    const char rows[16][17];
} Sprite16;

static inline uint16_t sprite_palette(char ch)
{
    switch (ch) {
        case '.': return COL_TRANSP;
        case 'R': return COL_RED;
        case 'r': return COL_DARKRED;
        case 'S': return COL_SKIN;
        case 'B': return COL_BLUE;
        case 'N': return COL_BROWN;
        case 'Y': return COL_GOLD;
        case 'y': return COL_GOLD_DK;
        case 'W': return COL_WHITE;
        case 'K': return COL_BLACK;
        case 'G': return COL_PIPE_MD;
        case 'g': return COL_PIPE_LT;
        case 'd': return COL_PIPE_DK;
        case 'C': return COL_BRICK;
        case 'c': return COL_BRICK_DK;
        case 'D': return COL_GROUND;
        case 'L': return COL_GRASS;
        case 'A': return COL_GRAY;
        case 'a': return COL_DARKGRAY;
        case 'U': return COL_WATER_LT;
        case 'u': return COL_WATER_DK;
        case 'O': return COL_LAVA_LT;
        case 'P': return COL_PINK;
        default:  return COL_TRANSP;
    }
}

static inline uint16_t sprite_palette_slot(char ch, uint8_t slot)
{
    if (slot == 1U) {
        if (ch == 'R') return COL_P2_LT;
        if (ch == 'r') return COL_P2_DK;
    } else if (slot == 2U) {
        if (ch == 'R') return COL_P3_LT;
        if (ch == 'r') return COL_P3_DK;
        if (ch == 'B') return COL_DARKRED;
    } else if (slot == 3U) {
        if (ch == 'R') return COL_P4_LT;
        if (ch == 'r') return COL_P4_DK;
        if (ch == 'B') return COL_PIPE_DK;
    } else if (slot == 4U) {
        /* Fire Flower Mario: White cap & Red overalls */
        if (ch == 'R') return COL_WHITE;
        if (ch == 'r') return COL_GRAY;
        if (ch == 'B') return COL_RED;
    }
    return sprite_palette(ch);
}

static const Sprite16 SPR_MARIO_STAND = {{
    ".....RRRRRR.....",
    "....RRRRRRRRR...",
    "....NNNSSSKS....",
    "...NSNSSSSKSSS..",
    "...NSNNSSSSKSSS.",
    "...NNSSSSSKKKK..",
    ".....SSSSSSSS...",
    "....RRBRRBR.....",
    "...RRRBRRBRRR...",
    "..RRRRBBBBBRRRR.",
    "..SSRYBYYBYRSS..",
    "..SSSBBBBBBSSS..",
    "..SS.BBBBBB.SS..",
    ".....BBB.BBB....",
    "....NNN...NNN...",
    "...NNNN...NNNN.."
}};

static const Sprite16 SPR_MARIO_RUN1 = {{
    ".....RRRRRR.....",
    "....RRRRRRRRR...",
    "....NNNSSSKS....",
    "...NSNSSSSKSSS..",
    "...NSNNSSSSKSSS.",
    "...NNSSSSSKKKK..",
    ".....SSSSSSSS...",
    "....RRBRRR......",
    "...RRRBRRRRSS...",
    "..SSRRBBBBRSS...",
    "..SS.BBYYBB.....",
    ".....BBBBBBB....",
    "....BBBB.BBBB...",
    "...NNN....NNNN..",
    "..NNNN..........",
    "................"
}};

static const Sprite16 SPR_MARIO_RUN2 = {{
    ".....RRRRRR.....",
    "....RRRRRRRRR...",
    "....NNNSSSKS....",
    "...NSNSSSSKSSS..",
    "...NSNNSSSSKSSS.",
    "...NNSSSSSKKKK..",
    ".....SSSSSSSS...",
    ".....RRBRRR.....",
    "....RRRRBBRRR...",
    "...SSRRBBYYBR...",
    "...SS.BBBBBBB...",
    "......BBBBBB....",
    ".....BBBB.BBB...",
    ".....NNN...NNN..",
    "....NNNN........",
    "................"
}};

static const Sprite16 SPR_MARIO_JUMP = {{
    ".....RRRRRR..SS.",
    "....RRRRRRRRRSS.",
    "....NNNSSSKS.SS.",
    "...NSNSSSSKSSS..",
    "...NSNNSSSSKSS..",
    "...NNSSSSSKKKK..",
    ".....SSSSSSSS...",
    "..SSRRRRBRR.....",
    ".SSSRRRBBBBRR...",
    ".SS..BBYYBBBBR..",
    ".....BBBBBBBBB..",
    "....BBBBBBBB....",
    "...NNN....BBB...",
    "..NNNN....NNN...",
    "..........NNNN..",
    "................"
}};

static const Sprite16 SPR_GOOMBA_1 = {{
    "......NNNN......",
    "....NNNNNNNN....",
    "...NNNNNNNNNN...",
    "..NNNNNNNNNNNN..",
    ".NNKKNNNNNNKKNN.",
    ".NNKWKNNNNKWKNN.",
    ".NNKWKNNNNKWKNN.",
    ".NNNNNNNNNNNNNN.",
    "..NNNNKKKKNNNN..",
    "....SSSSSSSS....",
    "...SSSSSSSSSS...",
    "...SSSSSSSSSS...",
    "....SSSSSSSS....",
    "..KKKK....NNN...",
    ".KKKKKK...NNNN..",
    "................"
}};

static const Sprite16 SPR_GOOMBA_2 = {{
    "......NNNN......",
    "....NNNNNNNN....",
    "...NNNNNNNNNN...",
    "..NNNNNNNNNNNN..",
    ".NNKKNNNNNNKKNN.",
    ".NNKWKNNNNKWKNN.",
    ".NNKWKNNNNKWKNN.",
    ".NNNNNNNNNNNNNN.",
    "..NNNNKKKKNNNN..",
    "....SSSSSSSS....",
    "...SSSSSSSSSS...",
    "...SSSSSSSSSS...",
    "....SSSSSSSS....",
    "...NNN....KKKK..",
    "..NNNN...KKKKKK.",
    "................"
}};

static const Sprite16 SPR_GROUND = {{
    "LLLLLLLLLLLLLLLL",
    "LGLGLGLLGLGLLGLG",
    "LLLLLLLLLLLLLLLL",
    "cDDDDDDDDDDDcDDD",
    "cDDDDDDDcDDDDDDD",
    "DDDDcDDDDDDDcDDD",
    "DDcDDDDDDDDDDDDD",
    "DDDDDDDDcDDDDcDD",
    "DDDDcDDDDDDDDDDD",
    "cDDDDDDDDDcDDDDD",
    "DDDDDDcDDDDDDDcD",
    "DDcDDDDDDDDDDDDD",
    "DDDDDDDDcDDDDcDD",
    "DDDDcDDDDDDDDDDD",
    "cDDDDDDDDDcDDDDD",
    "DDDDDDDDDDDDDDDD"
}};

static const Sprite16 SPR_BRICK = {{
    "CCCCCCCCCCCCCCCC",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "cccccccccccccccc",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "cccccccccccccccc",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "cccccccccccccccc",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "cccccccccccccccc"
}};

static const Sprite16 SPR_QBLOCK = {{
    "cYYYYYYYYYYYYYYc",
    "YyyyyyyyyyyyyyyY",
    "YyKyyyyyyyyyyKyY",
    "YyyyyYYYYYYyyyyY",
    "YyyyYYWWWWYYyyyY",
    "YyyyYWKWWKWYyyyY",
    "YyyyYWKWWKWYyyyY",
    "YyyyyyyWWKyyyyyY",
    "YyyyyyWWKyyyyyyY",
    "YyyyyyWWyyyyyyyY",
    "YyyyyyKKyyyyyyyY",
    "YyyyyyWWyyyyyyyY",
    "YyyyyyKKyyyyyyyY",
    "YyKyyyyyyyyyyKyY",
    "YyyyyyyyyyyyyyyY",
    "cYYYYYYYYYYYYYYc"
}};

static const Sprite16 SPR_USED = {{
    "cNNNNNNNNNNNNNNc",
    "NccccccccccccccN",
    "NcKccccccccccKcN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NccccccccccccccN",
    "NcKccccccccccKcN",
    "NccccccccccccccN",
    "cNNNNNNNNNNNNNNc"
}};

static const Sprite16 SPR_PIPE_TL = {{
    "KKKKKKKKKKKKKKKK",
    "KggGGGGGGGGGGGGG",
    "KggGGGGGGGGGGGGG",
    "KggGGGGGGGGGGGGG",
    "KggGGGGGGGGGGGGG",
    "KggGGGGGGGGGGGGG",
    "KKKKKKKKKKKKKKKK",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG"
}};

static const Sprite16 SPR_PIPE_TR = {{
    "KKKKKKKKKKKKKKKK",
    "GGGGGGGGGGddddgK",
    "GGGGGGGGGGddddgK",
    "GGGGGGGGGGddddgK",
    "GGGGGGGGGGddddgK",
    "GGGGGGGGGGddddgK",
    "KKKKKKKKKKKKKKKK",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK.."
}};

static const Sprite16 SPR_PIPE_BL = {{
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG",
    ".KggGGGGGGGGGGGG"
}};

static const Sprite16 SPR_PIPE_BR = {{
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK..",
    "GGGGGGGGGGdddK.."
}};

static const Sprite16 SPR_COIN_1 = {{
    "................",
    ".....YYYYYY.....",
    "....YWWWWYYY....",
    "...YWWYYYYYYy...",
    "...YWWYYKKYYy...",
    "..YWWYYKWWKYyy..",
    "..YWWYYKWWKYyy..",
    "..YWWYYKWWKYyy..",
    "..YWWYYKWWKYyy..",
    "..YWWYYKWWKYyy..",
    "..YWWYYKWWKYyy..",
    "...YWWYYKKYYy...",
    "...YYYYYYYYYy...",
    "....YYYYYYyy....",
    ".....yyyyyy.....",
    "................"
}};

static const Sprite16 SPR_COIN_2 = {{
    "................",
    "......YYYY......",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YWWYYy.....",
    ".....YYYYyy.....",
    ".....YYYYy......",
    "......yyyy......",
    "................"
}};

static const Sprite16 SPR_FLAG_TOP = {{
    "......YYYY......",
    ".....YYYYYY.....",
    ".....YYYYYY.....",
    "......YYYY......",
    ".......GG.......",
    "..RRRRRGG.......",
    ".RRRRRRGG.......",
    "RRRRRRRGG.......",
    ".RRRRRRGG.......",
    "..RRRRRGG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG......."
}};

static const Sprite16 SPR_FLAG_POLE = {{
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG.......",
    ".......GG......."
}};

static const Sprite16 SPR_BLOCK_STONE = {{
    "WWWWWWWWWWWWWWWA",
    "WAAAAAAAAAAAAAAa",
    "WAWWWWWWWWWWWWaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "WAWAAAAAAAAAAaaa",
    "Waaaaaaaaaaaaaaa",
    "Aaaaaaaaaaaaaaaa",
    "aaaaaaaaaaaaaaaa"
}};

static const Sprite16 SPR_MUSHROOM = {{
    ".....KKKKKK.....",
    "...KKWRRRRKKK...",
    "..KWWWRRRRWWWK..",
    ".KWWWWRRRRWWWWK.",
    ".KWWWRRRRRRWWWK.",
    "KRRRRWWWRRRWRRRK",
    "KRRRWWWWWRWWWWWK",
    "KRRRRWWWRRRRWWWK",
    "KRRRRRRRRRRRRRRK",
    ".KKKKKKKKKKKKKK.",
    "..KSSSSSSSSSSK..",
    "..KSSSKSSKSSSK..",
    "..KSSSKSSKSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKKKKKKK...."
}};

static const Sprite16 SPR_FLOWER = {{
    "....KKKKKKKK....",
    "..KKRRRRRRRRKK..",
    ".KRRYYYYYYYYRRK.",
    "KRYWWWWWWWWWWYRK",
    "KRYWWKKKKKKWWYRK",
    "KRYWWKWWWWKWWYRK",
    ".KRRYYYYYYYYRRK.",
    "..KKRRRRRRRRKK..",
    "....KKKKKKKK....",
    ".......GG.......",
    "..KKKK.GG.KKKK..",
    ".KggggKGGKggggK.",
    ".KGGggGGGGggGGK.",
    "..KKKKGGGGKKKK..",
    ".......GG.......",
    ".......GG......."
}};

static const Sprite16 SPR_FIREBALL = {{
    "................",
    "................",
    "................",
    "......RRRR......",
    "....RRYYYYRR....",
    "...RYWWWWWWYR...",
    "...RYWWYYYYYR...",
    "...RYWYYYYYYR...",
    "...RYYYYYYWYR...",
    "...RYYYYWWYYR...",
    "....RRYYYYRR....",
    "......RRRR......",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_FIRE_PUFF = {{
    "................",
    "...Y........Y...",
    "....R......R....",
    "..Y..W....W..Y..",
    "...R..YYYY..R...",
    "....WYYYYYYW....",
    ".Y..YYWWWWYY..Y.",
    "....YYWWWWYY....",
    "....YYWWWWYY....",
    ".Y..YYWWWWYY..Y.",
    "....WYYYYYYW....",
    "...R..YYYY..R...",
    "..Y..W....W..Y..",
    "....R......R....",
    "...Y........Y...",
    "................"
}};

static const Sprite16 SPR_PIRANHA1 = {{
    "..WW........WW..",
    ".WWWW......WWWW.",
    ".WWWRR....RRWWW.",
    ".KRRWWR..RWWWRK.",
    ".KRRRRRRRRRRRRK.",
    ".KRWWWRRRRWWWRK.",
    ".KRRRRRRRRRRRRK.",
    "..KRRRRWWRRRRK..",
    "...KKKKKKKKKK...",
    "......KggK......",
    "...KK.KGGK.KK...",
    "..KggKKGGKKggK..",
    "..KGGGgGGgGGGK..",
    "...KKKGddGKKK...",
    "......KGGK......",
    "......KddK......"
}};

static const Sprite16 SPR_PIRANHA2 = {{
    ".....KKKKKK.....",
    "...KKWRRRRKKK...",
    "..KWWWRRRRWWWK..",
    ".KRRRRRWWWRRRRK.",
    ".KRWWWRRRRWWWRK.",
    ".KRRRRRRRRRRRRK.",
    ".KRRRWWWRRWWWRK.",
    "..KRRRRRRRRRRK..",
    "...KKKKKKKKKK...",
    "......KggK......",
    "...KK.KGGK.KK...",
    "..KggKKGGKKggK..",
    "..KGGGgGGgGGGK..",
    "...KKKGddGKKK...",
    "......KGGK......",
    "......KddK......"
}};

static const Sprite16 SPR_KOOPA1 = {{
    "....KKKK........",
    "...KSSSSK.......",
    "..KSWKKSSK......",
    "..KSWKSSSK......",
    "...KSSSSK..KKKK.",
    "....KSSKKKggggKK",
    "....KSSKgGGGGggK",
    "....KSSKgGGgGGGK",
    "....KSSKgGGGGggK",
    "....KKKKGGGGGGGK",
    ".....KSSSSSSSSK.",
    ".....KSSKYYKSSK.",
    "......KK.YY.KK..",
    ".....KGGK..KGGK.",
    "....KGGGK..KGGGK",
    "....KKKK....KKKK"
}};

static const Sprite16 SPR_KOOPA2 = {{
    "....KKKK........",
    "...KSSSSK.......",
    "..KSWKKSSK......",
    "..KSWKSSSK......",
    "...KSSSSK..KKKK.",
    "....KSSKKKggggKK",
    "....KSSKgGGGGggK",
    "....KSSKgGGgGGGK",
    "....KSSKgGGGGggK",
    "....KKKKGGGGGGGK",
    ".....KSSSSSSSSK.",
    ".....KSSKYYKSSK.",
    "......KK.YY.KK..",
    "......KGGKKGGK..",
    ".....KGGGK.KGGGK",
    ".....KKKK...KKKK"
}};

static const Sprite16 SPR_SHELL = {{
    "................",
    "................",
    "................",
    "................",
    "................",
    "....KKKKKKKK....",
    "..KKggggggggKK..",
    ".KgGGGGWWGGGGgK.",
    "KGGGGWWWWWWGGGGK",
    "KGGWWGGGGGGWWGGK",
    "KdGGGGGGGGGGGGdK",
    "KWWWWWWWWWWWWWWK",
    "KddddddddddddddK",
    ".KKKKKKKKKKKKKK.",
    "................",
    "................"
}};

static const Sprite16 SPR_PARATROOPA1 = {{
    "....KKKK...WW...",
    "...KSSSSK.WWWW..",
    "..KSWKKSSKWWWWK.",
    "..KSWKSSSKWWggK.",
    "...KSSSSKWWGGGGK",
    "....KSSKKKGGGGgK",
    "....KSSKgGGGGggK",
    "....KSSKgGGgGGGK",
    "....KSSKgGGGGggK",
    "....KKKKGGGGGGGK",
    ".....KSSSSSSSSK.",
    ".....KSSKYYKSSK.",
    "......KK.YY.KK..",
    ".....KGGK..KGGK.",
    "....KGGGK..KGGGK",
    "....KKKK....KKKK"
}};

static const Sprite16 SPR_PARATROOPA2 = {{
    "....KKKK........",
    "...KSSSSK.......",
    "..KSWKKSSK.WW...",
    "..KSWKSSSKWWWW..",
    "...KSSSSK.WWggK.",
    "....KSSKKKGGGGKK",
    "....KSSKgGGGGggK",
    "....KSSKgGGgGGGK",
    "....KSSKgGGGGggK",
    "....KKKKGGGGGGGK",
    ".....KSSSSSSSSK.",
    ".....KSSKYYKSSK.",
    "......KK.YY.KK..",
    "......KGGKKGGK..",
    ".....KGGGK.KGGGK",
    ".....KKKK...KKKK"
}};

static const Sprite16 SPR_BUZZY1 = {{
    "................",
    "................",
    "....KKKKKKKK....",
    "..KKuBBBBBBuKK..",
    ".KuBBBBBBBBBBuK.",
    "KBBBBWWBBBBWWBBK",
    "KBBBBWKBBBBWKBBK",
    "KBBBBBBBBBBBBBBK",
    "KddddddddddddddK",
    "KKKSSSSSSSSSSKKK",
    "KSKSSSSSSSSSSKSK",
    ".KKKKKKKKKKKKKK.",
    "..KuuK....KuuK..",
    ".KuuuK....KuuuK.",
    ".KKKKK....KKKKK.",
    "................"
}};

static const Sprite16 SPR_BUZZY2 = {{
    "................",
    "................",
    "....KKKKKKKK....",
    "..KKuBBBBBBuKK..",
    ".KuBBBBBBBBBBuK.",
    "KBBBBWWBBBBWWBBK",
    "KBBBBWKBBBBWKBBK",
    "KBBBBBBBBBBBBBBK",
    "KddddddddddddddK",
    "KKKSSSSSSSSSSKKK",
    "KSKSSSSSSSSSSKSK",
    ".KKKKKKKKKKKKKK.",
    "...KuuK..KuuK...",
    "..KuuuK..KuuuK..",
    "..KKKKK..KKKKK..",
    "................"
}};

static const Sprite16 SPR_SPINY1 = {{
    "....K......K....",
    "...KWK....KWK...",
    "..KWWWK..KWWWK..",
    ".KKRRRRKKRRRRKK.",
    "KRRRRWWWRRRWWWRK",
    "KRRRRWKKRRRWKKRK",
    "KRRRRRRRRRRRRRRK",
    "KrrrrrrrrrrrrrrK",
    "KWWWWWWWWWWWWWWK",
    ".KKKKKKKKKKKKKK.",
    "..KSSSSKKSSSSK..",
    "..KSSSK..KSSSK..",
    "..KKKK....KKKK..",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_SPINY2 = {{
    "...K........K...",
    "..KWK......KWK..",
    ".KWWWK....KWWWK.",
    ".KRRRRKKKKRRRRK.",
    "KRRRRWWWRRRWWWRK",
    "KRRRRWKKRRRWKKRK",
    "KRRRRRRRRRRRRRRK",
    "KrrrrrrrrrrrrrrK",
    "KWWWWWWWWWWWWWWK",
    ".KKKKKKKKKKKKKK.",
    "...KSSSKKKSSSK..",
    "...KSSSK.KSSSK..",
    "...KKKK...KKKK..",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_LAKITU = {{
    ".....KKKKKK.....",
    "....KggggggK....",
    "...KgGGKKGGgK...",
    "...KGKWWKWKGK...",
    "...KGKWKKWKGK...",
    "...KGGGGGGGGK...",
    "..KKKKKKKKKKKK..",
    ".KWWWWWWWWWWWWK.",
    "KWWWWWWWWWWWWWWK",
    "KWWKKKKKKKKKKWWK",
    "KWWKWWWWWWWWKWWK",
    "KWWWWWWWWWWWWWWK",
    ".KWWWWWWWWWWWWK.",
    "..KKKKKKKKKKKK..",
    "................",
    "................"
}};

static const Sprite16 SPR_HAMMER_BRO1 = {{
    "....KKKKKKKK....",
    "...KggggggggK...",
    "..KgGGGGGGGGgK..",
    "..KSSWKKWKKSSK..",
    "..KSSWKKWKKSSK..",
    "...KSSSSSSSSK...",
    "....KKGGGGKK....",
    "..SSKgGGGGgK....",
    ".SSSKGGGGGGK....",
    "..KKKGGWWGGK....",
    "....KGGGGGGK....",
    "....KSSSSSSK....",
    ".....KK..KK.....",
    "....KGGKKGGK....",
    "...KGGGKKGGGK...",
    "...KKKK..KKKK..."
}};

static const Sprite16 SPR_HAMMER_BRO2 = {{
    "....KKKKKKKK....",
    "...KggggggggK...",
    "..KgGGGGGGGGgK..",
    "..KSSWKKWKKSSK..",
    "..KSSWKKWKKSSK..",
    "...KSSSSSSSSK...",
    "....KKGGGGKK....",
    "....KggggggKSS..",
    "....KGGGGGGKSSS.",
    "....KGGWWGGKKK..",
    "....KGGGGGGK....",
    "....KSSSSSSK....",
    ".....KK..KK.....",
    "...KGGK..KGGK...",
    "..KGGGK..KGGGK..",
    "..KKKK....KKKK.."
}};

static const Sprite16 SPR_HAMMER = {{
    "................",
    "....KKKKKKKK....",
    "...KNNNNNNNNK...",
    "...KNNNNNNNNK...",
    "...KccccccccK...",
    "....KKKKKKKK....",
    "......KNNK......",
    "......KNNK......",
    "......KNNK......",
    "......KNNK......",
    "......KNNK......",
    "......KNNK......",
    "......KKKK......",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_BULLET = {{
    "................",
    "................",
    ".....KKKKKKKKKKK",
    "...KKAAAAAAAAAAK",
    "..KAAWWWWWWWWAAK",
    ".KAAWKKWWWWWWAAK",
    "KAAWWKKWWWWWWAAK",
    "KAAWWWWWWWWWWAAK",
    "KAAAAAAAAAAAAAAK",
    "KaaaaaaaaaaaaaaK",
    "KaaaaaaaaaaaaaaK",
    ".KaaaaaaaaaaaaK.",
    "..KaaaaaaaaaaK..",
    "...KKKKKKKKKKK..",
    "................",
    "................"
}};

static const Sprite16 SPR_CANNON_TOP = {{
    "KKKKKKKKKKKKKKKK",
    "KAAAAAAAAAAAAAAK",
    "KAAWWWWWWAAAAAAK",
    "KAAWWWWWWAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KaaaaaaaaaaaaaaK",
    "KKKKKKKKKKKKKKKK",
    ".KAAAAAAAAAAAAK.",
    ".KAAWWWWAAAAAAK.",
    ".KAAAAAAAAAAAAK.",
    ".KaaaaaaaaaaaaK.",
    ".KKKKKKKKKKKKKK.",
    ".KAAAAAAAAAAAAK.",
    ".KaaaaaaaaaaaaK.",
    ".KKKKKKKKKKKKKK.",
    ".KKKKKKKKKKKKKK."
}};

static const Sprite16 SPR_CANNON_BOT = {{
    ".KAAAAAAAAAAAAK.",
    ".KAAWWWWAAAAAAK.",
    ".KAAWWWWAAAAAAK.",
    ".KAAAAAAAAAAAAK.",
    ".KAAAAAAAAAAAAK.",
    ".KaaaaaaaaaaaaK.",
    ".KaaaaaaaaaaaaK.",
    ".KAAAAAAAAAAAAK.",
    ".KAAAAAAAAAAAAK.",
    ".KAAWWWWAAAAAAK.",
    ".KAAWWWWAAAAAAK.",
    ".KAAAAAAAAAAAAK.",
    ".KaaaaaaaaaaaaK.",
    ".KaaaaaaaaaaaaK.",
    ".KaaaaaaaaaaaaK.",
    "KKKKKKKKKKKKKKKK"
}};

static const Sprite16 SPR_CHEEP1 = {{
    "................",
    ".......KKKK.....",
    ".....KKGGGGKK...",
    "....KGGGGWWWWK..",
    "...KGGGGGWWKWK..",
    "..KGGGGGGWWWWK..",
    ".KWGGGGGGGGGGK..",
    "KWWGGGGGGGGGGGK.",
    "KWWGGGGWWWWGGGK.",
    ".KWGGGGWWWWGGK..",
    "..KGGGGGGGGGGK..",
    "...KKGGGGGGKK...",
    ".....KKKKKK.....",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_CHEEP2 = {{
    "................",
    ".......KKKK.....",
    ".....KKGGGGKK...",
    "....KGGGGWWWWK..",
    "...KGGGGGWWKWK..",
    "..KGGGGGGWWWWK..",
    "KWWGGGGGGGGGGK..",
    "KWWGGGGGGGGGGGK.",
    "KWWGGGGWWWWGGGK.",
    "KWWGGGGWWWWGGK..",
    "..KGGGGGGGGGGK..",
    "...KKGGGGGGKK...",
    ".....KKKKKK.....",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_BLOOBER1 = {{
    ".....KKKKKK.....",
    "...KKWWWWWWKK...",
    "..KWWWWWWWWWWK..",
    ".KWWWWWWWWWWWWK.",
    ".KWWKKWWWWKKWWK.",
    ".KWWKKWWWWKKWWK.",
    ".KWWWWWWWWWWWWK.",
    "..KWWWWWWWWWWK..",
    "...KKKKKKKKKK...",
    "..KWWKKWWKKWWK..",
    ".KWWK.KWWK.KWWK.",
    ".KWWK.KWWK.KWWK.",
    ".KKKK.KKKK.KKKK.",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_BLOOBER2 = {{
    ".....KKKKKK.....",
    "...KKWWWWWWKK...",
    "..KWWWWWWWWWWK..",
    ".KWWWWWWWWWWWWK.",
    ".KWWKKWWWWKKWWK.",
    ".KWWKKWWWWKKWWK.",
    ".KWWWWWWWWWWWWK.",
    "..KWWWWWWWWWWK..",
    "...KKKKKKKKKK...",
    "..KWWK.WW.KWWK..",
    "..KWWK.WW.KWWK..",
    "...KK..KK..KK...",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_BOWSER_TL = {{
    ".......WW.......",
    "......WWWW......",
    ".....KKWWKK.....",
    "....KGGGGGGK....",
    "...KGGGGGGGGK...",
    "..KGGWWKKGGGGK..",
    ".KGGGWKWKGGGGGK.",
    ".KGGGGGGGGGGGGK.",
    "KGGGGWWWWWWGGGGK",
    "KGGGGWKWKWKGGGGK",
    "KGGGGGGGGGGGGGGK",
    ".KKKKGGGGGGKKKK.",
    "....KYYYYYYK....",
    "...KYYYYYYYYK...",
    "..KGGGGGGGGGGK..",
    "..KGGGGGGGGGGK.."
}};

static const Sprite16 SPR_BOWSER_TR = {{
    "................",
    "................",
    "....KK..........",
    "...KWWK.........",
    "..KGGGGKK.......",
    ".KGGGGGGGK......",
    "KGGGGGGGGGK.....",
    "KGGGGWWGGGK.....",
    "KGGGWWWWGGGK....",
    "KGGGGGGGGGGK....",
    "KGGGGGGWWGGK....",
    "KGGGGGGWWGGK....",
    "KGGGGGGGGGGK....",
    ".KGGGGWWGGGK....",
    ".KGGGGWWGGGK....",
    "..KGGGGGGGGK...."
}};

static const Sprite16 SPR_BOWSER_BL = {{
    "..KYYYYYYYYYYK..",
    ".KYYYYYYYYYYYYK.",
    ".KYYYYYYYYYYYYK.",
    ".KGGGGYYYYGGGGK.",
    "..KGGGGGGGGGGK..",
    "..KGGGGGGGGGGK..",
    "...KGGGGGGGGK...",
    "...KGGGGGGGGK...",
    "....KGGKKGGK....",
    "...KGGK..KGGK...",
    "...KGGK..KGGK...",
    "..KGGGK..KGGGK..",
    "..KWWWK..KWWWK..",
    "..KKKK....KKKK..",
    "................",
    "................"
}};

static const Sprite16 SPR_BOWSER_BR = {{
    "..KGGGGGGGGK....",
    "..KGGGGGGWWK....",
    "...KGGGGGWWK....",
    "...KGGGGGGGK....",
    "....KGGGGGGK....",
    "....KGGGGGGK....",
    ".....KGGGGK.....",
    ".....KGGGGK.....",
    "....KGGKKGGK....",
    "....KGGK.KGGK...",
    "...KGGK...KGGK..",
    "...KGGK...KGGK..",
    "...KWWK...KWWK..",
    "...KKK.....KKK..",
    "................",
    "................"
}};

static const Sprite16 SPR_BOWSER_FIRE = {{
    "................",
    "................",
    "................",
    ".......RRRRR....",
    "....RRRYYYYYRR..",
    "..RRYYYYWWWWYYR.",
    ".ROOYYWWWWWWWWYR",
    "ROOOYYYYWWWWWWYR",
    "ROOOYYYYWWWWWWYR",
    ".ROOYYWWWWWWWWYR",
    "..RRYYYYWWWWYYR.",
    "....RRRYYYYYRR..",
    ".......RRRRR....",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_TOAD = {{
    "....KKKKKKKK....",
    "..KKWWWWWWWWKK..",
    ".KWWRRRWWWRRRWK.",
    ".KWWRRRWWWRRRWK.",
    "KWWWWWWWWWWWWWWK",
    "KRRWWWWWWWWWWWRK",
    "KRRWWWWWWWWWWWRK",
    ".KKKKKKKKKKKKKK.",
    "...KSSSSSSSSK...",
    "...KSSKSSKSSK...",
    "...KSSSSSSSSK...",
    "..KRRBBBBBBBRK..",
    "..KWWBBBBBBWWK..",
    "...KKKKKKKKKK...",
    "...KNNK..KNNK...",
    "...KKK....KKK..."
}};

static const Sprite16 SPR_PEACH = {{
    ".....YYYYYY.....",
    "....YYWWWWYY....",
    "....YYYYYYYY....",
    "...KYYYYYYYYK...",
    "...KYSSSSSSYK...",
    "...KSSKSSKSSK...",
    "...KSSSSSSSSK...",
    "....KPPPPPPK....",
    "...KPPPPPPPPK...",
    "..KPPPPPPPPPPK..",
    "..KPPWWWWWWPPK..",
    ".KPPPPPPPPPPPPK.",
    ".KPPPPPPPPPPPPK.",
    ".KKKKKKKKKKKKKK.",
    "....KRR..RRK....",
    "....KKK..KKK...."
}};

static const Sprite16 SPR_STAR = {{
    ".......KK.......",
    "......KYYK......",
    "......KYYK......",
    ".....KYYYYK.....",
    "KKKKKKYYYYKKKKKK",
    "KYYYYYYYYYYYYYYK",
    ".KYYYYKYYKYYYYK.",
    "..KYYYKYYKYYYK..",
    "...KYYKYYKYYK...",
    "...KYYYYYYYYK...",
    "..KYYYYYYYYYYK..",
    "..KYYYYKKYYYYK..",
    ".KYYYKK..KKYYYK.",
    ".KYKK......KKYK.",
    ".KK..........KK.",
    "................"
}};

static const Sprite16 SPR_AXE = {{
    "................",
    "......KKKK......",
    "....KKYYYYKK....",
    "...KYYWWWWYYK...",
    "..KYYWWKKWWYYK..",
    "..KYYWKKKKWYYK..",
    "..KYYWWKKWWYYK..",
    "...KYYWWWWYYK...",
    "....KKYYYYKK....",
    ".......KNNK.....",
    ".......KNNK.....",
    ".......KNNK.....",
    ".......KNNK.....",
    ".......KNNK.....",
    ".......KKKK.....",
    "................"
}};

static const Sprite16 SPR_CHAIN = {{
    "................",
    "................",
    "................",
    "................",
    "....KKKK........",
    "...KAAAAK.......",
    "..KAA..AAK......",
    "..KAA..AAKKKK...",
    "...KAAAAKAAAAK..",
    "....KKKKAA..AAK.",
    ".......KAA..AAK.",
    "........KAAAAK..",
    ".........KKKK...",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_BRIDGE = {{
    "YYYYYYYYYYYYYYYY",
    "yYYYYYYYYYYYYYYy",
    "cYYYYYYYYYYYYYYc",
    "KKKKKKKKKKKKKKKK",
    "..KNNN....KNNN..",
    "..KNNN....KNNN..",
    "..KNNN....KNNN..",
    "..KNNN....KNNN..",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_WATER_SURF = {{
    "................",
    "................",
    "....UU....UU....",
    "..UUUUU..UUUUU..",
    ".UUUUUUUUUUUUUU.",
    "UUUUUUUUUUUUUUUU",
    "UUUUuuuuUUUUuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu"
}};

static const Sprite16 SPR_WATER_FILL = {{
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu"
}};

static const Sprite16 SPR_LAVA_SURF = {{
    "................",
    "................",
    "...OOO....OOO...",
    ".OOOOOO..OOOOOO.",
    "OOOOOOOOOOOOOOOO",
    "OOrrOOOOOOrrOOOO",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr"
}};

static const Sprite16 SPR_LAVA_FILL = {{
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr",
    "rrrrrrrrrrrrrrrr"
}};

static const Sprite16 SPR_SPRING_UP = {{
    "KKKKKKKKKKKKKKKK",
    "KGGGGGGGGGGGGGGK",
    "KKKKKKKKKKKKKKKK",
    "....KKKKKKKK....",
    "....KAAAAAAK....",
    "....KKKKKKKK....",
    "....KAAAAAAK....",
    "....KKKKKKKK....",
    "....KAAAAAAK....",
    "....KKKKKKKK....",
    "....KAAAAAAK....",
    "....KKKKKKKK....",
    "....KAAAAAAK....",
    "KKKKKKKKKKKKKKKK",
    "KGGGGGGGGGGGGGGK",
    "KKKKKKKKKKKKKKKK"
}};

static const Sprite16 SPR_SPRING_DN = {{
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "KKKKKKKKKKKKKKKK",
    "KGGGGGGGGGGGGGGK",
    "KKKKKKKKKKKKKKKK",
    "....KKKKKKKK....",
    "....KAAAAAAK....",
    "....KKKKKKKK....",
    "....KAAAAAAK....",
    "KKKKKKKKKKKKKKKK",
    "KGGGGGGGGGGGGGGK",
    "KKKKKKKKKKKKKKKK"
}};

static const Sprite16 SPR_CLOUD_PLAT = {{
    "....KKKKKKKK....",
    "..KKWWWWWWWWKK..",
    ".KWWWWWWWWWWWWK.",
    "KWWWWWWWWWWWWWWK",
    "KWWKKKKKKKKKKWWK",
    "KWWKWWWWWWWWKWWK",
    "KWWWWWWWWWWWWWWK",
    "KWWWWWWWWWWWWWWK",
    ".KWWWWAAAAWWWWK.",
    "..KKKKAAAAKKKK..",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_MUSH_L = {{
    ".....KKKKKKKKKKK",
    "...KKRRRRRRRRRRR",
    "..KRRRRWWWRRRRRR",
    ".KRRRRWWWWWRRRRR",
    "KRRRRRRWWWRRRRRR",
    "KRRRRRRRRRRRRRRR",
    "KRRRRRRRRRRRRRRR",
    ".KKKKKKKKKKKKKKK",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_MUSH_M = {{
    "KKKKKKKKKKKKKKKK",
    "RRRRRRRRRRRRRRRR",
    "RRWWWRRRRRWWWRRR",
    "RWWWWWRRRWWWWWRR",
    "RRWWWRRRRRWWWRRR",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "KKKKKKKKKKKKKKKK",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_MUSH_R = {{
    "KKKKKKKKKKK.....",
    "RRRRRRRRRRRKK...",
    "RRRRRRWWWRRRRK..",
    "RRRRRWWWWWRRRRK.",
    "RRRRRRWWWRRRRRRK",
    "RRRRRRRRRRRRRRRK",
    "RRRRRRRRRRRRRRRK",
    "KKKKKKKKKKKKKKK.",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_MUSH_STEM = {{
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK.....",
    ".....KSSSSK....."
}};

static const Sprite16 SPR_SIDE_PIPE_T = {{
    "KKKKKKKKKKKKKKKK",
    "KggKgggggggggggg",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KggKGGGGGGGGGGGG",
    "KKKKKKKKKKKKKKKK"
}};

static const Sprite16 SPR_SIDE_PIPE_B = {{
    "KKKKKKKKKKKKKKKK",
    "KddKGGGGGGGGGGGG",
    "KddKGGGGGGGGGGGG",
    "KddKGGGGGGGGGGGG",
    "KddKGGGGGGGGGGGG",
    "KddKGGGGGGGGGGGG",
    "KddKGGGGGGGGGGGG",
    "KddKGGGGGGGGGGGG",
    "KddKGGGGGGGGGGGG",
    "KddKdddddddddddd",
    "KddKdddddddddddd",
    "KddKdddddddddddd",
    "KddKdddddddddddd",
    "KddKdddddddddddd",
    "KddKdddddddddddd",
    "KKKKKKKKKKKKKKKK"
}};

static const Sprite16 SPR_CASTLE_WALL = {{
    "CCCCCCCCCCCCCCCC",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "cccccccccccccccc",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "cccccccccccccccc",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "cccccccccccccccc",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "cccccccccccccccc"
}};

static const Sprite16 SPR_CASTLE_BATT = {{
    "cCCCCCCccCCCCCCc",
    "cCCCCCCccCCCCCCc",
    "cCCCCCCccCCCCCCc",
    "cCCCCCCccCCCCCCc",
    "cccccccccccccccc",
    "CCCCCCCCCCCCCCCC",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "cccccccccccccccc",
    "CCCcCCCCCCCcCCCC",
    "CCCcCCCCCCCcCCCC",
    "cccccccccccccccc",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "CCCCCCCcCCCCCCCc",
    "cccccccccccccccc"
}};

static const Sprite16 SPR_CASTLE_ARCH = {{
    "CCCcKKKKKKKKcCCC",
    "CCCcKKKKKKKKcCCC",
    "CCcKKKKKKKKKKcCC",
    "CCcKKKKKKKKKKcCC",
    "CcKKKKKKKKKKKKcC",
    "CcKKKKKKKKKKKKcC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC",
    "CKKKKKKKKKKKKKKC"
}};

static const Sprite16 SPR_ROPE = {{
    ".......KK.......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    "......KYYK......",
    ".......KK......."
}};

static const Sprite16 SPR_VINE = {{
    "......KGGK......",
    "....KKgGGK......",
    "...KgGGggK......",
    "...KGGgGGK......",
    "....KKKGGK.KK...",
    "......KGGKKggK..",
    "......KGGggGGK..",
    "......KGGgGGK...",
    "......KGGKKK....",
    "....KKgGGK......",
    "...KgGGggK......",
    "...KGGgGGK......",
    "....KKKGGK......",
    "......KGGK......",
    "......KGGK......",
    "......KGGK......"
}};

static const Sprite16 SPR_PLAT = {{
    "YYYYYYYYYYYYYYYY",
    "yYYYYYYYYYYYYYYy",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................"
}};

static const Sprite16 SPR_GROUND_SUB = {{
    "DDDDDDDDDDDDDDDD",
    "cDDDDDDDDDDDcDDD",
    "cDDDDDDDcDDDDDDD",
    "DDDDcDDDDDDDcDDD",
    "DDcDDDDDDDDDDDDD",
    "DDDDDDDDcDDDDcDD",
    "DDDDcDDDDDDDDDDD",
    "cDDDDDDDDDcDDDDD",
    "DDDDDDcDDDDDDDcD",
    "DDcDDDDDDDDDDDDD",
    "DDDDDDDDcDDDDcDD",
    "DDDDcDDDDDDDDDDD",
    "cDDDDDDDDDcDDDDD",
    "DDDDDDcDDDDDDDcD",
    "DDcDDDDDDDDDDDDD",
    "DDDDDDDDDDDDDDDD"
}};

#endif
