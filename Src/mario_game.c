#include "mario_game.h"
#include "mario_sprites.h"
#include "mario_audio.h"
#include "mario_levels.h"
#include "mario_ui.h"
#include "ld3320_audio.h"
#include "wifi_net.h"
#include "lcd.h"
#include "gpio_api.h"
#include "usart.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>
#include <string.h>

#define SCREEN_W       320
#define SCREEN_H       240
#define HUD_H          16
#define TILE_SIZE      16
#define MAP_ROWS       14
#define MAP_MAX_COLS   400
#define WORLD_H        (MAP_ROWS * TILE_SIZE)

#define MAX_ENEMIES    20
#define MAX_PLATFORMS  8
#define MAX_FIREBARS   8
#define MAX_POPUPS     8
#define MAX_ITEMS      4
#define MAX_FIREBALLS  4
#define FRAME_MS       16U

#define KEY_FIRE       0x10U
#define KEY_DOWN       0x20U
#define KEY_DASH       0x40U

/* Tile IDs matching tools/convert_smb_levels.py */
enum {
    T_EMPTY = 0,
    T_GROUND,
    T_BRICK,
    T_QBLOCK,
    T_USED,
    T_PIPE_TL,
    T_PIPE_TR,
    T_PIPE_BL,
    T_PIPE_BR,
    T_COIN,
    T_FLAG_TOP,
    T_FLAG_POLE,
    T_STONE,
    T_QBLOCK_MUSH,
    T_QBLOCK_FLOWER,
    T_QBLOCK_HIDDEN_COIN,
    T_QBLOCK_HIDDEN_1UP,
    T_BRICK_POWER,
    T_BRICK_VINE,
    T_BRICK_STAR,
    T_BRICK_10COIN,
    T_BRICK_1UP,
    T_BRIDGE,
    T_WATER_SURF,
    T_WATER_FILL,
    T_LAVA_SURF,
    T_LAVA_FILL,
    T_CANNON_TOP,
    T_CANNON_MID,
    T_CANNON_BOT,
    T_AXE,
    T_CHAIN,
    T_SPRING_TOP,
    T_SPRING_BOT,
    T_CLOUD_PLAT,
    T_MUSH_PLAT_L,
    T_MUSH_PLAT_M,
    T_MUSH_PLAT_R,
    T_MUSH_STEM,
    T_PIPE_WARP_TL,
    T_PIPE_WARP_TR,
    T_SIDE_PIPE_T,
    T_SIDE_PIPE_B,
    T_CASTLE_WALL,
    T_CASTLE_ARCH,
    T_CASTLE_BATT,
    T_ROPE,
    T_COUNT
};

/* Flash Sprite IDs matching SPR_LIST order */
enum {
    S_MARIO_STAND = 0,
    S_MARIO_RUN1,
    S_MARIO_RUN2,
    S_MARIO_JUMP,
    S_GOOMBA1,
    S_GOOMBA2,
    S_GROUND,
    S_BRICK,
    S_QBLOCK,
    S_USED,
    S_PIPE_TL,
    S_PIPE_TR,
    S_PIPE_BL,
    S_PIPE_BR,
    S_COIN1,
    S_COIN2,
    S_FLAG_TOP,
    S_FLAG_POLE,
    S_STONE,
    S_MUSHROOM,
    S_FLOWER,
    S_FIREBALL,
    S_FIRE_PUFF,
    S_PIRANHA1,
    S_PIRANHA2,
    S_KOOPA1,
    S_KOOPA2,
    S_SHELL,
    S_PARATROOPA1,
    S_PARATROOPA2,
    S_BUZZY1,
    S_BUZZY2,
    S_SPINY1,
    S_SPINY2,
    S_LAKITU,
    S_HAMMER_BRO1,
    S_HAMMER_BRO2,
    S_HAMMER,
    S_BULLET,
    S_CANNON_TOP,
    S_CANNON_BOT,
    S_CHEEP1,
    S_CHEEP2,
    S_BLOOBER1,
    S_BLOOBER2,
    S_BOWSER_TL,
    S_BOWSER_TR,
    S_BOWSER_BL,
    S_BOWSER_BR,
    S_BOWSER_FIRE,
    S_TOAD,
    S_PEACH,
    S_STAR,
    S_AXE,
    S_CHAIN,
    S_BRIDGE,
    S_WATER_SURF,
    S_WATER_FILL,
    S_LAVA_SURF,
    S_LAVA_FILL,
    S_SPRING_UP,
    S_SPRING_DN,
    S_CLOUD_PLAT,
    S_MUSH_L,
    S_MUSH_M,
    S_MUSH_R,
    S_MUSH_STEM,
    S_SIDE_PIPE_T,
    S_SIDE_PIPE_B,
    S_CASTLE_WALL,
    S_CASTLE_BATT,
    S_CASTLE_ARCH,
    S_ROPE,
    S_VINE,
    S_PLAT,
    S_GROUND_SUB,
    S_SPRITE_COUNT
};

/* Enemy kinds matching tools/convert_smb_levels.py */
enum {
    EN_NONE = 0,
    EN_GOOMBA = 1,
    EN_KOOPA_G = 2,
    EN_KOOPA_R = 3,
    EN_PARATROOPA_J = 4,
    EN_PARATROOPA_F = 5,
    EN_PARATROOPA_H = 6,
    EN_PIRANHA = 7,
    EN_BUZZY = 8,
    EN_SPINY = 9,
    EN_LAKITU = 10,
    EN_HAMMER_BRO = 11,
    EN_BULLET = 12,
    EN_BLOOBER = 13,
    EN_CHEEP_GREY = 14,
    EN_CHEEP_RED = 15,
    EN_PODOBOO = 16,
    EN_BOWSER = 17,
    EN_TOAD_PEACH = 18,
    EN_SHELL = 19,
    EN_HAMMER = 20,
    EN_BOWSER_FIRE = 21
};

enum {
    SP_FIREBAR_SHORT = 1,
    SP_FIREBAR_LONG = 2,
    SP_PLAT_VERT = 3,
    SP_PLAT_HORIZ = 4,
    SP_PLAT_UP = 5,
    SP_PLAT_DOWN = 6,
    SP_PLAT_DROP = 7,
    SP_PLAT_BAL_L = 8
};

enum {
    ITEM_NONE = 0,
    ITEM_MUSHROOM = 1,
    ITEM_FLOWER = 2,
    ITEM_STAR = 3,
    ITEM_1UP = 4,
    ITEM_VINE = 5
};

enum {
    POP_NONE = 0,
    POP_COIN = 1,
    POP_BRICK_TL = 2,
    POP_BRICK_TR = 3,
    POP_SCORE_100 = 4,
    POP_SCORE_200 = 5,
    POP_SCORE_1000 = 6,
    POP_SCORE_1UP = 7
};

typedef enum {
    STATE_TITLE = 0,
    STATE_LOBBY,
    STATE_PLAYING,
    STATE_DEAD_WAIT,
    STATE_LEVEL_CLEAR,
    STATE_GAME_OVER,
    STATE_WIN,
    STATE_PRE_STAGE
} GameState;

typedef enum {
    SFX_INTRO = 0,
    SFX_JUMP,
    SFX_COIN,
    SFX_HIT,
    SFX_HURT,
    SFX_POWERUP,
    SFX_CLEAR,
    SFX_DIE,
    SFX_1UP,
    SFX_PIPE,
    SFX_FIREBALL,
    SFX_BOWSER
} SfxId;

typedef enum {
    BGM_NONE = 0,
    BGM_OVERWORLD,
    BGM_UNDERGROUND,
    BGM_UNDERWATER,
    BGM_CASTLE,
    BGM_STAR,
    BGM_COUNT
} BgmTrack;

typedef struct {
    int32_t x_fp; /* 8.8 fixed point */
    int32_t y_fp;
    int32_t vx_fp;
    int32_t vy_fp;
    int8_t  dir;
    uint8_t on_ground;
    uint8_t invuln_ticks;
    uint8_t anim_tick;
    uint8_t power;          /* 0=Small, 1=Super, 2=Fire */
    uint8_t prev_power;
    uint8_t transform_ticks;
    uint8_t fire_cd;
    uint16_t star_ticks;
    uint8_t run_ticks;
    uint8_t crouch;
} Player;

typedef struct {
    int16_t  x;
    int16_t  y;
    int16_t  base_x;
    int16_t  base_y;
    int8_t   vx;
    int16_t  vy;
    uint8_t  kind;
    uint8_t  alive;
    uint8_t  sub;        /* shell origin or bowser hp or piranha h_off */
    uint8_t  timer;
    uint8_t  dir;
    uint8_t  squash_timer;
} Enemy;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t base_y;
    int16_t base_x;
    int8_t  vx;
    int8_t  vy;
    uint8_t kind;
    uint8_t active;
    uint8_t timer;
} Platform;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t angle;
    uint8_t len;
    int8_t  speed;
    uint8_t active;
} FireBar;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;
    int8_t  vy;
    uint8_t kind;
    uint8_t timer;
} PopupObj;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;
    int8_t  vy;
    uint8_t kind;
    uint8_t rise_ticks;
} PowerItem;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;
    int8_t  vy;
    uint8_t active;
    uint8_t puff_timer;
} Fireball;

/* Flash-resident Sprite pointer table (0 RAM overhead) */
static const Sprite16 *const SPR_LIST[S_SPRITE_COUNT] = {
    &SPR_MARIO_STAND, &SPR_MARIO_RUN1, &SPR_MARIO_RUN2, &SPR_MARIO_JUMP,
    &SPR_GOOMBA_1, &SPR_GOOMBA_2, &SPR_GROUND, &SPR_BRICK,
    &SPR_QBLOCK, &SPR_USED, &SPR_PIPE_TL, &SPR_PIPE_TR,
    &SPR_PIPE_BL, &SPR_PIPE_BR, &SPR_COIN_1, &SPR_COIN_2,
    &SPR_FLAG_TOP, &SPR_FLAG_POLE, &SPR_BLOCK_STONE, &SPR_MUSHROOM,
    &SPR_FLOWER, &SPR_FIREBALL, &SPR_FIRE_PUFF,
    &SPR_PIRANHA1, &SPR_PIRANHA2, &SPR_KOOPA1, &SPR_KOOPA2,
    &SPR_SHELL, &SPR_PARATROOPA1, &SPR_PARATROOPA2, &SPR_BUZZY1,
    &SPR_BUZZY2, &SPR_SPINY1, &SPR_SPINY2, &SPR_LAKITU,
    &SPR_HAMMER_BRO1, &SPR_HAMMER_BRO2, &SPR_HAMMER, &SPR_BULLET,
    &SPR_CANNON_TOP, &SPR_CANNON_BOT, &SPR_CHEEP1, &SPR_CHEEP2,
    &SPR_BLOOBER1, &SPR_BLOOBER2, &SPR_BOWSER_TL, &SPR_BOWSER_TR,
    &SPR_BOWSER_BL, &SPR_BOWSER_BR, &SPR_BOWSER_FIRE, &SPR_TOAD,
    &SPR_PEACH, &SPR_STAR, &SPR_AXE, &SPR_CHAIN,
    &SPR_BRIDGE, &SPR_WATER_SURF, &SPR_WATER_FILL, &SPR_LAVA_SURF,
    &SPR_LAVA_FILL, &SPR_SPRING_UP, &SPR_SPRING_DN, &SPR_CLOUD_PLAT,
    &SPR_MUSH_L, &SPR_MUSH_M, &SPR_MUSH_R, &SPR_MUSH_STEM,
    &SPR_SIDE_PIPE_T, &SPR_SIDE_PIPE_B, &SPR_CASTLE_WALL, &SPR_CASTLE_BATT,
    &SPR_CASTLE_ARCH, &SPR_ROPE, &SPR_VINE, &SPR_PLAT, &SPR_GROUND_SUB
};

/* DMA ping-pong strips (kept in SRAM for 58+ FPS) */
static uint16_t  strip_buf[2][SCREEN_W * TILE_SIZE] __attribute__((aligned(4)));
static uint64_t  strip_hash[1 + MAP_ROWS];
static uint8_t   dirty_force_all = 1;

/* Compact ASCII-char RGB565 LUTs (~1.8 KB SRAM total vs 20 KB sprite_cache) */
static uint16_t  pal_theme[128];
static uint16_t  pal_red_swap[128];
static uint16_t  pal_1up[128];
static uint16_t  pal_slots[5][128];

static uint8_t   tile_map[MAP_ROWS][MAP_MAX_COLS];
static uint16_t  cur_map_cols = 218;
static uint8_t   cur_stage_idx = 0; /* 0..31 (World 1-1 .. 8-4) */
static uint8_t   cur_area_id = 0;   /* 0..33 */
static uint8_t   cur_theme = 1;     /* 0=water, 1=overworld, 2=underground, 3=castle */
static uint8_t   cur_night = 0;
static uint8_t   cur_snow = 0;
static uint8_t   cur_bg_scenery = 2;
static uint8_t   cur_in_sub = 0;
static uint8_t   cur_courtyard_exit = 0;
static uint8_t   sub_ret_area = 0;
static uint16_t  sub_ret_col = 0;
static uint16_t  spawn_cursor = 0;

static Player    player;
static Enemy     enemies[MAX_ENEMIES];
static Platform  platforms[MAX_PLATFORMS];
static FireBar   firebars[MAX_FIREBARS];
static PopupObj  popups[MAX_POPUPS];
static PowerItem items[MAX_ITEMS];
static Fireball  fireballs[MAX_FIREBALLS];
static int16_t   remote_disp_x[WIFI_MAX_PLAYERS];
static int16_t   remote_disp_y[WIFI_MAX_PLAYERS];

static GameState game_state = STATE_TITLE;
static uint8_t   title_world = 1;   /* 1..8 starting world selector */
static uint8_t   lives = 3;
static uint16_t  coins = 0;
static uint32_t  score = 0;
static uint16_t  time_left = 400;
static uint8_t   time_subtick = 0;
static uint8_t   combo_kills = 0;
static uint8_t   bridge_collapsing = 0;
static int16_t   bridge_col_cur = 0;
static int16_t   flag_slide_y = -1;
static int16_t   cam_x = 0;
static uint32_t  frame_count = 0;
static uint32_t  state_timer = 0;
/* audio_mode: 0 = Mute, 1 = Full SFX only, 2 = BGM + Key SFX (default) */
static uint8_t   audio_mode = 2;
static uint8_t   bgm_hw_active = 0;
static BgmTrack  bgm_hw_track = BGM_NONE;
static uint32_t  bgm_resume_pos[BGM_COUNT] = {0};
static uint32_t  beep_until_ms = 0;
static uint16_t  beep_seq_ms[6];
static uint8_t   beep_seq_len = 0;
static uint8_t   beep_seq_idx = 0;
static uint8_t   prev_keys = 0;
static uint8_t   fps_display = 60;
static uint32_t  fps_window_start_ms = 0;
static uint16_t  fps_window_frames = 0;

/* Track up to 4 active 10-coin bricks' remaining coin count */
typedef struct { uint16_t col; uint8_t row; uint8_t left; } CoinBrick;
static CoinBrick coin_bricks[6];

/* PC keyboard / SWD mirror globals */
static uint32_t  uart_hold_until[7] = {0};
static uint8_t   uart_state_mask = 0;
static uint32_t  uart_state_until = 0;
static uint8_t   uart_esc_state = 0;
volatile uint8_t g_swd_key_cmd = 0;
static uint8_t   swd_state_mask = 0;
static uint32_t  swd_state_until = 0;

typedef struct __attribute__((packed, aligned(4))) {
    uint32_t magic;
    uint32_t frame;
    uint8_t  state;
    uint8_t  level;       /* 1..32 */
    uint8_t  area_id;
    uint8_t  theme_flags; /* bits0..1=theme, bit2=night, bit3=snow, bits4..5=bg_scenery */
    uint8_t  lives;
    uint8_t  sound;
    uint16_t coins;
    uint16_t map_rev;
    uint16_t cur_cols;
    uint16_t time_left;
    uint16_t title_world;
    uint32_t score;
    int16_t  cam_x;
    int16_t  px;
    int16_t  py;
    uint8_t  p_spr;
    uint8_t  p_flags;
    uint8_t  fps;
    uint8_t  ev_row;      /* low 4 bits = row, bit4 = col MSB (col>>8) */
    uint8_t  ev_col;
    uint8_t  ev_val;
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t kind;
        uint8_t flags;    /* bit0:alive, bit1:dir, bit2:anim, bit3:shell_or_open */
    } enemies[MAX_ENEMIES];
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t kind;
        uint8_t active;
    } platforms[MAX_PLATFORMS];
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t angle;
        uint8_t len;
    } firebars[MAX_FIREBARS];
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t kind;
        uint8_t timer;
    } popups[MAX_POPUPS];
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t kind;
    } items[MAX_ITEMS];
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t active;
    } fireballs[MAX_FIREBALLS];
    uint8_t  net_role;
    uint8_t  net_stage;
    uint8_t  my_slot;
    uint8_t  net_count;
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t spr;
        uint8_t flags;
        uint8_t active;
        uint8_t slot;
    } peers[3];
} MirrorState;

volatile MirrorState g_mirror_state;
_Static_assert(sizeof(MirrorState) == 372, "MirrorState size mismatch");
static uint16_t mirror_map_rev = 0;
static uint8_t  mirror_ev_row = 0xFFU;
static uint8_t  mirror_ev_col = 0U;
static uint8_t  mirror_ev_val = 0U;

/* Minimal 5x7 font for HUD banners & overlays */
typedef struct {
    char ch;
    uint8_t cols[5];
} Font5x7;

static const Font5x7 FONT5X7[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}},
    {'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
    {'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
    {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
    {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}},
    {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
    {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
    {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
    {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
    {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
    {'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
    {'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
    {'W', {0x3F, 0x40, 0x38, 0x40, 0x3F}},
    {'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
    {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
    {'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},
    {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
    {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
    {'!', {0x00, 0x00, 0x5F, 0x00, 0x00}},
    {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {'/', {0x20, 0x10, 0x08, 0x04, 0x02}},
    {'*', {0x14, 0x08, 0x3E, 0x08, 0x14}},
    {'+', {0x08, 0x08, 0x3E, 0x08, 0x08}},
    {'<', {0x08, 0x14, 0x22, 0x41, 0x00}},
    {'>', {0x00, 0x41, 0x22, 0x14, 0x08}},
    {'[', {0x00, 0x7F, 0x41, 0x41, 0x00}},
    {']', {0x00, 0x41, 0x41, 0x7F, 0x00}},
};

/* 64-step sine LUT in [-127..127] for Fire Bar rotation */
static const int8_t SIN64[64] = {
       0,   12,   25,   37,   49,   60,   71,   81,
      90,   98,  106,  112,  117,  122,  125,  126,
     127,  126,  125,  122,  117,  112,  106,   98,
      90,   81,   71,   60,   49,   37,   25,   12,
       0,  -12,  -25,  -37,  -49,  -60,  -71,  -81,
     -90,  -98, -106, -112, -117, -122, -125, -126,
    -127, -126, -125, -122, -117, -112, -106,  -98,
     -90,  -81,  -71,  -60,  -49,  -37,  -25,  -12
};

static uint8_t is_solid_tile(int row, int col);
static void load_stage(uint8_t stage_idx);
static void load_area(uint8_t area_id, int16_t start_col, int16_t start_row, uint8_t is_sub);
static void kill_player(void);

static void build_palettes(void)
{
    for (int i = 0; i < 128; i++) {
        uint16_t c = sprite_palette((char)i);
        pal_theme[i] = c;
        pal_red_swap[i] = c;
        pal_1up[i] = c;
        for (int s = 0; s < 5; s++) {
            pal_slots[s][i] = sprite_palette_slot((char)i, (uint8_t)s);
        }
    }
    /* Red enemy variant (Red Koopa, Red Cheep-Cheep) */
    pal_red_swap['G'] = COL_RED;
    pal_red_swap['g'] = 0xFB20U;
    pal_red_swap['d'] = COL_DARKRED;
    /* 1-UP Green Mushroom variant */
    pal_1up['R'] = COL_PIPE_MD;
    pal_1up['r'] = COL_PIPE_DK;

    /* Theme overrides on pal_theme */
    if (cur_theme == 2U) {
        /* Underground */
        pal_theme['C'] = COL_UG_BRK;
        pal_theme['c'] = COL_UG_DK;
        pal_theme['D'] = COL_UG_BRK;
        pal_theme['G'] = COL_UG_DK;
        pal_theme['L'] = 0x5EFFU;
        pal_theme['A'] = COL_UG_BRK;
        pal_theme['a'] = COL_UG_DK;
    } else if (cur_theme == 3U) {
        /* Castle */
        pal_theme['C'] = COL_CAST_WL;
        pal_theme['c'] = COL_CAST_DK;
        pal_theme['D'] = COL_CAST_WL;
        pal_theme['G'] = COL_CAST_DK;
        pal_theme['L'] = 0xAD55U;
    } else if (cur_theme == 0U) {
        /* Underwater coral floor */
        pal_theme['D'] = 0x2EC6U;
        pal_theme['G'] = 0x2EC6U;
        pal_theme['L'] = 0x4FE6U;
        pal_theme['C'] = 0x04A2U;
        pal_theme['c'] = 0x0261U;
    }
}

static void fill_u32_strip(uint16_t *buf, uint16_t color)
{
    uint32_t c32 = ((uint32_t)color << 16) | color;
    uint32_t *p = (uint32_t *)buf;
    for (int i = 0; i < (SCREEN_W * TILE_SIZE) / 2; i++) {
        p[i] = c32;
    }
}

static uint64_t hash_strip(const uint16_t *buf)
{
    const uint32_t *p = (const uint32_t *)buf;
    uint64_t h = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < (SCREEN_W * TILE_SIZE) / 2; i++) {
        h ^= p[i] + 0x9E3779B9U + (h << 6) + (h >> 2);
    }
    return h;
}

static void start_beep_pattern(const uint16_t *seq, uint8_t count)
{
    if (audio_mode == 0U || count == 0U) {
        return;
    }
    if (count > 6U) count = 6U;
    for (uint8_t i = 0U; i < count; i++) {
        beep_seq_ms[i] = seq[i];
    }
    beep_seq_len = count;
    beep_seq_idx = 0U;
    beep_open();
    beep_until_ms = HAL_GetTick() + beep_seq_ms[0];
}

static void play_sfx_id(SfxId id)
{
    if (audio_mode == 0U) {
        return;
    }
    /* In BGM mode during active gameplay, keep micro-SFX (jump / fireball) from chopping BGM */
    if (audio_mode == 2U && game_state == STATE_PLAYING && (id == SFX_JUMP || id == SFX_FIREBALL)) {
        return;
    }
    const uint8_t *mp3 = MP3_JUMP;
    uint32_t len = sizeof(MP3_JUMP);
    uint8_t prio = 2U;
    uint32_t lock_ms = 120U;
    switch (id) {
        case SFX_INTRO: {
            mp3 = MP3_INTRO; len = sizeof(MP3_INTRO); prio = 1U; lock_ms = 2400U;
            const uint16_t p[] = {45U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 1U);
            break;
        }
        case SFX_JUMP: {
            mp3 = MP3_JUMP; len = sizeof(MP3_JUMP); prio = 2U; lock_ms = 120U;
            const uint16_t p[] = {20U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 1U);
            break;
        }
        case SFX_COIN: {
            mp3 = MP3_COIN; len = sizeof(MP3_COIN); prio = 3U; lock_ms = 170U;
            const uint16_t p[] = {22U, 16U, 26U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 3U);
            break;
        }
        case SFX_HIT: {
            mp3 = MP3_HIT; len = sizeof(MP3_HIT); prio = 4U; lock_ms = 130U;
            const uint16_t p[] = {22U, 18U, 16U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 3U);
            break;
        }
        case SFX_HURT: {
            mp3 = MP3_HURT; len = sizeof(MP3_HURT); prio = 5U; lock_ms = 320U;
            const uint16_t p[] = {55U, 30U, 55U, 30U, 80U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 5U);
            break;
        }
        case SFX_POWERUP: {
            mp3 = MP3_POWERUP; len = sizeof(MP3_POWERUP); prio = 5U; lock_ms = 380U;
            const uint16_t p[] = {25U, 18U, 25U, 18U, 48U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 5U);
            break;
        }
        case SFX_CLEAR: {
            mp3 = MP3_CLEAR; len = sizeof(MP3_CLEAR); prio = 6U; lock_ms = 550U;
            const uint16_t p[] = {45U, 25U, 45U, 25U, 90U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 5U);
            break;
        }
        case SFX_DIE: {
            mp3 = MP3_DIE; len = sizeof(MP3_DIE); prio = 7U; lock_ms = 480U;
            const uint16_t p[] = {120U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 1U);
            break;
        }
        case SFX_1UP: {
            mp3 = MP3_1UP; len = sizeof(MP3_1UP); prio = 6U; lock_ms = 380U;
            const uint16_t p[] = {25U, 15U, 25U, 15U, 50U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 5U);
            break;
        }
        case SFX_PIPE: {
            mp3 = MP3_PIPE; len = sizeof(MP3_PIPE); prio = 5U; lock_ms = 330U;
            const uint16_t p[] = {40U, 20U, 40U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 3U);
            break;
        }
        case SFX_FIREBALL: {
            mp3 = MP3_FIREBALL; len = sizeof(MP3_FIREBALL); prio = 2U; lock_ms = 70U;
            const uint16_t p[] = {12U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 1U);
            break;
        }
        case SFX_BOWSER: {
            mp3 = MP3_BOWSER; len = sizeof(MP3_BOWSER); prio = 6U; lock_ms = 450U;
            const uint16_t p[] = {80U, 30U, 80U}; if (!LD3320_Audio_IsReady()) start_beep_pattern(p, 3U);
            break;
        }
    }
    if (LD3320_Audio_IsReady()) {
        if (bgm_hw_active != 0U) {
            if (bgm_hw_track < BGM_COUNT) {
                bgm_resume_pos[bgm_hw_track] = LD3320_Audio_GetPos();
            }
            bgm_hw_active = 0U;
        }
        LD3320_Audio_Start(mp3, len, prio, lock_ms);
    }
}

static void bgm_get_track(BgmTrack tr, const uint8_t **out_ptr, uint32_t *out_len)
{
    switch (tr) {
        case BGM_OVERWORLD:
            *out_ptr = MP3_BGM_OVERWORLD;
            *out_len = sizeof(MP3_BGM_OVERWORLD);
            break;
        case BGM_UNDERGROUND:
            *out_ptr = MP3_BGM_UNDERGROUND;
            *out_len = sizeof(MP3_BGM_UNDERGROUND);
            break;
        case BGM_UNDERWATER:
            *out_ptr = MP3_BGM_UNDERWATER;
            *out_len = sizeof(MP3_BGM_UNDERWATER);
            break;
        case BGM_CASTLE:
            *out_ptr = MP3_BGM_CASTLE;
            *out_len = sizeof(MP3_BGM_CASTLE);
            break;
        case BGM_STAR:
            *out_ptr = MP3_BGM_STAR;
            *out_len = sizeof(MP3_BGM_STAR);
            break;
        default:
            *out_ptr = NULL;
            *out_len = 0U;
            break;
    }
}

static BgmTrack bgm_desired_track(void)
{
    if (!LD3320_Audio_IsReady() || audio_mode != 2U || game_state != STATE_PLAYING) {
        return BGM_NONE;
    }
    if (player.star_ticks > 0U) {
        return BGM_STAR;
    }
    if (cur_theme == 0U) return BGM_UNDERWATER;
    if (cur_theme == 2U) return BGM_UNDERGROUND;
    if (cur_theme == 3U) return BGM_CASTLE;
    return BGM_OVERWORLD;
}

static void update_bgm(void)
{
    BgmTrack want = bgm_desired_track();
    if (want == BGM_NONE) {
        if (bgm_hw_active != 0U) {
            if (bgm_hw_track < BGM_COUNT) {
                bgm_resume_pos[bgm_hw_track] = LD3320_Audio_GetPos();
            }
            LD3320_Audio_Stop();
            bgm_hw_active = 0U;
        }
        return;
    }

    if (bgm_hw_active != 0U) {
        if (!LD3320_Audio_IsPlaying()) {
            bgm_hw_active = 0U;
        } else if (bgm_hw_track != want) {
            if (bgm_hw_track < BGM_COUNT) {
                bgm_resume_pos[bgm_hw_track] = LD3320_Audio_GetPos();
            }
            const uint8_t *ptr = NULL;
            uint32_t len = 0U;
            bgm_get_track(want, &ptr, &len);
            if (ptr != NULL && len > 0U) {
                LD3320_Audio_StartLoop(ptr, len, bgm_resume_pos[want]);
                bgm_hw_track = want;
            }
            return;
        } else {
            return;
        }
    }

    /* Start / resume BGM once any active SFX finishes */
    if (!LD3320_Audio_IsPlaying()) {
        const uint8_t *ptr = NULL;
        uint32_t len = 0U;
        bgm_get_track(want, &ptr, &len);
        if (ptr != NULL && len > 0U) {
            LD3320_Audio_StartLoop(ptr, len, bgm_resume_pos[want]);
            bgm_hw_track = want;
            bgm_hw_active = 1U;
        }
    }
}

static void update_beep(void)
{
    if (beep_until_ms != 0U && HAL_GetTick() >= beep_until_ms) {
        beep_seq_idx++;
        if (beep_seq_idx < beep_seq_len) {
            if ((beep_seq_idx & 1U) == 0U) beep_open(); else beep_close();
            beep_until_ms = HAL_GetTick() + beep_seq_ms[beep_seq_idx];
        } else {
            beep_close();
            beep_until_ms = 0U;
            beep_seq_len = 0U;
        }
    }
}

static void update_life_leds(void)
{
    (lives >= 1) ? led_open(LED1_PORT) : led_close(LED1_PORT);
    (lives >= 2) ? led_open(LED2_PORT) : led_close(LED2_PORT);
    (lives >= 3) ? led_open(LED3_PORT) : led_close(LED3_PORT);
}

static uint16_t sky_color(void)
{
    if (cur_theme == 0U) return COL_UW_BG;
    if (cur_theme == 2U || cur_theme == 3U) return COL_BLACK;
    if (cur_night) return 0x0843U;
    if (cur_snow) return 0x447DU;
    return COL_SKY_DAY;
}

static const uint8_t *find_glyph(char ch)
{
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    for (size_t i = 0; i < sizeof(FONT5X7) / sizeof(FONT5X7[0]); i++) {
        if (FONT5X7[i].ch == ch) return FONT5X7[i].cols;
    }
    return FONT5X7[0].cols;
}

static void draw_text_strip(uint16_t *buf, int16_t x0, int16_t y_local, const char *str, uint16_t fg, uint8_t scale)
{
    int16_t cx = x0;
    while (*str) {
        const uint8_t *cols = find_glyph(*str++);
        for (int col = 0; col < 5; col++) {
            uint8_t bits = cols[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1U << row)) {
                    for (uint8_t sy = 0; sy < scale; sy++) {
                        int16_t py = (int16_t)(y_local + row * scale + sy);
                        if (py < 0 || py >= TILE_SIZE) continue;
                        for (uint8_t sx = 0; sx < scale; sx++) {
                            int16_t px = (int16_t)(cx + col * scale + sx);
                            if (px >= 0 && px < SCREEN_W) buf[py * SCREEN_W + px] = fg;
                        }
                    }
                }
            }
        }
        cx = (int16_t)(cx + 6 * scale);
    }
}

static void blit_sprite_pal(uint16_t *buf, int16_t sx, int16_t sy, int16_t strip_y,
                            uint8_t spr_id, uint8_t flip_x, const uint16_t *pal)
{
    if (sx <= -TILE_SIZE || sx >= SCREEN_W || spr_id >= S_SPRITE_COUNT) return;
    if (sy + TILE_SIZE <= strip_y || sy >= strip_y + TILE_SIZE) return;
    const Sprite16 *sp = SPR_LIST[spr_id];
    int c_start = (sx < 0) ? -sx : 0;
    int c_end = (sx + TILE_SIZE > SCREEN_W) ? (SCREEN_W - sx) : TILE_SIZE;
    for (int r = 0; r < TILE_SIZE; r++) {
        int wy = sy + r;
        if (wy < strip_y || wy >= strip_y + TILE_SIZE) continue;
        int dy = wy - strip_y;
        uint16_t *dst_row = &buf[dy * SCREEN_W];
        const char *src_row = sp->rows[r];
        for (int c = c_start; c < c_end; c++) {
            char ch = src_row[flip_x ? (TILE_SIZE - 1 - c) : c];
            if (ch != '.') {
                uint16_t color = pal[(uint8_t)ch];
                if (color != COL_TRANSP) {
                    dst_row[sx + c] = color;
                }
            }
        }
    }
}

/* Clip drawing vertically above clamp_bottom_y (e.g. Piranha Plant emerging from pipe mouth) */
static void blit_sprite_clipped_bot(uint16_t *buf, int16_t sx, int16_t sy, int16_t clamp_bot_y,
                                    int16_t strip_y, uint8_t spr_id, const uint16_t *pal)
{
    if (sx <= -TILE_SIZE || sx >= SCREEN_W || spr_id >= S_SPRITE_COUNT) return;
    if (sy + TILE_SIZE <= strip_y || sy >= strip_y + TILE_SIZE) return;
    const Sprite16 *sp = SPR_LIST[spr_id];
    int c_start = (sx < 0) ? -sx : 0;
    int c_end = (sx + TILE_SIZE > SCREEN_W) ? (SCREEN_W - sx) : TILE_SIZE;
    for (int r = 0; r < TILE_SIZE; r++) {
        int wy = sy + r;
        if (wy >= clamp_bot_y || wy < strip_y || wy >= strip_y + TILE_SIZE) continue;
        int dy = wy - strip_y;
        uint16_t *dst_row = &buf[dy * SCREEN_W];
        const char *src_row = sp->rows[r];
        for (int c = c_start; c < c_end; c++) {
            char ch = src_row[c];
            if (ch != '.') {
                dst_row[sx + c] = pal[(uint8_t)ch];
            }
        }
    }
}

#define TALL_H 24
static void blit_sprite_tall_pal(uint16_t *buf, int16_t sx, int16_t sy_top, int16_t strip_y,
                                 uint8_t spr_id, uint8_t flip_x, const uint16_t *pal)
{
    if (sx <= -TILE_SIZE || sx >= SCREEN_W || spr_id >= S_SPRITE_COUNT) return;
    if (sy_top + TALL_H <= strip_y || sy_top >= strip_y + TILE_SIZE) return;
    const Sprite16 *sp = SPR_LIST[spr_id];
    int c_start = (sx < 0) ? -sx : 0;
    int c_end = (sx + TILE_SIZE > SCREEN_W) ? (SCREEN_W - sx) : TILE_SIZE;
    for (int r = 0; r < TALL_H; r++) {
        int wy = sy_top + r;
        if (wy < strip_y || wy >= strip_y + TILE_SIZE) continue;
        int src_r = (r * TILE_SIZE) / TALL_H;
        int dy = wy - strip_y;
        uint16_t *dst_row = &buf[dy * SCREEN_W];
        const char *src_row = sp->rows[src_r];
        for (int c = c_start; c < c_end; c++) {
            char ch = src_row[flip_x ? (TILE_SIZE - 1 - c) : c];
            if (ch != '.') {
                dst_row[sx + c] = pal[(uint8_t)ch];
            }
        }
    }
}

static void spawn_popup(int16_t x, int16_t y, uint8_t kind)
{
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (popups[i].timer == 0U) {
            popups[i].x = x;
            popups[i].y = y;
            popups[i].vx = (kind == POP_BRICK_TL) ? -2 : ((kind == POP_BRICK_TR) ? 2 : 0);
            popups[i].vy = (kind == POP_BRICK_TL || kind == POP_BRICK_TR) ? -5 : ((kind == POP_COIN) ? -4 : -1);
            popups[i].kind = kind;
            popups[i].timer = (kind == POP_COIN) ? 18U : 28U;
            return;
        }
    }
}

static void spawn_brick_shatter(int col, int row)
{
    int16_t bx = (int16_t)(col * TILE_SIZE);
    int16_t by = (int16_t)(row * TILE_SIZE);
    spawn_popup(bx, by, POP_BRICK_TL);
    spawn_popup((int16_t)(bx + 8), by, POP_BRICK_TR);
}

static void spawn_item(int col, int row, uint8_t kind)
{
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (items[i].kind == ITEM_NONE) {
            items[i].x = (int16_t)(col * TILE_SIZE);
            items[i].y = (int16_t)(row * TILE_SIZE);
            items[i].vx = (kind == ITEM_MUSHROOM || kind == ITEM_1UP || kind == ITEM_STAR) ? 1 : 0;
            items[i].vy = (kind == ITEM_STAR) ? -3 : 0;
            items[i].kind = kind;
            items[i].rise_ticks = (kind == ITEM_VINE) ? 64U : 8U;
            return;
        }
    }
}

static uint8_t add_enemy_slot(int16_t col, int16_t row, uint8_t kind)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive && enemies[i].squash_timer == 0U) {
            enemies[i].x = (int16_t)(col * TILE_SIZE);
            enemies[i].y = (int16_t)(row * TILE_SIZE);
            if (kind == EN_PIRANHA) {
                enemies[i].x = (int16_t)(col * TILE_SIZE + 8);
            }
            enemies[i].base_x = enemies[i].x;
            enemies[i].base_y = enemies[i].y;
            enemies[i].kind = kind;
            enemies[i].alive = 1U;
            enemies[i].squash_timer = 0U;
            enemies[i].dir = 0U;
            enemies[i].vy = 0;
            enemies[i].timer = (uint8_t)(i * 11U);
            enemies[i].sub = (kind == EN_BOWSER) ? 5U : 0U;
            switch (kind) {
                case EN_PARATROOPA_F:
                    enemies[i].vx = 0;
                    enemies[i].vy = 1;
                    break;
                case EN_PARATROOPA_H:
                    enemies[i].vx = -1;
                    break;
                case EN_PIRANHA:
                case EN_TOAD_PEACH:
                    enemies[i].vx = 0;
                    enemies[i].sub = 0U; /* height popped above pipe (0..20) */
                    break;
                case EN_PODOBOO:
                    enemies[i].vx = 0;
                    enemies[i].y = (int16_t)(WORLD_H + 16);
                    enemies[i].base_y = enemies[i].y;
                    break;
                case EN_BULLET:
                    enemies[i].vx = -3;
                    break;
                case EN_BOWSER:
                    enemies[i].vx = -1;
                    enemies[i].y = (int16_t)(row * TILE_SIZE - 16);
                    break;
                case EN_CHEEP_GREY:
                case EN_CHEEP_RED:
                case EN_BLOOBER:
                    enemies[i].vx = -1;
                    break;
                default:
                    enemies[i].vx = -1;
                    break;
            }
            return 1U;
        }
    }
    return 0U;
}

static uint8_t spawn_fireball(int16_t x, int16_t y, int8_t dir)
{
    uint8_t active_cnt = 0U;
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        if (fireballs[i].active) active_cnt++;
    }
    if (active_cnt >= 2U) return 0U;
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        if (!fireballs[i].active && fireballs[i].puff_timer == 0U) {
            fireballs[i].x = (int16_t)(x + (dir > 0 ? 10 : -4));
            fireballs[i].y = (int16_t)(y + 2);
            fireballs[i].vx = (dir > 0) ? 4 : -4;
            fireballs[i].vy = 1;
            fireballs[i].active = 1U;
            fireballs[i].puff_timer = 0U;
            return 1U;
        }
    }
    return 0U;
}

static uint8_t visual_power_level(void)
{
    if (player.transform_ticks > 0U && ((player.transform_ticks >> 2) & 1U)) {
        return player.prev_power;
    }
    return player.power;
}

static uint8_t player_sprite_id_for_slot(uint8_t m_spr)
{
    return (m_spr < 4U) ? m_spr : S_MARIO_STAND;
}

static const uint16_t *player_pal_for_slot(uint8_t slot, uint8_t pwr)
{
    if (player.star_ticks > 0U && slot == 0U) {
        return pal_slots[(frame_count >> 1) & 3U];
    }
    if (pwr >= 2U) return pal_slots[4];
    if (slot < 4U) return pal_slots[slot];
    return pal_slots[0];
}

static void pump_scroll_spawns(void)
{
    const SmbAreaDesc *a = &SMB_AREAS[cur_area_id];
    int16_t right_col = (int16_t)((cam_x + SCREEN_W + 48) / TILE_SIZE);
    while (spawn_cursor < a->en_cnt) {
        const SmbEnemySpawn *es = &SMB_ENEMIES[a->en_off + spawn_cursor];
        if ((int16_t)es->col > right_col) break;
        add_enemy_slot((int16_t)es->col, (int16_t)es->row, es->kind);
        spawn_cursor++;
    }
}

static void load_area(uint8_t area_id, int16_t start_col, int16_t start_row, uint8_t is_sub)
{
    if (area_id >= SMB_AREA_COUNT) area_id = 0U;
    cur_area_id = area_id;
    cur_in_sub = is_sub;
    const SmbAreaDesc *a = &SMB_AREAS[area_id];
    cur_map_cols = a->width;
    if (cur_map_cols > MAP_MAX_COLS) cur_map_cols = MAP_MAX_COLS;
    cur_theme = a->area_type;
    cur_bg_scenery = a->bg_scenery;

    const SmbStageInfo *st = &SMB_STAGES[cur_stage_idx];
    cur_night = is_sub ? 0U : st->night;
    cur_snow = is_sub ? 0U : st->snow;
    build_palettes();

    memset(tile_map, T_EMPTY, sizeof(tile_map));
    memset(enemies, 0, sizeof(enemies));
    memset(platforms, 0, sizeof(platforms));
    memset(firebars, 0, sizeof(firebars));
    memset(popups, 0, sizeof(popups));
    memset(items, 0, sizeof(items));
    memset(fireballs, 0, sizeof(fireballs));
    memset(coin_bricks, 0, sizeof(coin_bricks));

    /* Decode RLE tile grid */
    uint32_t out_idx = 0U;
    uint32_t max_cells = (uint32_t)MAP_ROWS * (uint32_t)a->width;
    const uint8_t *rle = &SMB_AREA_RLE[a->rle_off];
    for (uint16_t i = 0U; i + 1U < a->rle_len && out_idx < max_cells; i = (uint16_t)(i + 2U)) {
        uint8_t cnt = rle[i];
        uint8_t val = rle[i + 1U];
        for (uint8_t k = 0U; k < cnt && out_idx < max_cells; k++) {
            uint16_t r = (uint16_t)(out_idx / a->width);
            uint16_t c = (uint16_t)(out_idx % a->width);
            if (r < MAP_ROWS && c < MAP_MAX_COLS) {
                tile_map[r][c] = val;
            }
            out_idx++;
        }
    }

    /* Decode specials (firebars & moving platforms) */
    uint8_t p_idx = 0U, f_idx = 0U;
    for (uint8_t i = 0U; i < a->sp_cnt; i++) {
        const SmbSpecialObj *sp = &SMB_SPECIALS[a->sp_off + i];
        if (sp->kind == SP_FIREBAR_SHORT || sp->kind == SP_FIREBAR_LONG) {
            if (f_idx < MAX_FIREBARS) {
                firebars[f_idx].x = (int16_t)(sp->col * TILE_SIZE + 8);
                firebars[f_idx].y = (int16_t)(sp->row * TILE_SIZE + 8);
                firebars[f_idx].len = (sp->kind == SP_FIREBAR_LONG) ? 10U : 6U;
                firebars[f_idx].angle = (uint8_t)(i * 32U);
                firebars[f_idx].speed = (sp->arg == 0x1CU || sp->arg == 0x1EU) ? -2 : 2;
                f_idx++;
            }
        } else {
            if (p_idx < MAX_PLATFORMS) {
                platforms[p_idx].x = (int16_t)(sp->col * TILE_SIZE);
                platforms[p_idx].y = (int16_t)(sp->row * TILE_SIZE);
                platforms[p_idx].base_x = platforms[p_idx].x;
                platforms[p_idx].base_y = platforms[p_idx].y;
                platforms[p_idx].kind = sp->kind;
                platforms[p_idx].active = 1U;
                platforms[p_idx].vx = (sp->kind == SP_PLAT_HORIZ) ? 1 : 0;
                platforms[p_idx].vy = (sp->kind == SP_PLAT_DOWN) ? 1 : ((sp->kind == SP_PLAT_UP) ? -1 : 1);
                platforms[p_idx].timer = 0U;
                p_idx++;
            }
        }
    }

    /* Reset player at start_col / start_row (auto-adjust upward if tile is solid) */
    const WifiStatus *ws = WiFi_Net_GetStatus();
    int col_off = (ws->role != WIFI_MODE_SOLO && !is_sub && start_col <= 4) ? (int)ws->my_slot : 0;
    int16_t spawn_c = (int16_t)(start_col + col_off);
    int16_t spawn_r = start_row;
    while (spawn_r > 2 && (is_solid_tile(spawn_r, spawn_c) || is_solid_tile(spawn_r - 1, spawn_c))) {
        spawn_r--;
    }
    player.x_fp = ((int32_t)spawn_c * TILE_SIZE) << 8;
    player.y_fp = ((int32_t)spawn_r * TILE_SIZE) << 8;
    player.vx_fp = 0;
    player.vy_fp = 0;
    player.dir = 1;
    player.on_ground = 0;
    player.invuln_ticks = 45;
    player.anim_tick = 0;
    player.transform_ticks = 0;
    player.fire_cd = 0;
    player.run_ticks = 0;
    player.crouch = 0;

    int16_t target_cam = (int16_t)((player.x_fp >> 8) - (SCREEN_W / 2 - TILE_SIZE / 2));
    if (target_cam < 0) target_cam = 0;
    int16_t max_cam = (int16_t)(cur_map_cols * TILE_SIZE - SCREEN_W);
    if (max_cam < 0) max_cam = 0;
    if (target_cam > max_cam) target_cam = max_cam;
    cam_x = target_cam;

    /* Advance spawn cursor past start area behind screen, then spawn screen-visible ones */
    spawn_cursor = 0U;
    while (spawn_cursor < a->en_cnt) {
        const SmbEnemySpawn *es = &SMB_ENEMIES[a->en_off + spawn_cursor];
        if ((int16_t)(es->col * TILE_SIZE) >= cam_x - 32) break;
        spawn_cursor++;
    }
    pump_scroll_spawns();

    bridge_collapsing = 0U;
    flag_slide_y = -1;
    dirty_force_all = 1U;
    mirror_ev_row = 0xFFU;
    mirror_ev_col = 0U;
    mirror_ev_val = 0U;
    mirror_map_rev++;
}

static void load_stage(uint8_t stage_idx)
{
    if (stage_idx >= SMB_STAGE_COUNT) stage_idx = 0U;
    cur_stage_idx = stage_idx;
    cur_courtyard_exit = 0U;
    memset(bgm_resume_pos, 0, sizeof(bgm_resume_pos));
    const SmbStageInfo *st = &SMB_STAGES[stage_idx];
    time_left = SMB_AREAS[st->area_id].timer;
    time_subtick = 0U;
    int16_t sr = (SMB_AREAS[st->area_id].area_type == 3U) ? 5 : 10;
    int16_t sc = (SMB_AREAS[st->area_id].area_type == 3U) ? 2 : 3;
    load_area(st->area_id, sc, sr, 0U);
    update_life_leds();
}

static uint8_t is_solid_tile(int row, int col)
{
    if (col < 0 || col >= (int)cur_map_cols) return 1U;
    if (row < 0) return 0U;
    if (row >= MAP_ROWS) return 0U;
    uint8_t t = tile_map[row][col];
    switch (t) {
        case T_GROUND:
        case T_BRICK:
        case T_QBLOCK:
        case T_USED:
        case T_PIPE_TL:
        case T_PIPE_TR:
        case T_PIPE_BL:
        case T_PIPE_BR:
        case T_PIPE_WARP_TL:
        case T_PIPE_WARP_TR:
        case T_STONE:
        case T_QBLOCK_MUSH:
        case T_QBLOCK_FLOWER:
        case T_BRICK_POWER:
        case T_BRICK_VINE:
        case T_BRICK_STAR:
        case T_BRICK_10COIN:
        case T_BRICK_1UP:
        case T_BRIDGE:
        case T_CANNON_TOP:
        case T_CANNON_MID:
        case T_CANNON_BOT:
        case T_SPRING_TOP:
        case T_SPRING_BOT:
        case T_CLOUD_PLAT:
        case T_MUSH_PLAT_L:
        case T_MUSH_PLAT_M:
        case T_MUSH_PLAT_R:
        case T_SIDE_PIPE_T:
        case T_SIDE_PIPE_B:
            return 1U;
        default:
            return 0U;
    }
}

static void sync_tile_change(int row, int col, uint8_t new_val)
{
    tile_map[row][col] = new_val;
    mirror_ev_row = (uint8_t)((row & 0x0F) | (((col >> 8) & 1) << 4));
    mirror_ev_col = (uint8_t)(col & 0xFF);
    mirror_ev_val = new_val;
    mirror_map_rev++;
    if (WiFi_Net_GetStatus()->role != WIFI_MODE_SOLO) {
        WiFi_Net_PushEvent(NET_EV_TILE, mirror_ev_row, mirror_ev_col, new_val);
    }
}

static void collect_coin(void)
{
    coins++;
    score += 200;
    if (coins >= 100U) {
        coins = 0U;
        if (lives < 99U) lives++;
        update_life_leds();
        play_sfx_id(SFX_1UP);
    } else {
        play_sfx_id(SFX_COIN);
    }
}

static void hit_block_from_below(int row, int col)
{
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= (int)cur_map_cols) return;
    uint8_t t = tile_map[row][col];

    if (t == T_QBLOCK || t == T_QBLOCK_HIDDEN_COIN) {
        sync_tile_change(row, col, T_USED);
        spawn_popup((int16_t)(col * TILE_SIZE), (int16_t)((row - 1) * TILE_SIZE), POP_COIN);
        collect_coin();
    } else if (t == T_QBLOCK_MUSH || t == T_BRICK_POWER) {
        sync_tile_change(row, col, T_USED);
        spawn_item(col, row, (player.power >= 1U) ? ITEM_FLOWER : ITEM_MUSHROOM);
        play_sfx_id(SFX_POWERUP);
    } else if (t == T_QBLOCK_FLOWER) {
        sync_tile_change(row, col, T_USED);
        spawn_item(col, row, ITEM_FLOWER);
        play_sfx_id(SFX_POWERUP);
    } else if (t == T_QBLOCK_HIDDEN_1UP || t == T_BRICK_1UP) {
        sync_tile_change(row, col, T_USED);
        spawn_item(col, row, ITEM_1UP);
        play_sfx_id(SFX_POWERUP);
    } else if (t == T_BRICK_STAR) {
        sync_tile_change(row, col, T_USED);
        spawn_item(col, row, ITEM_STAR);
        play_sfx_id(SFX_POWERUP);
    } else if (t == T_BRICK_VINE) {
        sync_tile_change(row, col, T_USED);
        spawn_item(col, row, ITEM_VINE);
        play_sfx_id(SFX_POWERUP);
    } else if (t == T_BRICK_10COIN) {
        int slot = -1;
        for (int i = 0; i < 6; i++) {
            if (coin_bricks[i].left > 0U && coin_bricks[i].col == (uint16_t)col && coin_bricks[i].row == (uint8_t)row) {
                slot = i; break;
            }
            if (slot < 0 && coin_bricks[i].left == 0U) slot = i;
        }
        if (slot >= 0) {
            if (coin_bricks[slot].left == 0U) {
                coin_bricks[slot].col = (uint16_t)col;
                coin_bricks[slot].row = (uint8_t)row;
                coin_bricks[slot].left = 10U;
            }
            coin_bricks[slot].left--;
            spawn_popup((int16_t)(col * TILE_SIZE), (int16_t)((row - 1) * TILE_SIZE), POP_COIN);
            collect_coin();
            if (coin_bricks[slot].left == 0U) {
                sync_tile_change(row, col, T_USED);
            }
        }
    } else if (t == T_BRICK) {
        if (player.power >= 1U) {
            sync_tile_change(row, col, T_EMPTY);
            spawn_brick_shatter(col, row);
            score += 50;
            play_sfx_id(SFX_HIT);
        } else {
            play_sfx_id(SFX_HIT);
        }
    }
}

static void trigger_stage_clear(uint16_t bonus)
{
    score += bonus;
    play_sfx_id(SFX_CLEAR);
    if (WiFi_Net_GetStatus()->role == WIFI_MODE_JOIN) {
        WiFi_Net_PushEvent(NET_EV_STAGE_CLEAR, 0U, 0U, 0U);
    }
    game_state = STATE_LEVEL_CLEAR;
    state_timer = 100U;
}

static uint8_t try_warp_trigger(uint8_t kind_mask, int16_t px, int16_t py)
{
    const SmbAreaDesc *a = &SMB_AREAS[cur_area_id];
    int16_t pc = (int16_t)((px + 8) / TILE_SIZE);
    int16_t pr_feet = (int16_t)((py + TILE_SIZE + 2) / TILE_SIZE);
    int16_t pr_mid = (int16_t)((py + 8) / TILE_SIZE);

    for (uint8_t i = 0U; i < a->wp_cnt; i++) {
        const SmbWarpDef *w = &SMB_WARPS[a->wp_off + i];
        if ((w->kind & kind_mask) == 0U) continue;
        uint8_t matched = 0U;
        if (w->kind == WARP_PIPE_DOWN || w->kind == WARP_TO_WORLD) {
            if (pc >= (int16_t)w->src_col - 1 && pc <= (int16_t)w->src_col + 2 &&
                pr_feet >= (int16_t)w->src_row && pr_feet <= (int16_t)w->src_row + 1) {
                matched = 1U;
            }
        } else if (w->kind == WARP_SIDE_PIPE) {
            if (pc >= (int16_t)w->src_col && pc <= (int16_t)w->src_col + 2 &&
                pr_mid >= (int16_t)w->src_row - 1 && pr_mid <= (int16_t)w->src_row + 2) {
                matched = 1U;
            }
        } else if (w->kind == WARP_VINE_TOP || w->kind == WARP_FALL_RETURN) {
            matched = 1U;
        }
        if (!matched) continue;

        play_sfx_id(SFX_PIPE);
        if (w->kind == WARP_TO_WORLD) {
            uint8_t dest_world = w->dest_area; /* 2..8 */
            if (dest_world < 1U) dest_world = 1U;
            if (dest_world > 8U) dest_world = 8U;
            cur_stage_idx = (uint8_t)((dest_world - 1U) * 4U);
            load_stage(cur_stage_idx);
            game_state = STATE_PRE_STAGE;
            state_timer = 75U;
            return 1U;
        }
        if (w->dest_area == SMB_WARP_RET_SENTINEL) {
            cur_courtyard_exit = 0U;
            load_area(sub_ret_area, (int16_t)sub_ret_col, (w->kind == WARP_FALL_RETURN) ? 0 : 9, 0U);
            return 1U;
        }
        if (w->dest_area == SMB_WARP_EXIT_COURTYARD) {
            cur_courtyard_exit = 1U;
            load_area(SMB_COURTYARD_AREA_ID, (int16_t)w->dest_col, (int16_t)w->dest_row, 0U);
            return 1U;
        }
        if (w->ret_col > 0U) {
            sub_ret_area = cur_area_id;
            sub_ret_col = w->ret_col;
        }
        load_area(w->dest_area, (int16_t)w->dest_col, (int16_t)w->dest_row, (w->dest_area != cur_area_id) ? 1U : cur_in_sub);
        return 1U;
    }
    return 0U;
}

static void check_overlaps(int16_t px, int16_t py, int16_t head_off)
{
    int c0 = (px + 2) / TILE_SIZE;
    int c1 = (px + TILE_SIZE - 3) / TILE_SIZE;
    int r0 = (py - head_off + 2) / TILE_SIZE;
    int r1 = (py + TILE_SIZE - 3) / TILE_SIZE;
    for (int r = r0; r <= r1; r++) {
        for (int c = c0; c <= c1; c++) {
            if (r < 0 || r >= MAP_ROWS || c < 0 || c >= (int)cur_map_cols) continue;
            uint8_t t = tile_map[r][c];
            if (t == T_COIN) {
                sync_tile_change(r, c, T_EMPTY);
                collect_coin();
            } else if ((t == T_FLAG_TOP || t == T_FLAG_POLE) && flag_slide_y < 0) {
                flag_slide_y = (int16_t)(r * TILE_SIZE);
                trigger_stage_clear((uint16_t)(100U * (uint16_t)(13 - r)));
                return;
            } else if (t == T_AXE && !bridge_collapsing) {
                sync_tile_change(r, c, T_EMPTY);
                bridge_collapsing = 1U;
                bridge_col_cur = (int16_t)(c - 1);
                play_sfx_id(SFX_BOWSER);
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].alive && enemies[i].kind == EN_BOWSER) {
                        enemies[i].vy = 2;
                        enemies[i].vx = 0;
                    }
                }
                if (WiFi_Net_GetStatus()->role != WIFI_MODE_SOLO) {
                    WiFi_Net_PushEvent(NET_EV_BOSS_AXE, (uint8_t)r, (uint8_t)(c & 0xFF), 0U);
                }
                trigger_stage_clear(5000U);
                return;
            } else if (t == T_LAVA_SURF || t == T_LAVA_FILL) {
                kill_player();
                return;
            } else if (t == T_SPRING_TOP && player.vy_fp > 0) {
                player.vy_fp = -1650;
                play_sfx_id(SFX_JUMP);
            }
        }
    }
}

static void hurt_player(void)
{
    player.prev_power = player.power;
    player.power = 0U;
    player.transform_ticks = 16U;
    player.invuln_ticks = 75U;
    player.vy_fp = -650;
    play_sfx_id(SFX_HURT);
}

static void kill_player(void)
{
    if (player.star_ticks > 0U && (player.y_fp >> 8) <= WORLD_H) return;
    player.power = 0U;
    player.star_ticks = 0U;
    player.transform_ticks = 0U;
    play_sfx_id(SFX_DIE);
    if (WiFi_Net_GetStatus()->role == WIFI_MODE_JOIN) {
        player.invuln_ticks = 60U;
        const SmbStageInfo *st = &SMB_STAGES[cur_stage_idx];
        load_area(st->area_id, 3, 10, 0U);
        return;
    }
    if (lives > 0U) lives--;
    update_life_leds();
    game_state = STATE_DEAD_WAIT;
    state_timer = 75U;
}

static void defeat_enemy(Enemy *e, int idx, uint8_t by_shell_or_fire)
{
    e->alive = 0U;
    e->squash_timer = by_shell_or_fire ? 0U : 12U;
    uint32_t pts = (e->kind == EN_BOWSER) ? 5000U : 200U;
    score += pts;
    combo_kills++;
    spawn_popup(e->x, (int16_t)(e->y - 8), (pts >= 1000U) ? POP_SCORE_1000 : POP_SCORE_200);
    play_sfx_id((e->kind == EN_BOWSER) ? SFX_BOWSER : SFX_HIT);
    if (WiFi_Net_GetStatus()->role != WIFI_MODE_SOLO) {
        WiFi_Net_PushEvent(NET_EV_ENEMY_KILL, (uint8_t)idx, 0U, 0U);
    }
}

static void update_gameplay(uint8_t keys, uint8_t pressed)
{
    /* Bridge collapse animation on Castle Axe */
    if (bridge_collapsing) {
        if ((frame_count & 1U) == 0U && bridge_col_cur >= 0) {
            if (tile_map[9][bridge_col_cur] == T_BRIDGE) {
                sync_tile_change(9, bridge_col_cur, T_EMPTY);
                bridge_col_cur--;
            } else {
                bridge_collapsing = 0U;
            }
        }
        return;
    }

    if (player.transform_ticks > 0U) {
        player.transform_ticks--;
        if (player.transform_ticks > 0U) return;
    }
    if (player.star_ticks > 0U) player.star_ticks--;

    /* Countdown game timer */
    if (++time_subtick >= 30U) {
        time_subtick = 0U;
        if (time_left > 0U) {
            time_left--;
            if (time_left == 0U) {
                kill_player();
                return;
            }
        }
    }

    /* Crouch (only Tall Mario on ground) */
    player.crouch = (player.power >= 1U && player.on_ground && (keys & KEY_DOWN)) ? 1U : 0U;

    /* Horizontal accel + Sprint speed (Shift on PC, or auto-sprint after 18 frames holding dir) */
    uint8_t moving = ((keys & (KEY1 | KEY2)) == KEY1) || ((keys & (KEY1 | KEY2)) == KEY2);
    if (moving && !player.crouch) {
        if (player.run_ticks < 30U) player.run_ticks++;
    } else if (!(keys & KEY_DASH)) {
        player.run_ticks = 0U;
    }
    int32_t max_vx = ((keys & KEY_DASH) || player.run_ticks >= 18U) ? 790 : 640;

    if ((keys & KEY1) && !(keys & KEY2) && !player.crouch) {
        player.vx_fp -= 115;
        if (player.vx_fp < -max_vx) player.vx_fp = -max_vx;
        player.dir = -1;
    } else if ((keys & KEY2) && !(keys & KEY1) && !player.crouch) {
        player.vx_fp += 115;
        if (player.vx_fp > max_vx) player.vx_fp = max_vx;
        player.dir = 1;
    } else {
        if (player.vx_fp > 85) player.vx_fp -= 85;
        else if (player.vx_fp < -85) player.vx_fp += 85;
        else player.vx_fp = 0;
    }

    /* Check vine climbing (touching active vine + holding Jump/Up) */
    int16_t px0 = (int16_t)(player.x_fp >> 8);
    int16_t py0 = (int16_t)(player.y_fp >> 8);
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (items[i].kind == ITEM_VINE && px0 + 12 > items[i].x && px0 + 4 < items[i].x + 16 &&
            py0 + 16 >= 0 && py0 <= items[i].y) {
            if (keys & KEY3) {
                player.vy_fp = -640;
                player.on_ground = 0U;
                if (py0 <= 8) {
                    if (try_warp_trigger(WARP_VINE_TOP, px0, py0)) return;
                }
            }
        }
    }

    /* Down warp pipe check (S/Down on PC, or K1+K2 tapped when standing on warp pipe) */
    uint8_t combo_down = ((keys & (KEY1 | KEY2)) == (KEY1 | KEY2)) && ((pressed & (KEY1 | KEY2)) != 0U);
    if (player.on_ground && ((pressed & KEY_DOWN) || combo_down)) {
        if (try_warp_trigger(WARP_PIPE_DOWN | WARP_TO_WORLD, px0, py0)) return;
    }

    /* Jump vs Underwater Swim */
    if (cur_theme == 0U) {
        /* Underwater physics: gentle gravity & infinite mid-water swim kicks */
        if (pressed & KEY3) {
            player.vy_fp = -680;
            player.on_ground = 0U;
            play_sfx_id(SFX_JUMP);
        }
        player.vy_fp += 18;
        if (player.vy_fp > 580) player.vy_fp = 580;
    } else {
        if ((pressed & KEY3) && player.on_ground) {
            player.vy_fp = -1240;
            player.on_ground = 0U;
            combo_kills = 0U;
            play_sfx_id(SFX_JUMP);
        }
        if (!(keys & KEY3) && player.vy_fp < -210) {
            player.vy_fp = -210;
        }
        player.vy_fp += (player.vy_fp < 0) ? 41 : 54;
        if (player.vy_fp > 1980) player.vy_fp = 1980;
    }

    if (player.fire_cd > 0U) player.fire_cd--;
    if (player.power >= 2U && player.fire_cd == 0U &&
        ((pressed & KEY_FIRE) || combo_down)) {
        if (spawn_fireball((int16_t)(player.x_fp >> 8), (int16_t)(player.y_fp >> 8), player.dir)) {
            player.fire_cd = 15U;
            play_sfx_id(SFX_FIREBALL);
        }
    }

    /* Horizontal move + tile collision */
    int16_t head_off = (player.power >= 1U && !player.crouch) ? 8 : 0;
    player.x_fp += player.vx_fp;
    int16_t px = (int16_t)(player.x_fp >> 8);
    int16_t py = (int16_t)(player.y_fp >> 8);
    if (px < cam_x) {
        if (px < 0) { px = 0; player.x_fp = 0; player.vx_fp = 0; }
    }
    if (px > (int16_t)(cur_map_cols * TILE_SIZE - TILE_SIZE)) {
        px = (int16_t)(cur_map_cols * TILE_SIZE - TILE_SIZE);
        player.x_fp = ((int32_t)px) << 8;
        player.vx_fp = 0;
    }

    int r_top = (py - head_off + 2) / TILE_SIZE;
    int r_mid = (py + 6) / TILE_SIZE;
    int r_bot = (py + TILE_SIZE - 2) / TILE_SIZE;
    if (player.vx_fp > 0) {
        int c_right = (px + TILE_SIZE - 2) / TILE_SIZE;
        if (is_solid_tile(r_top, c_right) || is_solid_tile(r_mid, c_right) || is_solid_tile(r_bot, c_right)) {
            /* Check sideways ExitPipe warp when walking right into pipe mouth */
            if (tile_map[r_mid][c_right] == T_SIDE_PIPE_T || tile_map[r_mid][c_right] == T_SIDE_PIPE_B ||
                tile_map[r_bot][c_right] == T_SIDE_PIPE_T || tile_map[r_bot][c_right] == T_SIDE_PIPE_B) {
                if (try_warp_trigger(WARP_SIDE_PIPE, (int16_t)(c_right * TILE_SIZE), py)) return;
            }
            px = (int16_t)(c_right * TILE_SIZE - (TILE_SIZE - 1));
            player.x_fp = ((int32_t)px) << 8;
            player.vx_fp = 0;
        }
    } else if (player.vx_fp < 0) {
        int c_left = (px + 1) / TILE_SIZE;
        if (is_solid_tile(r_top, c_left) || is_solid_tile(r_mid, c_left) || is_solid_tile(r_bot, c_left)) {
            px = (int16_t)((c_left + 1) * TILE_SIZE - 1);
            player.x_fp = ((int32_t)px) << 8;
            player.vx_fp = 0;
        }
    }

    /* Update moving platforms & check player standing on one */
    for (int i = 0; i < MAX_PLATFORMS; i++) {
        Platform *p = &platforms[i];
        if (!p->active) continue;
        if (p->kind == SP_PLAT_HORIZ) {
            p->x = (int16_t)(p->x + p->vx);
            if (p->x > p->base_x + 48 || p->x < p->base_x - 48) p->vx = (int8_t)(-p->vx);
        } else if (p->kind == SP_PLAT_VERT || p->kind == SP_PLAT_BAL_L) {
            p->timer++;
            if ((p->timer & 1U) == 0U) {
                p->y = (int16_t)(p->y + p->vy);
                if (p->y > p->base_y + 40 || p->y < p->base_y - 40) p->vy = (int8_t)(-p->vy);
            }
        } else if (p->kind == SP_PLAT_UP) {
            p->y--;
            if (p->y < 16) p->y = WORLD_H;
        } else if (p->kind == SP_PLAT_DOWN) {
            p->y++;
            if (p->y > WORLD_H) p->y = 16;
        }
    }

    /* Vertical move + tile & platform collision */
    int16_t prev_py = py;
    player.y_fp += player.vy_fp;
    py = (int16_t)(player.y_fp >> 8);
    if (py < head_off && cur_theme == 0U) {
        py = head_off;
        player.y_fp = ((int32_t)py) << 8;
        player.vy_fp = 0;
    }

    int c_l = (px + 3) / TILE_SIZE;
    int c_r = (px + TILE_SIZE - 4) / TILE_SIZE;
    if (player.vy_fp >= 0) {
        int r_foot = (py + TILE_SIZE) / TILE_SIZE;
        if (is_solid_tile(r_foot, c_l) || is_solid_tile(r_foot, c_r)) {
            py = (int16_t)(r_foot * TILE_SIZE - TILE_SIZE);
            player.y_fp = ((int32_t)py) << 8;
            player.vy_fp = 0;
            player.on_ground = 1U;
            combo_kills = 0U;
        } else {
            player.on_ground = 0U;
            /* Moving platforms landing check (48px wide x 8px thick) */
            for (int i = 0; i < MAX_PLATFORMS; i++) {
                Platform *p = &platforms[i];
                if (!p->active) continue;
                if (px + TILE_SIZE - 2 > p->x && px + 2 < p->x + 48 &&
                    prev_py + TILE_SIZE <= p->y + 4 && py + TILE_SIZE >= p->y) {
                    py = (int16_t)(p->y - TILE_SIZE);
                    player.y_fp = ((int32_t)py) << 8;
                    player.vy_fp = 0;
                    player.on_ground = 1U;
                    combo_kills = 0U;
                    if (p->kind == SP_PLAT_HORIZ) {
                        player.x_fp += ((int32_t)p->vx) << 8;
                        px = (int16_t)(player.x_fp >> 8);
                    } else if (p->kind == SP_PLAT_DROP) {
                        p->y = (int16_t)(p->y + 2);
                        if (p->y > WORLD_H) p->active = 0U;
                    }
                    break;
                }
            }
        }
    } else if (py - head_off >= 0) {
        int r_head = (py - head_off) / TILE_SIZE;
        if (is_solid_tile(r_head, c_l) || is_solid_tile(r_head, c_r) ||
            tile_map[r_head][c_l] == T_QBLOCK_HIDDEN_COIN || tile_map[r_head][c_l] == T_QBLOCK_HIDDEN_1UP ||
            tile_map[r_head][c_r] == T_QBLOCK_HIDDEN_COIN || tile_map[r_head][c_r] == T_QBLOCK_HIDDEN_1UP) {
            int hit_c = (is_solid_tile(r_head, c_l) || tile_map[r_head][c_l] == T_QBLOCK_HIDDEN_COIN || tile_map[r_head][c_l] == T_QBLOCK_HIDDEN_1UP) ? c_l : c_r;
            hit_block_from_below(r_head, hit_c);
            py = (int16_t)((r_head + 1) * TILE_SIZE + head_off);
            player.y_fp = ((int32_t)py) << 8;
            player.vy_fp = 0;
        }
    }

    if (py > WORLD_H) {
        if (try_warp_trigger(WARP_FALL_RETURN, px, py)) return;
        kill_player();
        return;
    }

    check_overlaps(px, py, head_off);
    if (game_state != STATE_PLAYING) return;

    if (player.invuln_ticks > 0U) player.invuln_ticks--;
    player.anim_tick = (player.vx_fp != 0) ? (uint8_t)(player.anim_tick + 1U) : 0U;

    /* Camera smooth follow (prevent backtracking past screen left if courtyard, else smooth follow) */
    int16_t target_cam = (int16_t)(px - (SCREEN_W / 2 - TILE_SIZE / 2));
    if (target_cam < 0) target_cam = 0;
    int16_t max_cam = (int16_t)(cur_map_cols * TILE_SIZE - SCREEN_W);
    if (max_cam < 0) max_cam = 0;
    if (target_cam > max_cam) target_cam = max_cam;
    if (target_cam > cam_x || cur_in_sub) {
        cam_x = target_cam; /* classic SMB forward-only scroll in main levels */
    }
    pump_scroll_spawns();

    /* Bullet Bill Cannon periodic spawn */
    if ((frame_count % 90U) == 0U) {
        int col_s = cam_x / TILE_SIZE;
        int col_e = (cam_x + SCREEN_W) / TILE_SIZE;
        for (int c = col_s; c <= col_e && c < (int)cur_map_cols; c++) {
            for (int r = 1; r < MAP_ROWS - 1; r++) {
                if (tile_map[r][c] == T_CANNON_TOP) {
                    int16_t cx = (int16_t)(c * TILE_SIZE);
                    if (cx - px > 24 || px - cx > 24) {
                        if (add_enemy_slot((int16_t)c, (int16_t)r, EN_BULLET)) {
                            enemies[0].x = enemies[0].x; /* adjust direction toward player */
                            for (int k = MAX_ENEMIES - 1; k >= 0; k--) {
                                if (enemies[k].alive && enemies[k].kind == EN_BULLET && enemies[k].timer < 2U) {
                                    enemies[k].vx = (cx > px) ? -3 : 3;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* FireBar rotation & collision */
    for (int i = 0; i < MAX_FIREBARS; i++) {
        FireBar *fb = &firebars[i];
        if (fb->len == 0U) continue;
        fb->angle = (uint8_t)(fb->angle + fb->speed);
        if (fb->x < cam_x - 64 || fb->x > cam_x + SCREEN_W + 64) continue;
        uint8_t idx = (uint8_t)((fb->angle >> 2) & 63U);
        int16_t cosv = SIN64[(idx + 16U) & 63U];
        int16_t sinv = SIN64[idx];
        for (uint8_t b = 1U; b < fb->len; b++) {
            int16_t fx = (int16_t)(fb->x + ((cosv * (int16_t)(b * 6)) >> 7));
            int16_t fy = (int16_t)(fb->y + ((sinv * (int16_t)(b * 6)) >> 7));
            if (player.star_ticks == 0U && player.invuln_ticks == 0U &&
                px + 12 > fx - 3 && px + 4 < fx + 3 && py + 14 > fy - 3 && (py - head_off + 2) < fy + 3) {
                if (player.power > 0U) hurt_player(); else { kill_player(); return; }
            }
        }
    }

    /* Update enemies */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (e->squash_timer > 0U) { e->squash_timer--; continue; }
        if (!e->alive) continue;
        if (e->x < cam_x - 80 || e->x > cam_x + SCREEN_W + 96) {
            if (e->x < cam_x - 80 && e->kind != EN_BOWSER && e->kind != EN_TOAD_PEACH) {
                e->alive = 0U; /* free slot once scrolled off left */
            }
            continue;
        }
        e->timer++;

        if (e->kind == EN_TOAD_PEACH) {
            continue;
        } else if (e->kind == EN_PIRANHA) {
            /* Piranha Plant pipe cycle: hidden (sub==0) -> rise(20px) -> wait -> sink */
            uint8_t cyc = (uint8_t)(e->timer & 127U);
            int16_t dist = (px + 8) - (e->base_x + 8);
            if (dist < 0) dist = (int16_t)(-dist);
            if (e->sub == 0U && dist < 24) {
                e->timer = 0U; /* Stay retracted when player stands beside/atop pipe */
            } else if (cyc < 32U) {
                e->sub = 0U;
            } else if (cyc < 56U) {
                if (e->sub < 22U) e->sub++;
            } else if (cyc < 96U) {
                e->sub = 22U;
            } else {
                if (e->sub > 0U) e->sub--;
            }
            e->y = (int16_t)(e->base_y - e->sub);
            if (e->sub > 4U) {
                if (px + 13 > e->x + 3 && px + 3 < e->x + 13 &&
                    py + 15 > e->y + 2 && (py - head_off + 2) < e->base_y) {
                    if (player.star_ticks > 0U) { defeat_enemy(e, i, 1U); continue; }
                    if (player.invuln_ticks == 0U) {
                        if (player.power > 0U) hurt_player(); else { kill_player(); return; }
                    }
                }
            }
            continue;
        } else if (e->kind == EN_PODOBOO) {
            uint8_t cyc = (uint8_t)(e->timer % 96U);
            if (cyc == 1U) {
                e->y = WORLD_H;
                e->vy = -9;
            } else if (e->y <= WORLD_H) {
                e->y = (int16_t)(e->y + e->vy);
                if ((e->timer & 1U) == 0U && e->vy < 7) e->vy++;
            }
            if (px + 12 > e->x + 2 && px + 4 < e->x + 14 &&
                py + 15 > e->y + 2 && (py - head_off + 2) < e->y + 14) {
                if (player.star_ticks == 0U && player.invuln_ticks == 0U) {
                    if (player.power > 0U) hurt_player(); else { kill_player(); return; }
                }
            }
            continue;
        } else if (e->kind == EN_BULLET || e->kind == EN_BOWSER_FIRE) {
            e->x = (int16_t)(e->x + e->vx);
        } else if (e->kind == EN_HAMMER) {
            e->x = (int16_t)(e->x + e->vx);
            e->y = (int16_t)(e->y + e->vy);
            if ((e->timer & 1U) == 0U && e->vy < 6) e->vy++;
            if (e->y > WORLD_H) { e->alive = 0U; continue; }
        } else if (e->kind == EN_LAKITU) {
            int16_t tgt = (int16_t)(px + 32);
            e->vx = (e->x < tgt - 8) ? 2 : ((e->x > tgt + 8) ? -2 : 0);
            e->x = (int16_t)(e->x + e->vx);
            e->y = (int16_t)(32 + (SIN64[(e->timer >> 1) & 63U] >> 5));
            if ((e->timer % 110U) == 0U) {
                add_enemy_slot((int16_t)(e->x / TILE_SIZE), (int16_t)(e->y / TILE_SIZE + 1), EN_SPINY);
            }
        } else if (e->kind == EN_BLOOBER) {
            if ((e->timer & 15U) < 6U) {
                e->y -= 2;
                e->x += (px > e->x) ? 1 : -1;
            } else if ((e->timer & 1U) == 0U && e->y < WORLD_H - 32) {
                e->y++;
            }
        } else if (e->kind == EN_CHEEP_GREY || e->kind == EN_CHEEP_RED) {
            e->x = (int16_t)(e->x + e->vx);
            if (cur_theme != 0U) {
                /* Leaping bridge Cheep-Cheep */
                e->y = (int16_t)(e->y + e->vy);
                if ((e->timer & 1U) == 0U && e->vy < 5) e->vy++;
                if (e->y > WORLD_H) { e->y = WORLD_H; e->vy = -7; }
            }
        } else if (e->kind == EN_PARATROOPA_F) {
            e->y = (int16_t)(e->base_y + (SIN64[e->timer & 63U] >> 2));
        } else if (e->kind == EN_BOWSER) {
            e->x = (int16_t)(e->x + e->vx);
            if (e->x > e->base_x + 48 || e->x < e->base_x - 64) e->vx = (int8_t)(-e->vx);
            e->vy += 1;
            int16_t ny = (int16_t)(e->y + e->vy);
            if (ny + 32 >= 9 * TILE_SIZE && ny + 32 <= 10 * TILE_SIZE &&
                is_solid_tile(9, (e->x + 16) / TILE_SIZE)) {
                e->y = (int16_t)(9 * TILE_SIZE - 32);
                e->vy = ((e->timer % 64U) == 0U) ? -6 : 0;
            } else {
                e->y = ny;
                if (e->y > WORLD_H) { defeat_enemy(e, i, 1U); continue; }
            }
            if ((e->timer % 95U) == 0U) {
                for (int s = 0; s < MAX_ENEMIES; s++) {
                    if (!enemies[s].alive && enemies[s].squash_timer == 0U) {
                        enemies[s].x = (int16_t)(e->x - 8);
                        enemies[s].y = (int16_t)(e->y + 8);
                        enemies[s].vx = -2;
                        enemies[s].kind = EN_BOWSER_FIRE;
                        enemies[s].alive = 1U;
                        break;
                    }
                }
            }
        } else {
            /* Ground/Shell/Paratroopa-J/HammerBro/Spiny/Buzzy/Goomba walking physics */
            if (e->kind == EN_HAMMER_BRO && (e->timer % 60U) == 0U) {
                e->vy = -5;
                add_enemy_slot((int16_t)(e->x / TILE_SIZE), (int16_t)(e->y / TILE_SIZE), EN_HAMMER);
                for (int k = MAX_ENEMIES - 1; k >= 0; k--) {
                    if (enemies[k].alive && enemies[k].kind == EN_HAMMER && enemies[k].timer < 2U) {
                        enemies[k].vx = (px < e->x) ? -2 : 2;
                        enemies[k].vy = -4;
                        break;
                    }
                }
            }
            int16_t nx = (int16_t)(e->x + e->vx);
            int er = e->y / TILE_SIZE;
            int ec = (e->vx > 0) ? ((nx + TILE_SIZE - 1) / TILE_SIZE) : (nx / TILE_SIZE);
            uint8_t stay_on_ledge = (e->kind == EN_KOOPA_R || e->kind == EN_HAMMER_BRO) ? 1U : 0U;
            if (nx <= 0 || is_solid_tile(er, ec) || (stay_on_ledge && !is_solid_tile(er + 1, ec))) {
                e->vx = (int8_t)(-e->vx);
                if (e->kind == EN_SHELL && e->vx != 0) play_sfx_id(SFX_HIT);
            } else {
                e->x = nx;
            }
            /* Vertical fall for walking/shell/hopping enemies */
            if (e->vy < 5) e->vy++;
            int16_t ny = (int16_t)(e->y + e->vy);
            int fr = (ny + TILE_SIZE) / TILE_SIZE;
            int fc_l = (e->x + 2) / TILE_SIZE;
            int fc_r = (e->x + TILE_SIZE - 3) / TILE_SIZE;
            if (e->vy >= 0 && (is_solid_tile(fr, fc_l) || is_solid_tile(fr, fc_r))) {
                e->y = (int16_t)(fr * TILE_SIZE - TILE_SIZE);
                e->vy = (e->kind == EN_PARATROOPA_J) ? -5 : 0;
            } else {
                e->y = ny;
                if (e->y > WORLD_H) { e->alive = 0U; continue; }
            }
        }

        /* Fast sliding shell hits other enemies */
        if (e->kind == EN_SHELL && e->vx != 0) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (i == j || !enemies[j].alive) continue;
                if (enemies[j].kind == EN_BOWSER || enemies[j].kind == EN_TOAD_PEACH) continue;
                if (e->x + 14 > enemies[j].x + 2 && e->x + 2 < enemies[j].x + 14 &&
                    e->y + 14 > enemies[j].y + 2 && e->y + 2 < enemies[j].y + 14) {
                    defeat_enemy(&enemies[j], j, 1U);
                }
            }
        }

        /* Player <-> Enemy collision */
        int16_t ew = (e->kind == EN_BOWSER) ? 32 : TILE_SIZE;
        int16_t eh = (e->kind == EN_BOWSER) ? 32 : TILE_SIZE;
        if (px + TILE_SIZE - 3 > e->x + 2 && px + 3 < e->x + ew - 2 &&
            py + TILE_SIZE - 1 > e->y + 2 && (py - head_off + 2) < e->y + eh) {
            if (player.star_ticks > 0U && e->kind != EN_BOWSER) {
                defeat_enemy(e, i, 1U);
                continue;
            }
            uint8_t can_stomp = (e->kind != EN_SPINY && e->kind != EN_BOWSER &&
                                 e->kind != EN_HAMMER && e->kind != EN_BOWSER_FIRE && cur_theme != 0U);
            if (can_stomp && player.vy_fp > 0 && (py + TILE_SIZE - (player.vy_fp >> 8)) <= e->y + 9) {
                player.vy_fp = -1050;
                if (e->kind == EN_PARATROOPA_J || e->kind == EN_PARATROOPA_F || e->kind == EN_PARATROOPA_H) {
                    e->kind = (e->kind == EN_PARATROOPA_F) ? EN_KOOPA_R : EN_KOOPA_G;
                    e->vy = 0;
                    if (e->vx == 0) e->vx = -1;
                    score += 400U;
                    play_sfx_id(SFX_HIT);
                } else if (e->kind == EN_KOOPA_G || e->kind == EN_KOOPA_R || e->kind == EN_BUZZY) {
                    e->sub = e->kind; /* remember shell visual */
                    e->kind = EN_SHELL;
                    e->vx = 0;
                    score += 100U;
                    play_sfx_id(SFX_HIT);
                } else if (e->kind == EN_SHELL) {
                    if (e->vx == 0) {
                        e->vx = (px + 8 < e->x + 8) ? 4 : -4;
                    } else {
                        e->vx = 0;
                    }
                    play_sfx_id(SFX_HIT);
                } else {
                    defeat_enemy(e, i, 0U);
                }
            } else if (e->kind == EN_SHELL && e->vx == 0) {
                e->vx = (px + 8 < e->x + 8) ? 4 : -4;
                player.invuln_ticks = 12U;
                play_sfx_id(SFX_HIT);
            } else if (player.invuln_ticks == 0U) {
                if (player.power > 0U) hurt_player();
                else { kill_player(); return; }
            }
        }
    }

    /* Update items */
    for (int i = 0; i < MAX_ITEMS; i++) {
        PowerItem *it = &items[i];
        if (it->kind == ITEM_NONE) continue;
        if (it->rise_ticks > 0U) {
            it->y = (int16_t)(it->y - 2);
            it->rise_ticks--;
        } else if (it->kind == ITEM_MUSHROOM || it->kind == ITEM_1UP || it->kind == ITEM_STAR) {
            int16_t nx = (int16_t)(it->x + it->vx);
            int mr = (it->y + 8) / TILE_SIZE;
            int mc = (it->vx > 0) ? ((nx + TILE_SIZE - 1) / TILE_SIZE) : (nx / TILE_SIZE);
            if (nx <= 0 || is_solid_tile(mr, mc)) {
                it->vx = (int8_t)(-it->vx);
            } else {
                it->x = nx;
            }
            if (it->vy < 5) it->vy++;
            int16_t ny = (int16_t)(it->y + it->vy);
            int fr = (ny + TILE_SIZE) / TILE_SIZE;
            int fc_l = (it->x + 2) / TILE_SIZE;
            int fc_r = (it->x + TILE_SIZE - 3) / TILE_SIZE;
            if (is_solid_tile(fr, fc_l) || is_solid_tile(fr, fc_r)) {
                it->y = (int16_t)(fr * TILE_SIZE - TILE_SIZE);
                it->vy = (it->kind == ITEM_STAR) ? -5 : 0;
            } else {
                it->y = ny;
                if (it->y > WORLD_H) { it->kind = ITEM_NONE; continue; }
            }
        }
        if (it->kind != ITEM_VINE &&
            px + TILE_SIZE - 2 > it->x + 1 && px + 2 < it->x + TILE_SIZE - 1 &&
            py + TILE_SIZE - 1 > it->y + 1 && (py - head_off + 1) < it->y + TILE_SIZE) {
            if (it->kind == ITEM_1UP) {
                if (lives < 99U) lives++;
                update_life_leds();
                spawn_popup(it->x, (int16_t)(it->y - 8), POP_SCORE_1UP);
                play_sfx_id(SFX_1UP);
            } else if (it->kind == ITEM_STAR) {
                player.star_ticks = 580U;
                score += 1000U;
                spawn_popup(it->x, (int16_t)(it->y - 8), POP_SCORE_1000);
                play_sfx_id(SFX_POWERUP);
            } else {
                player.prev_power = player.power;
                if (it->kind == ITEM_FLOWER) player.power = 2U;
                else if (player.power < 1U) player.power = 1U;
                player.transform_ticks = 22U;
                score += 1000U;
                spawn_popup(it->x, (int16_t)(it->y - 8), POP_SCORE_1000);
                play_sfx_id(SFX_POWERUP);
            }
            it->kind = ITEM_NONE;
        }
    }

    /* Update fireballs */
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        Fireball *f = &fireballs[i];
        if (f->puff_timer > 0U) { f->puff_timer--; continue; }
        if (!f->active) continue;
        int16_t fx = (int16_t)(f->x + f->vx);
        int fr = (f->y + 8) / TILE_SIZE;
        int fc = (f->vx > 0) ? ((fx + 12) / TILE_SIZE) : ((fx + 3) / TILE_SIZE);
        if (fx < cam_x - 16 || fx > cam_x + SCREEN_W + 16 || is_solid_tile(fr, fc)) {
            f->active = 0U; f->puff_timer = 5U; continue;
        }
        f->x = fx;
        if (f->vy < 5) f->vy++;
        int16_t fy = (int16_t)(f->y + f->vy);
        int fbot = (fy + 13) / TILE_SIZE;
        if (f->vy > 0 && is_solid_tile(fbot, (f->x + 8) / TILE_SIZE)) {
            f->y = (int16_t)(fbot * TILE_SIZE - 13);
            f->vy = -4;
        } else {
            f->y = fy;
            if (f->y > WORLD_H) { f->active = 0U; continue; }
        }
        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &enemies[j];
            if (!e->alive) continue;
            if (e->kind == EN_PODOBOO || e->kind == EN_TOAD_PEACH || e->kind == EN_BULLET ||
                e->kind == EN_HAMMER || e->kind == EN_BOWSER_FIRE) continue;
            int16_t ew = (e->kind == EN_BOWSER) ? 32 : 16;
            int16_t eh = (e->kind == EN_BOWSER) ? 32 : 16;
            if (f->x + 12 > e->x + 1 && f->x + 4 < e->x + ew - 1 &&
                f->y + 12 > e->y + 1 && f->y + 4 < e->y + eh - 1) {
                f->active = 0U;
                f->puff_timer = 5U;
                if (e->kind == EN_BUZZY) {
                    play_sfx_id(SFX_HIT); /* Buzzy Beetle is fireproof */
                } else if (e->kind == EN_BOWSER) {
                    if (e->sub > 1U) {
                        e->sub--;
                        play_sfx_id(SFX_HIT);
                    } else {
                        defeat_enemy(e, j, 1U);
                    }
                } else {
                    defeat_enemy(e, j, 1U);
                }
                break;
            }
        }
    }

    /* Update popups & brick particles */
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (popups[i].timer > 0U) {
            popups[i].x = (int16_t)(popups[i].x + popups[i].vx);
            popups[i].y = (int16_t)(popups[i].y + popups[i].vy);
            if ((popups[i].kind == POP_BRICK_TL || popups[i].kind == POP_BRICK_TR) && popups[i].vy < 6) {
                popups[i].vy++;
            } else if (popups[i].kind == POP_COIN && popups[i].timer == 9U) {
                popups[i].vy = 2;
            }
            popups[i].timer--;
        }
    }
}

static uint8_t tile_to_spr_id(uint8_t t)
{
    uint8_t anim = (uint8_t)((frame_count / 10U) & 1U);
    switch (t) {
        case T_GROUND:    return S_GROUND;
        case T_BRICK:
        case T_BRICK_POWER:
        case T_BRICK_VINE:
        case T_BRICK_STAR:
        case T_BRICK_10COIN:
        case T_BRICK_1UP: return S_BRICK;
        case T_QBLOCK:
        case T_QBLOCK_MUSH:
        case T_QBLOCK_FLOWER: return S_QBLOCK;
        case T_USED:      return S_USED;
        case T_PIPE_TL:
        case T_PIPE_WARP_TL: return S_PIPE_TL;
        case T_PIPE_TR:
        case T_PIPE_WARP_TR: return S_PIPE_TR;
        case T_PIPE_BL:   return S_PIPE_BL;
        case T_PIPE_BR:   return S_PIPE_BR;
        case T_COIN:      return anim ? S_COIN2 : S_COIN1;
        case T_FLAG_TOP:  return S_FLAG_TOP;
        case T_FLAG_POLE: return S_FLAG_POLE;
        case T_STONE:     return S_STONE;
        case T_BRIDGE:    return S_BRIDGE;
        case T_WATER_SURF:return S_WATER_SURF;
        case T_WATER_FILL:return S_WATER_FILL;
        case T_LAVA_SURF: return S_LAVA_SURF;
        case T_LAVA_FILL: return S_LAVA_FILL;
        case T_CANNON_TOP:return S_CANNON_TOP;
        case T_CANNON_MID:
        case T_CANNON_BOT:return S_CANNON_BOT;
        case T_AXE:       return S_AXE;
        case T_CHAIN:     return S_CHAIN;
        case T_SPRING_TOP:return S_SPRING_UP;
        case T_SPRING_BOT:return S_SPRING_DN;
        case T_CLOUD_PLAT:return S_CLOUD_PLAT;
        case T_MUSH_PLAT_L:return S_MUSH_L;
        case T_MUSH_PLAT_M:return S_MUSH_M;
        case T_MUSH_PLAT_R:return S_MUSH_R;
        case T_MUSH_STEM: return S_MUSH_STEM;
        case T_SIDE_PIPE_T:return S_SIDE_PIPE_T;
        case T_SIDE_PIPE_B:return S_SIDE_PIPE_B;
        case T_CASTLE_WALL:return S_CASTLE_WALL;
        case T_CASTLE_BATT:return S_CASTLE_BATT;
        case T_CASTLE_ARCH:return S_CASTLE_ARCH;
        case T_ROPE:      return S_ROPE;
        default:          return S_STONE;
    }
}

/* Crisp 4-Column Classic HUD inside HUD_H = 16 px */
static void render_hud_strip(uint16_t *buf)
{
    char txt[16];
    const WifiStatus *ws = WiFi_Net_GetStatus();
    const SmbStageInfo *st = &SMB_STAGES[cur_stage_idx];
    fill_u32_strip(buf, COL_HUD_BG);

    /* Col 1: Mini Hat + MARIO + 6-digit score */
    ui_draw_icon8(buf, 4, 4, ICON_HAT_8);
    ui_draw_text4x5(buf, 15, 2, "MARIO", COL_WHITE);
    snprintf(txt, sizeof(txt), "%06lu", (unsigned long)(score % 1000000UL));
    ui_draw_text4x5(buf, 15, 9, txt, COL_GOLD);

    /* Col 2: Animated coin icon + coins + active power pill */
    ui_draw_icon8(buf, 66, 4, ICON_COIN_8);
    snprintf(txt, sizeof(txt), "*%02u", coins % 100U);
    ui_draw_text4x5(buf, 76, 9, txt, COL_WHITE);
    const char *pwr_tag = (player.star_ticks > 0U) ? "STAR" :
                          ((player.power >= 2U) ? "FIRE" : ((player.power == 1U) ? "BIG " : ""));
    if (pwr_tag[0] != '\0') {
        ui_draw_text4x5(buf, 76, 2, pwr_tag, (player.star_ticks > 0U) ? COL_GOLD : 0x5EFFU);
    }

    /* Col 3: WORLD W-S + TIME */
    ui_draw_text4x5(buf, 126, 2, "WORLD", COL_WHITE);
    snprintf(txt, sizeof(txt), " %u-%u", st->world, st->stage);
    ui_draw_text4x5(buf, 126, 9, txt, COL_WHITE);

    ui_draw_text4x5(buf, 174, 2, "TIME", COL_WHITE);
    snprintf(txt, sizeof(txt), " %03u", time_left);
    ui_draw_text4x5(buf, 174, 9, txt, (time_left <= 100U) ? COL_RED : COL_WHITE);

    /* Col 4: Lives Heart + Mute icon + FPS/NET */
    ui_draw_icon8(buf, 224, 4, ICON_HEART_8);
    snprintf(txt, sizeof(txt), "*%u", lives);
    ui_draw_text4x5(buf, 234, 9, txt, COL_WHITE);

    ui_draw_icon8(buf, 260, 4, (audio_mode > 0U) ? ICON_SPK_ON_8 : ICON_SPK_OFF_8);
    if (audio_mode == 2U) {
        ui_draw_text4x5(buf, 269, 2, "B", COL_GOLD);
    } else if (audio_mode == 1U) {
        ui_draw_text4x5(buf, 269, 2, "S", 0x5EFFU);
    }
    if (ws->role == WIFI_MODE_SOLO) {
        snprintf(txt, sizeof(txt), "%2uFPS", fps_display);
        ui_draw_text4x5(buf, 274, 9, txt, 0xCE7FU);
    } else {
        snprintf(txt, sizeof(txt), "P%u/%u", (unsigned)(ws->my_slot + 1U), (unsigned)ws->player_count);
        ui_draw_text4x5(buf, 274, 2, txt, 0x5EFFU);
        snprintf(txt, sizeof(txt), "%2uF", fps_display);
        ui_draw_text4x5(buf, 274, 9, txt, 0xCE7FU);
    }
}

static void render_playfield_strip(uint16_t *buf, int16_t row_idx)
{
    int16_t world_strip_y = (int16_t)(row_idx * TILE_SIZE);

    if (game_state == STATE_PRE_STAGE || game_state == STATE_GAME_OVER) {
        fill_u32_strip(buf, COL_BLACK);
        if (game_state == STATE_PRE_STAGE && row_idx == 5) {
            char msg[24];
            const SmbStageInfo *st = &SMB_STAGES[cur_stage_idx];
            snprintf(msg, sizeof(msg), "WORLD  %u-%u", st->world, st->stage);
            draw_text_strip(buf, 100, 4, msg, COL_WHITE, 1);
        } else if (game_state == STATE_PRE_STAGE && row_idx == 7) {
            char msg[16];
            blit_sprite_pal(buf, 126, world_strip_y, world_strip_y, S_MARIO_STAND, 0U, pal_slots[0]);
            snprintf(msg, sizeof(msg), "*  %u", lives);
            draw_text_strip(buf, 150, 4, msg, COL_WHITE, 1);
        } else if (game_state == STATE_GAME_OVER && row_idx == 6) {
            draw_text_strip(buf, 96, 4, "GAME OVER", COL_RED, 2);
        } else if (game_state == STATE_GAME_OVER && row_idx == 8) {
            draw_text_strip(buf, 68, 4, "PRESS K3 TO CONTINUE", COL_WHITE, 1);
        }
        return;
    }

    ui_draw_scenery_band(buf, row_idx, cam_x, cur_theme, cur_night, cur_snow, cur_bg_scenery, sky_color());

    int col_start = cam_x / TILE_SIZE;
    int col_end = (cam_x + SCREEN_W) / TILE_SIZE + 1;
    if (col_end >= (int)cur_map_cols) col_end = (int)cur_map_cols - 1;

    /* Piranha Plants drawn BEFORE pipes so pipe lip naturally occludes retracted stem! */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (!e->alive || e->kind != EN_PIRANHA || e->sub == 0U) continue;
        uint8_t pspr = ((frame_count / 8U) & 1U) ? S_PIRANHA1 : S_PIRANHA2;
        blit_sprite_clipped_bot(buf, (int16_t)(e->x - cam_x), e->y, e->base_y, world_strip_y, pspr, pal_theme);
    }

    /* Draw map tiles */
    for (int c = col_start; c <= col_end; c++) {
        uint8_t t = tile_map[row_idx][c];
        if (t == T_EMPTY || t == T_QBLOCK_HIDDEN_COIN || t == T_QBLOCK_HIDDEN_1UP) continue;
        uint8_t sid = (t == T_GROUND && row_idx > 0 && tile_map[row_idx - 1][c] == T_GROUND) ? S_GROUND_SUB : tile_to_spr_id(t);
        blit_sprite_pal(buf, (int16_t)(c * TILE_SIZE - cam_x), world_strip_y, world_strip_y,
                        sid, 0U, pal_theme);
    }

    /* Moving Platforms */
    for (int i = 0; i < MAX_PLATFORMS; i++) {
        if (!platforms[i].active) continue;
        int16_t px_s = (int16_t)(platforms[i].x - cam_x);
        for (int k = 0; k < 3; k++) {
            blit_sprite_pal(buf, (int16_t)(px_s + k * 16), platforms[i].y, world_strip_y, S_PLAT, 0U, pal_theme);
        }
    }

    /* FireBars */
    for (int i = 0; i < MAX_FIREBARS; i++) {
        FireBar *fb = &firebars[i];
        if (fb->len == 0U || fb->x < cam_x - 64 || fb->x > cam_x + SCREEN_W + 64) continue;
        uint8_t idx = (uint8_t)((fb->angle >> 2) & 63U);
        int16_t cosv = SIN64[(idx + 16U) & 63U];
        int16_t sinv = SIN64[idx];
        for (uint8_t b = 0U; b < fb->len; b++) {
            int16_t fx = (int16_t)(fb->x + ((cosv * (int16_t)(b * 6)) >> 7) - 8 - cam_x);
            int16_t fy = (int16_t)(fb->y + ((sinv * (int16_t)(b * 6)) >> 7) - 8);
            blit_sprite_pal(buf, fx, fy, world_strip_y, S_FIREBALL, ((frame_count / 3U) & 1U), pal_theme);
        }
    }

    /* Popups / brick shatter / floating score text */
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (popups[i].timer == 0U) continue;
        int16_t sx = (int16_t)(popups[i].x - cam_x);
        if (popups[i].kind == POP_COIN) {
            blit_sprite_pal(buf, sx, popups[i].y, world_strip_y, (popups[i].timer & 2U) ? S_COIN2 : S_COIN1, 0U, pal_theme);
        } else if (popups[i].kind == POP_BRICK_TL || popups[i].kind == POP_BRICK_TR) {
            blit_sprite_pal(buf, sx, popups[i].y, world_strip_y, S_FIRE_PUFF, 0U, pal_theme);
        } else if (popups[i].y >= world_strip_y && popups[i].y < world_strip_y + TILE_SIZE) {
            const char *ptxt = (popups[i].kind == POP_SCORE_1UP) ? "1UP" :
                               ((popups[i].kind == POP_SCORE_1000) ? "1000" : "200");
            ui_draw_text4x5(buf, sx, (int16_t)(popups[i].y - world_strip_y + 4), ptxt, COL_WHITE);
        }
    }

    /* Items & Vine */
    for (int i = 0; i < MAX_ITEMS; i++) {
        PowerItem *it = &items[i];
        if (it->kind == ITEM_NONE) continue;
        int16_t sx = (int16_t)(it->x - cam_x);
        if (it->kind == ITEM_VINE) {
            for (int vy = it->y; vy >= 0; vy -= 16) {
                blit_sprite_pal(buf, sx, (int16_t)vy, world_strip_y, S_VINE, 0U, pal_theme);
            }
        } else {
            uint8_t spr = (it->kind == ITEM_FLOWER) ? S_FLOWER : ((it->kind == ITEM_STAR) ? S_STAR : S_MUSHROOM);
            const uint16_t *pal = (it->kind == ITEM_1UP) ? pal_1up : pal_theme;
            blit_sprite_pal(buf, sx, it->y, world_strip_y, spr, 0U, pal);
        }
    }

    /* Enemies */
    uint8_t anim2 = (uint8_t)((frame_count / 10U) & 1U);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (!e->alive || e->kind == EN_PIRANHA) continue;
        int16_t sx = (int16_t)(e->x - cam_x);
        uint8_t fx = (e->vx > 0) ? 1U : 0U;
        const uint16_t *epal = pal_theme;
        uint8_t espr = S_GOOMBA1;
        switch (e->kind) {
            case EN_GOOMBA: espr = anim2 ? S_GOOMBA2 : S_GOOMBA1; break;
            case EN_KOOPA_R: epal = pal_red_swap; /* fallthrough */
            case EN_KOOPA_G: espr = anim2 ? S_KOOPA2 : S_KOOPA1; break;
            case EN_SHELL:
                espr = S_SHELL;
                epal = (e->sub == EN_KOOPA_R) ? pal_red_swap : pal_theme;
                break;
            case EN_PARATROOPA_F: epal = pal_red_swap; /* fallthrough */
            case EN_PARATROOPA_J:
            case EN_PARATROOPA_H: espr = anim2 ? S_PARATROOPA2 : S_PARATROOPA1; break;
            case EN_BUZZY:  espr = anim2 ? S_BUZZY2 : S_BUZZY1; break;
            case EN_SPINY:  espr = anim2 ? S_SPINY2 : S_SPINY1; break;
            case EN_LAKITU: espr = S_LAKITU; break;
            case EN_HAMMER_BRO: espr = anim2 ? S_HAMMER_BRO2 : S_HAMMER_BRO1; break;
            case EN_HAMMER: espr = S_HAMMER; break;
            case EN_BULLET: espr = S_BULLET; break;
            case EN_CHEEP_RED: epal = pal_red_swap; /* fallthrough */
            case EN_CHEEP_GREY: espr = anim2 ? S_CHEEP2 : S_CHEEP1; break;
            case EN_BLOOBER: espr = anim2 ? S_BLOOBER2 : S_BLOOBER1; break;
            case EN_PODOBOO: espr = S_FIREBALL; break;
            case EN_BOWSER_FIRE: espr = S_BOWSER_FIRE; break;
            case EN_TOAD_PEACH: espr = (cur_stage_idx == 31U) ? S_PEACH : S_TOAD; break;
            case EN_BOWSER:
                blit_sprite_pal(buf, sx, e->y, world_strip_y, S_BOWSER_TL, 0U, pal_theme);
                blit_sprite_pal(buf, (int16_t)(sx + 16), e->y, world_strip_y, S_BOWSER_TR, 0U, pal_theme);
                blit_sprite_pal(buf, sx, (int16_t)(e->y + 16), world_strip_y, S_BOWSER_BL, 0U, pal_theme);
                blit_sprite_pal(buf, (int16_t)(sx + 16), (int16_t)(e->y + 16), world_strip_y, S_BOWSER_BR, 0U, pal_theme);
                continue;
            default: break;
        }
        blit_sprite_pal(buf, sx, e->y, world_strip_y, espr, fx, epal);
    }

    /* Fireballs */
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        if (fireballs[i].puff_timer > 0U) {
            blit_sprite_pal(buf, (int16_t)(fireballs[i].x - cam_x), fireballs[i].y, world_strip_y,
                            S_FIRE_PUFF, ((frame_count / 2U) & 1U), pal_theme);
        } else if (fireballs[i].active) {
            blit_sprite_pal(buf, (int16_t)(fireballs[i].x - cam_x), fireballs[i].y, world_strip_y,
                            S_FIREBALL, ((frame_count / 3U) & 1U), pal_theme);
        }
    }

    /* Remote teammates */
    const WifiStatus *ws = WiFi_Net_GetStatus();
    if (ws->role != WIFI_MODE_SOLO && game_state != STATE_TITLE && game_state != STATE_LOBBY) {
        for (uint8_t s = 0U; s < WIFI_MAX_PLAYERS; s++) {
            if (s == ws->my_slot || !ws->players[s].active) continue;
            if ((ws->players[s].flags & 0x02U) != 0U) {
                uint8_t r_pwr = (uint8_t)((ws->players[s].flags >> 3) & 0x03U);
                const uint16_t *rpal = (r_pwr >= 2U) ? pal_slots[4] : pal_slots[s];
                uint8_t rspr = player_sprite_id_for_slot(ws->players[s].spr);
                if (r_pwr >= 1U) {
                    blit_sprite_tall_pal(buf, (int16_t)(remote_disp_x[s] - cam_x),
                                         (int16_t)(remote_disp_y[s] - 8), world_strip_y,
                                         rspr, (ws->players[s].flags & 0x01U) ? 1U : 0U, rpal);
                } else {
                    blit_sprite_pal(buf, (int16_t)(remote_disp_x[s] - cam_x),
                                    remote_disp_y[s], world_strip_y,
                                    rspr, (ws->players[s].flags & 0x01U) ? 1U : 0U, rpal);
                }
            }
        }
    }

    /* Local player */
    if (game_state != STATE_LOBBY &&
        (player.transform_ticks > 0U || player.star_ticks > 0U || player.invuln_ticks == 0 || (player.invuln_ticks & 2U))) {
        uint8_t m_spr = S_MARIO_STAND;
        if (!player.on_ground) m_spr = S_MARIO_JUMP;
        else if (player.vx_fp != 0) m_spr = ((player.anim_tick / 4U) & 1U) ? S_MARIO_RUN1 : S_MARIO_RUN2;
        uint8_t vis_pwr = visual_power_level();
        uint8_t slot = (ws->role != WIFI_MODE_SOLO) ? ws->my_slot : 0U;
        const uint16_t *ppal = player_pal_for_slot(slot, vis_pwr);
        int16_t px_s = (int16_t)((player.x_fp >> 8) - cam_x);
        int16_t py_s = (int16_t)(player.y_fp >> 8);
        if (vis_pwr >= 1U && !player.crouch) {
            blit_sprite_tall_pal(buf, px_s, (int16_t)(py_s - 8), world_strip_y, m_spr, (player.dir < 0), ppal);
        } else {
            blit_sprite_pal(buf, px_s, py_s, world_strip_y, m_spr, (player.dir < 0), ppal);
        }
    }

    /* Classic Title Plaque & Menus */
    if (game_state == STATE_TITLE) {
        if (row_idx >= 1 && row_idx <= 3) {
            int16_t x0 = 42, x1 = 278;
            for (int r = 0; r < TILE_SIZE; r++) {
                if ((row_idx == 1 && r < 4) || (row_idx == 3 && r > 11)) continue;
                for (int16_t x = x0; x < x1; x++) {
                    uint8_t edge = (x <= x0 + 2 || x >= x1 - 3 || (row_idx == 1 && r < 6) || (row_idx == 3 && r > 9));
                    buf[r * SCREEN_W + x] = edge ? COL_GOLD : 0xB000U;
                }
            }
            if (row_idx == 2) {
                draw_text_strip(buf, 54, 1, "SUPER MARIO BROS", COL_GOLD, 2);
            }
        } else if (row_idx == 5) {
            char wt[32];
            snprintf(wt, sizeof(wt), "WORLD %u-1  [S:NEXT W]", title_world);
            draw_text_strip(buf, 92, 4, wt, COL_WHITE, 1);
        } else if (row_idx == 7) {
            draw_text_strip(buf, 22, 4, "K3:SOLO  K1:HOST  K2:JOIN  K4:SND", COL_GOLD, 1);
        } else if (row_idx == 9 && ((frame_count / 18U) & 1U)) {
            draw_text_strip(buf, 62, 4, "32 LEVELS FULL EDITION", COL_WHITE, 1);
        }
    } else if (game_state == STATE_LOBBY) {
        if (row_idx == 2) draw_text_strip(buf, 44, 2, "WIFI LOBBY [MARIO-MP]", COL_GOLD, 1);
        else if (row_idx == 4) draw_text_strip(buf, 44, 4, (ws->role == WIFI_MODE_HOST) ? "ROLE: HOST [1P RED]" : "ROLE: JOIN [2-4P]", COL_WHITE, 1);
        else if (row_idx == 6) draw_text_strip(buf, 44, 4, WiFi_Net_StageText(), COL_GOLD, 1);
        else if (row_idx == 8) {
            char pbuf[40];
            snprintf(pbuf, sizeof(pbuf), "PLAYERS ONLINE: %u / 4", (unsigned)ws->player_count);
            draw_text_strip(buf, 44, 4, pbuf, COL_WHITE, 1);
        } else if (row_idx == 10) draw_text_strip(buf, 44, 4, (ws->role == WIFI_MODE_HOST) ? "K3:START  K4:CANCEL" : "WAIT HOST  K4:CANCEL", COL_GOLD, 1);
    } else if (game_state == STATE_LEVEL_CLEAR) {
        if (cur_stage_idx % 4U == 3U) {
            if (row_idx == 4) draw_text_strip(buf, 52, 4, "THANK YOU MARIO!", COL_WHITE, 1);
            if (row_idx == 6) draw_text_strip(buf, 36, 4, (cur_stage_idx == 31U) ? "YOUR QUEST IS OVER!" : "PRINCESS IN OTHER CASTLE!", COL_GOLD, 1);
        } else if (row_idx == 5) {
            draw_text_strip(buf, 64, 1, "COURSE CLEAR!", COL_GOLD, 2);
        }
    } else if (game_state == STATE_WIN && row_idx == 4) {
        draw_text_strip(buf, 58, 1, "CONGRATULATIONS!", COL_GOLD, 1);
    } else if (game_state == STATE_WIN && row_idx == 6) {
        draw_text_strip(buf, 60, 4, "ALL 32 STAGES CLEARED!", COL_WHITE, 1);
    } else if (game_state == STATE_WIN && row_idx == 8) {
        draw_text_strip(buf, 56, 4, "PRESS K3 OR K4 TO REPLAY", COL_GOLD, 1);
    }
}

static void render_frame(void)
{
    uint8_t cur_buf = 0U;
    render_hud_strip(strip_buf[cur_buf]);
    uint64_t hh = hash_strip(strip_buf[cur_buf]);
    if (dirty_force_all || hh != strip_hash[0]) {
        strip_hash[0] = hh;
        LCD_BlitRGB565_AsyncStart(0, 0, SCREEN_W, HUD_H, strip_buf[cur_buf]);
        cur_buf ^= 1U;
    }
    for (int16_t r = 0; r < MAP_ROWS; r++) {
        render_playfield_strip(strip_buf[cur_buf], r);
        uint64_t h = hash_strip(strip_buf[cur_buf]);
        if (dirty_force_all || h != strip_hash[1 + r]) {
            strip_hash[1 + r] = h;
            LCD_BlitRGB565_AsyncStart(0, (uint16_t)(HUD_H + r * TILE_SIZE), SCREEN_W, TILE_SIZE, strip_buf[cur_buf]);
            cur_buf ^= 1U;
        }
    }
    LCD_BlitRGB565_Wait();
    dirty_force_all = 0U;
}

void Mario_Game_Init(void)
{
    LCD_direction(3);
    LCD_DMA_Init();
    LCD_FillRect(0, 0, SCREEN_W, SCREEN_H, COL_SKY_DAY);

    uint8_t ld_ok = LD3320_Audio_Init();
    lives = 3U; coins = 0U; score = 0U;
    title_world = 1U;
    game_state = STATE_TITLE;
    load_stage(0U);
    dirty_force_all = 1U;
    fps_window_start_ms = HAL_GetTick();
    fps_window_frames = 0U;

    printf("\r\nMARIO_INIT_OK 32STAGES LD3320=%u\r\n", ld_ok);
    play_sfx_id(SFX_INTRO);
}

static void update_mirror_state(void)
{
    const WifiStatus *ws = WiFi_Net_GetStatus();
    g_mirror_state.magic = 0x4D415232UL; /* 'MAR2' */
    g_mirror_state.frame = frame_count;
    g_mirror_state.state = (uint8_t)game_state;
    g_mirror_state.level = (uint8_t)(cur_stage_idx + 1U);
    g_mirror_state.area_id = cur_area_id;
    g_mirror_state.theme_flags = (uint8_t)((cur_theme & 3U) | ((cur_night & 1U) << 2) |
                                           ((cur_snow & 1U) << 3) | ((cur_bg_scenery & 3U) << 4));
    g_mirror_state.lives = lives;
    g_mirror_state.sound = audio_mode;
    g_mirror_state.coins = coins;
    g_mirror_state.map_rev = mirror_map_rev;
    g_mirror_state.cur_cols = cur_map_cols;
    g_mirror_state.time_left = time_left;
    g_mirror_state.title_world = title_world;
    g_mirror_state.score = score;
    g_mirror_state.cam_x = cam_x;
    g_mirror_state.px = (int16_t)(player.x_fp >> 8);
    g_mirror_state.py = (int16_t)(player.y_fp >> 8);
    uint8_t m_spr = S_MARIO_STAND;
    if (!player.on_ground) m_spr = S_MARIO_JUMP;
    else if (player.vx_fp != 0) m_spr = ((player.anim_tick / 4U) & 1U) ? S_MARIO_RUN1 : S_MARIO_RUN2;
    g_mirror_state.p_spr = m_spr;
    uint8_t vis = (player.transform_ticks > 0U || player.star_ticks > 0U ||
                   player.invuln_ticks == 0U || ((player.invuln_ticks & 2U) != 0U)) ? 1U : 0U;
    uint8_t vis_pwr = visual_power_level();
    g_mirror_state.p_flags = (uint8_t)((player.dir < 0 ? 0x01U : 0x00U) | (vis ? 0x02U : 0x00U) |
                                       ((game_state == STATE_PLAYING) ? 0x04U : 0x00U) |
                                       ((vis_pwr & 0x03U) << 3) |
                                       ((player.star_ticks > 0U) ? 0x20U : 0x00U) |
                                       (player.crouch ? 0x40U : 0x00U));
    g_mirror_state.fps = fps_display;
    g_mirror_state.ev_row = mirror_ev_row;
    g_mirror_state.ev_col = mirror_ev_col;
    g_mirror_state.ev_val = mirror_ev_val;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        g_mirror_state.enemies[i].x = enemies[i].x;
        g_mirror_state.enemies[i].y = enemies[i].y;
        g_mirror_state.enemies[i].kind = enemies[i].kind;
        g_mirror_state.enemies[i].flags = (uint8_t)((enemies[i].alive ? 1U : 0U) |
                                                    ((enemies[i].vx > 0) ? 2U : 0U) |
                                                    (((enemies[i].kind == EN_PIRANHA && enemies[i].sub > 0U) ? 8U : 0U)));
    }
    for (int i = 0; i < MAX_PLATFORMS; i++) {
        g_mirror_state.platforms[i].x = platforms[i].x;
        g_mirror_state.platforms[i].y = platforms[i].y;
        g_mirror_state.platforms[i].kind = platforms[i].kind;
        g_mirror_state.platforms[i].active = platforms[i].active;
    }
    for (int i = 0; i < MAX_FIREBARS; i++) {
        g_mirror_state.firebars[i].x = firebars[i].x;
        g_mirror_state.firebars[i].y = firebars[i].y;
        g_mirror_state.firebars[i].angle = firebars[i].angle;
        g_mirror_state.firebars[i].len = firebars[i].len;
    }
    for (int i = 0; i < MAX_POPUPS; i++) {
        g_mirror_state.popups[i].x = popups[i].x;
        g_mirror_state.popups[i].y = popups[i].y;
        g_mirror_state.popups[i].kind = popups[i].kind;
        g_mirror_state.popups[i].timer = popups[i].timer;
    }
    for (int i = 0; i < MAX_ITEMS; i++) {
        g_mirror_state.items[i].x = items[i].x;
        g_mirror_state.items[i].y = items[i].y;
        g_mirror_state.items[i].kind = items[i].kind;
    }
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        g_mirror_state.fireballs[i].x = fireballs[i].x;
        g_mirror_state.fireballs[i].y = fireballs[i].y;
        g_mirror_state.fireballs[i].active = fireballs[i].puff_timer > 0U ? 2U : fireballs[i].active;
    }
    g_mirror_state.net_role = (uint8_t)ws->role;
    g_mirror_state.net_stage = (uint8_t)ws->stage;
    g_mirror_state.my_slot = ws->my_slot;
    g_mirror_state.net_count = ws->player_count;
    uint8_t p_out = 0U;
    for (uint8_t s = 0U; s < WIFI_MAX_PLAYERS && p_out < 3U; s++) {
        if (s == ws->my_slot) continue;
        g_mirror_state.peers[p_out].x = remote_disp_x[s];
        g_mirror_state.peers[p_out].y = remote_disp_y[s];
        g_mirror_state.peers[p_out].spr = ws->players[s].spr;
        g_mirror_state.peers[p_out].flags = ws->players[s].flags;
        g_mirror_state.peers[p_out].active = ws->players[s].active;
        g_mirror_state.peers[p_out].slot = s;
        p_out++;
    }
}

static void poll_uart_keys(uint32_t now)
{
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart2);
    }
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
        uint8_t ch = (uint8_t)(huart2.Instance->RDR & 0xFFU);
        if (uart_esc_state == 0U) {
            if (ch == 0x1B) uart_esc_state = 1U;
            else if (ch >= 0x80U) {
                uart_state_mask = (uint8_t)(ch & 0x7FU);
                uart_state_until = now + 220U;
            } else {
                if (ch == 'a' || ch == 'A') uart_hold_until[0] = now + 170U;
                else if (ch == 'd' || ch == 'D') uart_hold_until[1] = now + 170U;
                else if (ch == 'w' || ch == 'W' || ch == ' ') uart_hold_until[2] = now + 180U;
                else if (ch == 'm' || ch == 'M') uart_hold_until[3] = now + 140U;
                else if (ch == 'f' || ch == 'F' || ch == 'j' || ch == 'J') uart_hold_until[4] = now + 140U;
                else if (ch == 's' || ch == 'S') uart_hold_until[5] = now + 140U;
            }
        } else if (uart_esc_state == 1U) {
            uart_esc_state = (ch == '[') ? 2U : 0U;
        } else {
            if (ch == 'D') uart_hold_until[0] = now + 170U;
            else if (ch == 'C') uart_hold_until[1] = now + 170U;
            else if (ch == 'A') uart_hold_until[2] = now + 180U;
            else if (ch == 'B') uart_hold_until[5] = now + 140U;
            uart_esc_state = 0U;
        }
    }
}

static uint8_t sample_merged_keys(uint32_t now)
{
    uint8_t mask = keyscan();
    poll_uart_keys(now);
    for (int i = 0; i < 7; i++) {
        if (now < uart_hold_until[i]) mask |= (uint8_t)(1U << i);
    }
    if (now < uart_state_until) mask |= uart_state_mask;
    uint8_t scmd = g_swd_key_cmd;
    if (scmd & 0x80U) {
        g_swd_key_cmd = 0U;
        swd_state_mask = (uint8_t)(scmd & 0x7FU);
        swd_state_until = now + 220U;
    }
    if (now < swd_state_until) mask |= swd_state_mask;
    return mask;
}

static void sync_wifi_frame(uint32_t now_ms)
{
    const WifiStatus *ws = WiFi_Net_GetStatus();
    if (ws->role == WIFI_MODE_SOLO) return;

    uint8_t m_spr = S_MARIO_STAND;
    if (!player.on_ground) m_spr = S_MARIO_JUMP;
    else if (player.vx_fp != 0) m_spr = ((player.anim_tick / 4U) & 1U) ? S_MARIO_RUN1 : S_MARIO_RUN2;
    uint8_t vis = (player.transform_ticks > 0U || player.invuln_ticks == 0 || (player.invuln_ticks & 2U)) ? 1U : 0U;
    uint8_t vis_pwr = visual_power_level();
    uint8_t pflags = (uint8_t)((player.dir < 0 ? 0x01U : 0x00U) | (vis ? 0x02U : 0x00U) |
                               ((game_state == STATE_PLAYING) ? 0x04U : 0x00U) | ((vis_pwr & 0x03U) << 3));
    WiFi_Net_SetLocalPlayer((int16_t)(player.x_fp >> 8), (int16_t)(player.y_fp >> 8), m_spr, pflags);

    if (ws->role == WIFI_MODE_HOST) {
        WiFi_Net_SetHostRoom((uint8_t)game_state, (uint8_t)(cur_stage_idx + 1U), lives, coins, score);
    }
    WiFi_Net_Tick(now_ms);
    ws = WiFi_Net_GetStatus();

    for (uint8_t s = 0U; s < WIFI_MAX_PLAYERS; s++) {
        if (s != ws->my_slot && ws->players[s].active) {
            int16_t rx = ws->players[s].x, ry = ws->players[s].y;
            int16_t dx = (int16_t)(rx - remote_disp_x[s]), dy = (int16_t)(ry - remote_disp_y[s]);
            if (remote_disp_x[s] == 0 && remote_disp_y[s] == 0) {
                remote_disp_x[s] = rx; remote_disp_y[s] = ry;
            } else {
                remote_disp_x[s] = (dx > 24 || dx < -24) ? rx : (int16_t)(remote_disp_x[s] + dx / 2);
                remote_disp_y[s] = (dy > 24 || dy < -24) ? ry : (int16_t)(remote_disp_y[s] + dy / 2);
            }
        }
    }

    uint8_t ev_t, ev_r, ev_c, ev_v;
    while (WiFi_Net_PopEvent(&ev_t, &ev_r, &ev_c, &ev_v)) {
        uint16_t full_c = (uint16_t)ev_c | (uint16_t)(((ev_r >> 4) & 1U) << 8);
        uint8_t real_r = (uint8_t)(ev_r & 0x0FU);
        if (ev_t == NET_EV_TILE && real_r < MAP_ROWS && full_c < cur_map_cols) {
            uint8_t old = tile_map[real_r][full_c];
            if (old != ev_v) {
                tile_map[real_r][full_c] = ev_v;
                mirror_ev_row = ev_r; mirror_ev_col = ev_c; mirror_ev_val = ev_v;
                mirror_map_rev++;
                if (ev_v == T_USED || (old == T_BRICK && ev_v == T_EMPTY)) play_sfx_id(SFX_HIT);
                else if (old == T_COIN && ev_v == T_EMPTY) { coins++; score += 200; play_sfx_id(SFX_COIN); }
            }
        } else if (ev_t == NET_EV_ENEMY_KILL && ev_r < MAX_ENEMIES) {
            if (enemies[ev_r].alive) {
                enemies[ev_r].alive = 0U; enemies[ev_r].squash_timer = 10U;
                score += 200U; play_sfx_id(SFX_HIT);
            }
        } else if (ev_t == NET_EV_COIN_SCORE) {
            if (ev_r == 1U) { coins++; score += 200; play_sfx_id(SFX_COIN); }
            else { score += 50; play_sfx_id(SFX_HIT); }
        } else if (ev_t == NET_EV_STAGE_CLEAR && ws->role == WIFI_MODE_HOST && game_state == STATE_PLAYING) {
            trigger_stage_clear(500U);
        }
    }

    if (ws->role == WIFI_MODE_JOIN && ws->stage == WIFI_ST_CONNECTED) {
        if (ws->remote_room_state == STATE_PLAYING && game_state == STATE_LOBBY) {
            uint8_t tgt_st = (ws->remote_level >= 1U && ws->remote_level <= SMB_STAGE_COUNT) ? (uint8_t)(ws->remote_level - 1U) : 0U;
            load_stage(tgt_st);
            game_state = STATE_PLAYING;
            play_sfx_id(SFX_COIN);
        } else if (ws->remote_room_state == STATE_LEVEL_CLEAR && game_state == STATE_PLAYING) {
            play_sfx_id(SFX_CLEAR);
            game_state = STATE_LEVEL_CLEAR;
            state_timer = 85U;
        } else if (ws->remote_room_state == STATE_PLAYING && game_state == STATE_LEVEL_CLEAR) {
            uint8_t tgt_st = (ws->remote_level >= 1U && ws->remote_level <= SMB_STAGE_COUNT) ? (uint8_t)(ws->remote_level - 1U) : 0U;
            load_stage(tgt_st);
            game_state = STATE_PLAYING;
        } else if (ws->remote_room_state == STATE_WIN && game_state != STATE_WIN) {
            game_state = STATE_WIN;
        } else if (ws->remote_room_state == STATE_GAME_OVER && game_state != STATE_GAME_OVER) {
            game_state = STATE_GAME_OVER;
        } else if (ws->remote_room_state == STATE_PLAYING && game_state == STATE_PLAYING &&
                   ws->remote_level >= 1U && ws->remote_level <= SMB_STAGE_COUNT &&
                   (uint8_t)(ws->remote_level - 1U) != cur_stage_idx) {
            load_stage((uint8_t)(ws->remote_level - 1U));
        }
        lives = ws->remote_lives; coins = ws->remote_coins; score = ws->remote_score;
    }
}

void Mario_Game_Loop(void)
{
    uint32_t t0 = HAL_GetTick();
    update_beep();
    LD3320_Audio_Tick();

    uint8_t keys = sample_merged_keys(t0);
    uint8_t pressed = (uint8_t)(keys & (uint8_t)(~prev_keys));
    prev_keys = keys;

    if (pressed & KEY4) {
        if (game_state == STATE_LOBBY || game_state == STATE_WIN) {
            WiFi_Net_Stop();
            lives = 3; coins = 0; score = 0;
            load_stage((uint8_t)((title_world - 1U) * 4U));
            game_state = STATE_TITLE;
            play_sfx_id(SFX_COIN);
        } else {
            audio_mode = (audio_mode == 0U) ? 2U : (uint8_t)(audio_mode - 1U);
            if (audio_mode == 0U) {
                beep_close(); beep_until_ms = 0U; beep_seq_len = 0U;
                LD3320_Audio_Stop();
                bgm_hw_active = 0U;
            } else {
                if (audio_mode == 1U && bgm_hw_active != 0U) {
                    LD3320_Audio_Stop();
                    bgm_hw_active = 0U;
                }
                play_sfx_id(SFX_COIN);
            }
        }
    }

    const WifiStatus *ws = WiFi_Net_GetStatus();

    switch (game_state) {
        case STATE_TITLE:
            if (pressed & KEY_DOWN) {
                title_world = (title_world >= 8U) ? 1U : (uint8_t)(title_world + 1U);
                load_stage((uint8_t)((title_world - 1U) * 4U));
                play_sfx_id(SFX_COIN);
            } else if (pressed & KEY3) {
                WiFi_Net_Stop();
                lives = 3; coins = 0; score = 0; player.power = 0; player.star_ticks = 0;
                load_stage((uint8_t)((title_world - 1U) * 4U));
                game_state = STATE_PRE_STAGE;
                state_timer = 75U;
                play_sfx_id(SFX_COIN);
            } else if (pressed & KEY1) {
                WiFi_Net_Start(WIFI_MODE_HOST);
                memset(remote_disp_x, 0, sizeof(remote_disp_x));
                memset(remote_disp_y, 0, sizeof(remote_disp_y));
                game_state = STATE_LOBBY;
                play_sfx_id(SFX_COIN);
            } else if (pressed & KEY2) {
                WiFi_Net_Start(WIFI_MODE_JOIN);
                memset(remote_disp_x, 0, sizeof(remote_disp_x));
                memset(remote_disp_y, 0, sizeof(remote_disp_y));
                game_state = STATE_LOBBY;
                play_sfx_id(SFX_COIN);
            }
            break;

        case STATE_LOBBY:
            if (ws->role == WIFI_MODE_HOST && (pressed & KEY3)) {
                lives = 3; coins = 0; score = 0; player.power = 0; player.star_ticks = 0;
                load_stage((uint8_t)((title_world - 1U) * 4U));
                game_state = STATE_PLAYING;
                if (cur_theme == 1U || audio_mode != 2U) play_sfx_id(SFX_INTRO);
            }
            break;

        case STATE_PRE_STAGE:
            if (state_timer > 0U) {
                state_timer--;
                if (state_timer == 0U || (pressed & KEY3)) {
                    game_state = STATE_PLAYING;
                    dirty_force_all = 1U;
                    if (cur_theme == 1U || audio_mode != 2U) play_sfx_id(SFX_INTRO);
                }
            }
            break;

        case STATE_PLAYING:
            update_gameplay(keys, pressed);
            break;

        case STATE_DEAD_WAIT:
            if (state_timer > 0U) {
                state_timer--;
                if (state_timer == 0U) {
                    if (lives == 0U) {
                        game_state = STATE_GAME_OVER;
                    } else {
                        load_stage(cur_stage_idx);
                        game_state = STATE_PRE_STAGE;
                        state_timer = 75U;
                    }
                }
            }
            break;

        case STATE_LEVEL_CLEAR:
            if (state_timer > 0U) {
                state_timer--;
                if (state_timer == 0U && ws->role != WIFI_MODE_JOIN) {
                    if (cur_stage_idx + 1U < SMB_STAGE_COUNT) {
                        cur_stage_idx++;
                        load_stage(cur_stage_idx);
                        game_state = (ws->role == WIFI_MODE_HOST) ? STATE_PLAYING : STATE_PRE_STAGE;
                        state_timer = 75U;
                    } else {
                        game_state = STATE_WIN;
                    }
                }
            }
            break;

        case STATE_GAME_OVER:
            if (pressed & KEY3) {
                lives = 3; coins = 0; score = 0; player.power = 0; player.star_ticks = 0;
                load_stage((uint8_t)((title_world - 1U) * 4U));
                game_state = STATE_PRE_STAGE;
                state_timer = 75U;
                play_sfx_id(SFX_COIN);
            }
            break;

        case STATE_WIN:
            if (pressed & KEY3) {
                WiFi_Net_Stop();
                lives = 3; coins = 0; score = 0; player.power = 0; player.star_ticks = 0;
                load_stage(0U);
                game_state = STATE_TITLE;
            }
            break;
    }

    update_bgm();
    sync_wifi_frame(t0);
    render_frame();
    frame_count++;
    fps_window_frames++;
    uint32_t fps_elapsed = HAL_GetTick() - fps_window_start_ms;
    if (fps_elapsed >= 500U) {
        uint32_t fps_calc = ((uint32_t)fps_window_frames * 1000U) / fps_elapsed;
        if (fps_calc > 99U) fps_calc = 99U;
        fps_display = (uint8_t)fps_calc;
        fps_window_frames = 0U;
        fps_window_start_ms = HAL_GetTick();
    }
    update_mirror_state();

    uint32_t dt = HAL_GetTick() - t0;
    if (dt < FRAME_MS) {
        HAL_Delay(FRAME_MS - dt);
    }
}
