#!/usr/bin/env python3
"""
Parse original 1985 Super Mario Bros. (smbdis.asm) Area & Enemy byte streams
into compact C headers (Inc/mario_levels.h) + verification summaries.

Vertical mapping:
  NES playfield has 13 rows (r=0..12, below 32px HUD).
  Our STM32 LCD has 14 rows (r=0..13, below 16px HUD).
  Mapping nes_r -> stm_r = nes_r + 1 preserves exact 2-row ground (rows 12..13),
  clean 1-row top headroom (row 0), and 1:1 pipe/block/stair/flagpole clearances.
"""

import os
import re
import sys

SMBDIS_PATH = "/tmp/smbdis.asm"

TERRAIN_BITS = [
    (0b00000000, 0b00000000),  # 0: no ceiling or floor
    (0b00000000, 0b00011000),  # 1: no ceiling, floor 2 (rows 11..12)
    (0b00000001, 0b00011000),  # 2: ceiling 1, floor 2
    (0b00000111, 0b00011000),  # 3: ceiling 3, floor 2
    (0b00001111, 0b00011000),  # 4: ceiling 4, floor 2
    (0b11111111, 0b00011000),  # 5: ceiling 8, floor 2
    (0b00000001, 0b00011111),  # 6: ceiling 1, floor 5 (rows 8..12)
    (0b00000111, 0b00011111),  # 7: ceiling 3, floor 5
    (0b00001111, 0b00011111),  # 8: ceiling 4, floor 5
    (0b10000001, 0b00011111),  # 9: ceiling 1 + row 8, floor 5
    (0b00000001, 0b00000000),  # 10: ceiling 1, no floor
    (0b10001111, 0b00011111),  # 11: ceiling 4 + row 8, floor 5
    (0b11110001, 0b00011111),  # 12: solid top 5 + floor 5
    (0b11111001, 0b00011000),  # 13: ceiling 1, middle 5, floor 2
    (0b11110001, 0b00011000),  # 14: ceiling 1, middle 4, floor 2
    (0b11111111, 0b00011111),  # 15: completely solid top to bottom
]

STAIR_ROW = [0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A]
STAIR_H =   [0x07, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00]

# Tile IDs matching Src/mario_game.c
T_EMPTY = 0
T_GROUND = 1
T_BRICK = 2
T_QBLOCK = 3
T_USED = 4
T_PIPE_TL = 5
T_PIPE_TR = 6
T_PIPE_BL = 7
T_PIPE_BR = 8
T_COIN = 9
T_FLAG_TOP = 10
T_FLAG_POLE = 11
T_STONE = 12
T_QBLOCK_MUSH = 13
T_QBLOCK_FLOWER = 14
T_QBLOCK_HIDDEN_COIN = 15
T_QBLOCK_HIDDEN_1UP = 16
T_BRICK_POWER = 17
T_BRICK_VINE = 18
T_BRICK_STAR = 19
T_BRICK_10COIN = 20
T_BRICK_1UP = 21
T_BRIDGE = 22
T_WATER_SURF = 23
T_WATER_FILL = 24
T_LAVA_SURF = 25
T_LAVA_FILL = 26
T_CANNON_TOP = 27
T_CANNON_MID = 28
T_CANNON_BOT = 29
T_AXE = 30
T_CHAIN = 31
T_SPRING_TOP = 32
T_SPRING_BOT = 33
T_CLOUD_PLAT = 34
T_MUSH_PLAT_L = 35
T_MUSH_PLAT_M = 36
T_MUSH_PLAT_R = 37
T_MUSH_STEM = 38
T_PIPE_WARP_TL = 39
T_PIPE_WARP_TR = 40
T_SIDE_PIPE_T = 41
T_SIDE_PIPE_B = 42
T_CASTLE_WALL = 43
T_CASTLE_ARCH = 44
T_CASTLE_BATT = 45
T_ROPE = 46

# Enemy IDs matching Src/mario_game.c
EN_GOOMBA = 1
EN_KOOPA_G = 2
EN_KOOPA_R = 3
EN_PARATROOPA_J = 4   # hopping green paratroopa
EN_PARATROOPA_F = 5   # flying red paratroopa (vertical patrol)
EN_PARATROOPA_H = 6   # horizontal green paratroopa
EN_PIRANHA = 7
EN_BUZZY = 8
EN_SPINY = 9
EN_LAKITU = 10
EN_HAMMER_BRO = 11
EN_BULLET = 12
EN_BLOOBER = 13
EN_CHEEP_GREY = 14
EN_CHEEP_RED = 15
EN_PODOBOO = 16
EN_BOWSER = 17
EN_TOAD_PEACH = 18

# Platform / Firebar special entities stored separately
SP_FIREBAR_SHORT = 1
SP_FIREBAR_LONG = 2
SP_PLAT_VERT = 3
SP_PLAT_HORIZ = 4
SP_PLAT_UP = 5
SP_PLAT_DOWN = 6
SP_PLAT_DROP = 7
SP_PLAT_BAL_L = 8


def parse_smbdis_labels(path):
    txt = open(path, "r", encoding="utf-8", errors="ignore").read()
    lines = txt.splitlines()
    labels = {}
    cur_label = None
    cur_bytes = []

    def flush():
        nonlocal cur_label, cur_bytes
        if cur_label and cur_bytes:
            labels[cur_label] = cur_bytes
        cur_label = None
        cur_bytes = []

    for line in lines:
        code = line.split(";")[0].strip()
        if not code:
            continue
        m_lbl = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):(.*)$", code)
        if m_lbl:
            flush()
            cur_label = m_lbl.group(1)
            rest = m_lbl.group(2).strip()
            if rest.startswith(".db "):
                code = rest
            else:
                continue
        if cur_label and code.startswith(".db "):
            vals = [v.strip() for v in code[4:].split(",")]
            parsed = []
            ok = True
            for v in vals:
                if v.startswith("$"):
                    parsed.append(int(v[1:], 16))
                elif v.startswith("%"):
                    parsed.append(int(v[1:], 2))
                elif v.isdigit():
                    parsed.append(int(v, 10))
                else:
                    ok = False
                    break
            if ok:
                cur_bytes.extend(parsed)
            else:
                flush()
        elif cur_label:
            flush()
    flush()

    # Read AreaAddrOffsets for Worlds 1..8
    world_areas = {}
    for w in range(1, 9):
        m = re.search(rf"World{w}Areas:\s*\.db\s+([^\n;]+)", txt)
        world_areas[w] = [int(x.strip()[1:], 16) for x in m.group(1).split(",")]

    # Read AreaDataAddrLow list and EnemyDataAddrLow list order
    area_names = []
    for m in re.finditer(r"AreaDataAddrLow:.*?(?=AreaDataAddrHigh:)", txt, re.S):
        area_names = re.findall(r"<(L_[A-Za-z0-9_]+)", m.group(0))
    enemy_names = []
    for m in re.finditer(r"EnemyDataAddrLow:.*?(?=EnemyDataAddrHigh:)", txt, re.S):
        enemy_names = re.findall(r"<(E_[A-Za-z0-9_]+)", m.group(0))

    return labels, world_areas, area_names, enemy_names


def area_ptr_to_indices(area_ptr):
    area_type = (area_ptr >> 5) & 0x03
    low5 = area_ptr & 0x1F
    area_h_offs = [0x00, 0x03, 0x19, 0x1C]
    enemy_h_offs = [0x1F, 0x06, 0x1C, 0x00]
    a_idx = area_h_offs[area_type] + low5
    e_idx = enemy_h_offs[area_type] + low5
    return area_type, a_idx, e_idx


def nes_render_under(grid, col, r_start, height, val):
    for i in range(height + 1):
        rr = r_start + i
        if rr >= 13:
            break
        cur = grid[rr][col]
        if cur in (T_MUSH_PLAT_M,) and val != cur:
            continue
        if val == T_MUSH_STEM and cur != T_EMPTY:
            continue
        grid[rr][col] = val


def decode_area(area_bytes, area_type, is_world8=False, no_piranha=False):
    b0, b1 = area_bytes[0], area_bytes[1]
    fg_or_bgc = b0 & 0x07
    fg_scenery = fg_or_bgc if fg_or_bgc < 4 else 0
    bg_color_ctrl = fg_or_bgc if fg_or_bgc >= 4 else 0
    entr_ctrl = (b0 >> 3) & 0x07
    timer_ctrl = (b0 >> 6) & 0x03

    terr_ctrl = b1 & 0x0F
    bg_scenery = (b1 >> 4) & 0x03
    init_bg_scenery = bg_scenery
    init_bg_color = bg_color_ctrl
    style_bits = (b1 >> 6) & 0x03
    cloud_override = (style_bits == 3)
    area_style = 0 if cloud_override else style_bits

    data = area_bytes[2:]
    # First pass to find exact stream end & max column
    # Simulate exact ProcessAreaData / AreaParserCore column-by-column!
    grid = [[T_EMPTY] * 480 for _ in range(13)]
    piranhas = []
    cannons = []
    springs = []
    stair_ctrl = 9
    castle_cols = []
    flagpole_col = None
    axe_col = None
    warp_zone = False
    max_obj_col = 0

    # We mirror the 3-slot NES object decoder faithfully
    obj_offset = 0
    obj_page = 0
    obj_page_sel = False
    slot_len = [-1, -1, -1]
    slot_ofs = [0, 0, 0]
    slot_meta = [None, None, None]  # extra state like mush half len

    def terrain_tile_for_row(r):
        if area_type == 0 and is_world8:
            return T_STONE
        if cloud_override:
            return T_CLOUD_PLAT
        if area_type == 0:
            return T_GROUND  # underwater coral floor
        if area_type == 1:
            return T_GROUND
        if area_type == 2:
            return T_GROUND if r >= 11 else T_BRICK
        return T_STONE

    col_idx = 0
    # Run up to 340 columns or 32 cols past $fd and end-of-level objects
    end_reached_col = None
    while col_idx < 460:
        page = col_idx >> 4
        c_in_page = col_idx & 0x0F

        # 1. Foreground scenery (applied to column before objects)
        if fg_scenery == 1:  # in water (ForeSceneryData rows 0=waves, 1..10=water, 11..12=coral floor)
            grid[0][col_idx] = T_WATER_SURF
            for rr in range(1, 11):
                grid[rr][col_idx] = T_EMPTY
            grid[11][col_idx] = T_GROUND
            grid[12][col_idx] = T_GROUND
        elif fg_scenery == 2:  # castle wall backdrop
            pass
        elif fg_scenery == 3:  # over water / over lava in castles
            if area_type == 3:
                grid[11][col_idx] = T_LAVA_SURF
                grid[12][col_idx] = T_LAVA_FILL
            else:
                grid[11][col_idx] = T_WATER_SURF
                grid[12][col_idx] = T_WATER_FILL

        # 2. Terrain bits (Bitmasks in smbdis.asm are LSB-first: bit 0=top row)
        hi_bits, lo_bits = TERRAIN_BITS[terr_ctrl]
        for r in range(13):
            if cloud_override:
                if r == 0:
                    mask = (hi_bits & 0x01) != 0
                elif r == 11:
                    mask = (lo_bits & 0x08) != 0
                else:
                    mask = False
            elif r < 8:
                mask = (hi_bits >> r) & 1
            else:
                mask = (lo_bits >> (r - 8)) & 1
            if mask:
                grid[r][col_idx] = terrain_tile_for_row(r)

        # 3. Process 3 object slots just like NES ProcessAreaData (backwards x=2,1,0)
        while True:
            behind_flag = False
            for x in (2, 1, 0):
                if obj_offset >= len(data) or data[obj_offset] == 0xFD:
                    if end_reached_col is None:
                        end_reached_col = col_idx
                    # active multi-col objects still finish
                    if slot_len[x] < 0:
                        continue
                if slot_len[x] < 0:
                    # Peek new object at obj_offset
                    if obj_offset >= len(data) or data[obj_offset] == 0xFD:
                        continue
                    b_a = data[obj_offset]
                    b_b = data[obj_offset + 1]
                    if (b_b & 0x80) and not obj_page_sel:
                        obj_page += 1
                        obj_page_sel = True
                    row_nib = b_a & 0x0F
                    if row_nib == 0x0D and not (b_b & 0x40):
                        if not obj_page_sel:
                            obj_page = b_b & 0x1F
                            obj_page_sel = True
                            obj_offset += 2
                            obj_page_sel = False
                            continue
                    if obj_page < page:
                        behind_flag = True
                        obj_offset += 2
                        obj_page_sel = False
                        continue
                    if obj_page > page:
                        continue
                    col_nib = (b_a >> 4) & 0x0F
                    if col_nib != c_in_page:
                        continue
                    slot_ofs[x] = obj_offset
                    obj_offset += 2
                    obj_page_sel = False

                # Decode active object in slot x
                ofs = slot_ofs[x]
                b_a = data[ofs]
                b_b = data[ofs + 1]
                row_nib = b_a & 0x0F
                col_nib = (b_a >> 4) & 0x0F

                if row_nib == 0x0E:
                    # AlterAreaAttributes
                    if b_b & 0x40:
                        v = b_b & 0x07
                        if v >= 4:
                            bg_color_ctrl = v
                            fg_scenery = 0
                        else:
                            fg_scenery = v
                    else:
                        terr_ctrl = b_b & 0x0F
                        bg_scenery = (b_b >> 4) & 0x03
                    slot_len[x] = -1
                    continue
                elif row_nib == 0x0D and (b_b & 0x40):
                    # Special row 13 objects
                    oid = b_b & 0x3F
                    if oid == 0x00:  # IntroPipe
                        if slot_len[x] < 0:
                            slot_len[x] = 3
                        # horizontal pipe leading into vertical shaft (3 cols + shaft)
                        rem = slot_len[x]
                        if rem in (3, 2):
                            grid[9][col_idx] = T_SIDE_PIPE_T
                            grid[10][col_idx] = T_SIDE_PIPE_B
                        elif rem == 1:
                            grid[7][col_idx] = T_PIPE_BL
                            grid[8][col_idx] = T_PIPE_BL
                            grid[9][col_idx] = T_SIDE_PIPE_T
                            grid[10][col_idx] = T_SIDE_PIPE_B
                        else:
                            grid[7][col_idx] = T_PIPE_BR
                            grid[8][col_idx] = T_PIPE_BR
                            grid[9][col_idx] = T_PIPE_BR
                            grid[10][col_idx] = T_PIPE_BR
                    elif oid == 0x01:  # FlagpoleObject
                        grid[0][col_idx] = T_FLAG_TOP
                        for rr in range(1, 10):
                            grid[rr][col_idx] = T_FLAG_POLE
                        grid[10][col_idx] = T_STONE
                        flagpole_col = col_idx
                    elif oid == 0x02:  # AxeObj
                        grid[6][col_idx] = T_AXE
                        axe_col = col_idx
                    elif oid == 0x03:  # ChainObj
                        grid[7][col_idx] = T_CHAIN
                    elif oid == 0x04:  # CastleBridgeObj
                        if slot_len[x] < 0:
                            slot_len[x] = 12
                        grid[8][col_idx] = T_BRIDGE
                    elif oid == 0x05:  # ScrollLockObject_Warp
                        warp_zone = True
                    # oid 6..11: scroll lock / frenzy / loop cmd (no tiles)
                    max_obj_col = max(max_obj_col, col_idx)
                elif row_nib == 0x0F:
                    oid = (b_b >> 4) & 0x07
                    arg = b_b & 0x0F
                    if oid == 0:  # EndlessRope
                        for rr in range(13):
                            grid[rr][col_idx] = T_ROPE
                    elif oid == 1:  # BalancePlatRope
                        for rr in range(1, arg + 1):
                            if rr < 13:
                                grid[rr][col_idx] = T_ROPE
                    elif oid == 2:  # CastleObject
                        if slot_len[x] < 0:
                            slot_len[x] = 4
                        step = 4 - slot_len[x]  # 0..4 left to right
                        c_top = min(10, arg)
                        if step == 0 or step == 4:
                            for rr in range(min(10, c_top + 2), 11):
                                grid[rr][col_idx] = T_CASTLE_BATT if rr == min(10, c_top + 2) else T_CASTLE_WALL
                        elif step in (1, 3):
                            for rr in range(c_top, 11):
                                grid[rr][col_idx] = T_CASTLE_BATT if rr == c_top else T_CASTLE_WALL
                        else:  # center doorway column step == 2
                            grid[c_top][col_idx] = T_CASTLE_BATT
                            for rr in range(c_top + 1, 9):
                                grid[rr][col_idx] = T_CASTLE_WALL
                            if c_top < 9:
                                grid[9][col_idx] = T_CASTLE_ARCH
                                grid[10][col_idx] = T_CASTLE_ARCH
                        castle_cols.append(col_idx)
                    elif oid == 3:  # StaircaseObject
                        if slot_len[x] < 0:
                            slot_len[x] = arg
                            stair_ctrl = 9
                        stair_ctrl -= 1
                        if stair_ctrl >= 0:
                            sr = STAIR_ROW[stair_ctrl]
                            sh = STAIR_H[stair_ctrl]
                            nes_render_under(grid, col_idx, sr, sh, T_STONE)
                    elif oid == 4:  # ExitPipe
                        if slot_len[x] < 0:
                            slot_len[x] = 3
                        rem = slot_len[x]
                        # vertical shaft down to bottom pipe entry
                        v_len = max(1, (arg & 0x0F) - 2)
                        if rem in (3, 2):
                            for rr in range(v_len):
                                if rr < 11:
                                    grid[rr][col_idx] = T_PIPE_BL if rem == 3 else T_PIPE_BR
                            grid[v_len][col_idx] = T_SIDE_PIPE_T
                            grid[v_len + 1][col_idx] = T_SIDE_PIPE_B
                        elif rem == 1:
                            grid[v_len][col_idx] = T_SIDE_PIPE_T
                            grid[v_len + 1][col_idx] = T_SIDE_PIPE_B
                        else:
                            grid[v_len][col_idx] = T_PIPE_WARP_TL
                            grid[v_len + 1][col_idx] = T_PIPE_BL
                    max_obj_col = max(max_obj_col, col_idx)
                elif row_nib == 0x0C:
                    oid = (b_b >> 4) & 0x07
                    arg = b_b & 0x0F
                    if oid == 0:  # Hole_Empty
                        if slot_len[x] < 0:
                            slot_len[x] = arg
                        val = T_WATER_FILL if area_type == 0 else T_EMPTY
                        for rr in range(8, 13):
                            grid[rr][col_idx] = val
                    elif oid == 1:  # PulleyRopeObject
                        if slot_len[x] < 0:
                            slot_len[x] = arg
                        grid[0][col_idx] = T_ROPE
                    elif oid in (2, 3, 4):  # Bridge High(6)/Middle(7)/Low(9)
                        if slot_len[x] < 0:
                            slot_len[x] = arg
                        br = {2: 6, 3: 7, 4: 9}[oid]
                        grid[br + 1][col_idx] = T_BRIDGE
                    elif oid == 5:  # Hole_Water / Lava pit
                        if slot_len[x] < 0:
                            slot_len[x] = arg
                        if area_type == 3:
                            grid[10][col_idx] = T_LAVA_SURF
                            grid[11][col_idx] = T_LAVA_FILL
                            grid[12][col_idx] = T_LAVA_FILL
                        else:
                            grid[10][col_idx] = T_WATER_SURF
                            grid[11][col_idx] = T_WATER_FILL
                            grid[12][col_idx] = T_WATER_FILL
                    elif oid in (6, 7):  # QuestionBlockRow_High(3) / Low(7)
                        if slot_len[x] < 0:
                            slot_len[x] = arg
                        qr = 3 if oid == 6 else 7
                        grid[qr][col_idx] = T_QBLOCK
                    max_obj_col = max(max_obj_col, col_idx)
                else:
                    # rows 0x00..0x0B
                    lrg = (b_b >> 4) & 0x07
                    arg = b_b & 0x0F
                    if lrg != 0:
                        # Large object
                        if lrg == 7:  # VerticalPipe
                            is_warp = bool(b_b & 0x08)
                            h = b_b & 0x07
                            if slot_len[x] < 0:
                                slot_len[x] = 1
                                if not no_piranha:
                                    piranhas.append((col_idx, row_nib))
                            if slot_len[x] == 1:
                                grid[row_nib][col_idx] = T_PIPE_WARP_TL if is_warp else T_PIPE_TL
                                top_t = grid[row_nib][col_idx]
                                nes_render_under(grid, col_idx, row_nib + 1, h - 1, T_PIPE_BL)
                            else:
                                grid[row_nib][col_idx] = T_PIPE_WARP_TR if is_warp else T_PIPE_TR
                                nes_render_under(grid, col_idx, row_nib + 1, h - 1, T_PIPE_BR)
                        elif lrg == 1:  # AreaStyleObject (Tree, Mushroom, Cannon)
                            if area_style == 2:  # BulletBillCannon
                                cannons.append((col_idx, row_nib))
                                grid[row_nib][col_idx] = T_CANNON_TOP
                                if arg >= 1 and row_nib + 1 < 13:
                                    grid[row_nib + 1][col_idx] = T_CANNON_MID
                                if arg >= 2:
                                    nes_render_under(grid, col_idx, row_nib + 2, arg - 2, T_CANNON_BOT)
                            elif area_style == 1:  # MushroomLedge
                                if slot_len[x] < 0:
                                    slot_len[x] = arg
                                    slot_meta[x] = (arg, arg // 2)
                                    grid[row_nib][col_idx] = T_MUSH_PLAT_L
                                else:
                                    tot, half = slot_meta[x]
                                    if slot_len[x] == 0:
                                        grid[row_nib][col_idx] = T_MUSH_PLAT_R
                                    else:
                                        grid[row_nib][col_idx] = T_MUSH_PLAT_M
                                        if slot_len[x] == half:
                                            nes_render_under(grid, col_idx, row_nib + 1, 12, T_MUSH_STEM)
                            else:  # TreeLedge / Cloud
                                if slot_len[x] < 0:
                                    slot_len[x] = arg
                                grid[row_nib][col_idx] = T_CLOUD_PLAT if cloud_override else T_GROUND
                        elif lrg == 2:  # RowOfBricks
                            if slot_len[x] < 0:
                                slot_len[x] = arg
                            bt = T_CLOUD_PLAT if cloud_override else T_BRICK
                            grid[row_nib][col_idx] = bt
                        elif lrg == 3:  # RowOfSolidBlocks
                            if slot_len[x] < 0:
                                slot_len[x] = arg
                            st = T_STONE if area_type in (1, 2, 3) else T_GROUND
                            grid[row_nib][col_idx] = st
                        elif lrg == 4:  # RowOfCoins
                            if slot_len[x] < 0:
                                slot_len[x] = arg
                            grid[row_nib][col_idx] = T_COIN
                        elif lrg == 5:  # ColumnOfBricks
                            nes_render_under(grid, col_idx, row_nib, arg, T_BRICK)
                        elif lrg == 6:  # ColumnOfSolidBlocks
                            st = T_STONE if area_type in (1, 2, 3) else T_GROUND
                            nes_render_under(grid, col_idx, row_nib, arg, st)
                    else:
                        # Small object (single col)
                        small_map = {
                            0: T_QBLOCK_MUSH,
                            1: T_QBLOCK,
                            2: T_QBLOCK_HIDDEN_COIN,
                            3: T_QBLOCK_HIDDEN_1UP,
                            4: T_BRICK_POWER,
                            5: T_BRICK_VINE,
                            6: T_BRICK_STAR,
                            7: T_BRICK_10COIN,
                            8: T_BRICK_1UP,
                            9: T_PIPE_BL,  # underwater pipe column
                            10: T_USED,
                            11: T_SPRING_TOP,
                        }
                        t = small_map.get(arg, T_USED)
                        grid[row_nib][col_idx] = t
                        if arg == 9 and row_nib + 1 < 13:
                            grid[row_nib + 1][col_idx] = T_PIPE_BR
                        if arg == 11:
                            springs.append((col_idx, row_nib))
                            if row_nib + 1 < 13:
                                grid[row_nib + 1][col_idx] = T_SPRING_BOT
                    max_obj_col = max(max_obj_col, col_idx)

                if slot_len[x] >= 0:
                    slot_len[x] -= 1
            if not behind_flag:
                break

        if end_reached_col is not None and col_idx >= max(max_obj_col + 6, (end_reached_col + 16)):
            if all(s < 0 for s in slot_len):
                break
        col_idx += 1

    width = min(460, max(col_idx, max_obj_col + 10))
    # Shift rows down by +1 (r=0..12 -> row 1..13)
    out = [[T_EMPTY] * width for _ in range(14)]
    for r in range(13):
        out[r + 1] = grid[r][:width]
    return {
        "grid": out,
        "width": width,
        "area_type": area_type,
        "bg_scenery": init_bg_scenery,
        "bg_color": init_bg_color,
        "cloud_override": cloud_override,
        "piranhas": [(c, r + 1) for c, r in piranhas if c < width],
        "cannons": [(c, r + 1) for c, r in cannons if c < width],
        "springs": [(c, r + 1) for c, r in springs if c < width],
        "flagpole": flagpole_col,
        "axe": axe_col,
        "warp_zone": warp_zone,
        "timer": [0, 400, 300, 200][timer_ctrl] or 400,
    }


def decode_enemies(enemy_bytes, width, world_idx, grid=None):
    """Parse EnemyData stream into enemy spawns, transitions, firebars & platforms."""
    enemies = []
    specials = []
    transitions = []
    solid_set = {
        T_GROUND, T_BRICK, T_USED, T_PIPE_TL, T_PIPE_TR, T_PIPE_BL, T_PIPE_BR,
        T_PIPE_WARP_TL, T_PIPE_WARP_TR, T_STONE, T_CANNON_TOP, T_CANNON_MID, T_CANNON_BOT,
    }

    def clear_col(c0, r0):
        if grid is None or not (0 <= r0 < 14):
            return c0
        if grid[r0][c0] not in solid_set:
            return c0
        for dc in (-1, 1, -2, 2, -3, 3):
            nc = c0 + dc
            if 0 <= nc < width and grid[r0][nc] not in solid_set:
                return nc
        return c0

    i = 0
    page = 0
    page_sel = False
    while i < len(enemy_bytes):
        b0 = enemy_bytes[i]
        if b0 == 0xFF:
            break
        row = b0 & 0x0F
        if row == 0x0F and not page_sel:
            page = enemy_bytes[i + 1] & 0x3F
            page_sel = True
            i += 2
            continue
        b1 = enemy_bytes[i + 1]
        if (b1 & 0x80) and not page_sel:
            page += 1
            page_sel = True

        col = (page << 4) | ((b0 >> 4) & 0x0F)
        if row == 0x0E:
            b2 = enemy_bytes[i + 2]
            w_req = (b2 >> 5) & 0x07
            dest_ptr = b1
            dest_page = b2 & 0x1F
            transitions.append((col, w_req, dest_ptr, dest_page))
            i += 3
            page_sel = False
            continue

        i += 2
        page_sel = False
        eid = b1 & 0x3F
        # In smbdis.asm, raw enemy row is the exact top-of-sprite row above ground/platform row (+1=floor)
        stm_r = min(11, row)
        if col >= width:
            continue

        # Map SMBDIS enemy IDs
        if eid in (0x00,):
            enemies.append((clear_col(col, stm_r), stm_r, EN_KOOPA_G))
        elif eid == 0x02:
            enemies.append((clear_col(col, stm_r), stm_r, EN_BUZZY))
        elif eid == 0x03:
            enemies.append((clear_col(col, stm_r), stm_r, EN_KOOPA_R))
        elif eid == 0x05:
            enemies.append((clear_col(col, stm_r), stm_r, EN_HAMMER_BRO))
        elif eid == 0x06:
            enemies.append((clear_col(col, stm_r), stm_r, EN_GOOMBA))
        elif eid == 0x07:
            enemies.append((col, max(2, min(10, stm_r)), EN_BLOOBER))
        elif eid == 0x0A:
            enemies.append((col, max(2, min(11, stm_r)), EN_CHEEP_GREY))
        elif eid == 0x0B:
            enemies.append((col, max(2, min(11, stm_r)), EN_CHEEP_RED))
        elif eid == 0x0C:
            enemies.append((col, 11, EN_PODOBOO))
        elif eid == 0x0D:
            enemies.append((col, stm_r, EN_PIRANHA))
        elif eid == 0x0E:
            enemies.append((clear_col(col, stm_r), stm_r, EN_PARATROOPA_J))
        elif eid == 0x0F:
            enemies.append((col, max(2, min(10, stm_r)), EN_PARATROOPA_F))
        elif eid == 0x10:
            enemies.append((col, max(2, min(10, stm_r)), EN_PARATROOPA_H))
        elif eid == 0x11:
            enemies.append((col, 2, EN_LAKITU))
        elif eid == 0x12:
            enemies.append((clear_col(col, stm_r), stm_r, EN_SPINY))
        elif eid in (0x1B, 0x1C, 0x1D, 0x1E, 0x1F):
            # Firebars attach to T_USED pivot block (raw_r - 1 in our +1 shifted grid)
            fb_r = max(1, min(12, row - 1))
            if grid is not None:
                for rr in range(max(1, row - 2), min(13, row + 2)):
                    if grid[rr][col] == T_USED:
                        fb_r = rr
                        break
            kind = SP_FIREBAR_LONG if eid == 0x1F else SP_FIREBAR_SHORT
            specials.append((col, fb_r, kind, eid))
        elif eid in (0x24,):
            specials.append((col, max(2, min(11, row)), SP_PLAT_BAL_L, eid))
        elif eid in (0x25,):
            specials.append((col, max(2, min(11, row)), SP_PLAT_VERT, eid))
        elif eid in (0x26, 0x2B):
            specials.append((col, max(2, min(11, row)), SP_PLAT_UP, eid))
        elif eid in (0x27, 0x2C):
            specials.append((col, max(2, min(11, row)), SP_PLAT_DOWN, eid))
        elif eid in (0x28, 0x2A):
            specials.append((col, max(2, min(11, row)), SP_PLAT_HORIZ, eid))
        elif eid == 0x29:
            specials.append((col, max(2, min(11, row)), SP_PLAT_DROP, eid))
        elif eid == 0x2D:
            enemies.append((col, 8, EN_BOWSER))
        elif eid == 0x35:
            enemies.append((col, 8, EN_TOAD_PEACH))
        elif 0x37 <= eid <= 0x3E:
            off = eid - 0x37
            kind = EN_KOOPA_G if off >= 4 else EN_GOOMBA
            cnt = 3 if (off & 1) else 2
            gr = 7 if (off & 2) else 11
            for k in range(cnt):
                if col + k < width:
                    ec = clear_col(col + k, gr)
                    enemies.append((ec, gr, kind))
    return enemies, specials, transitions


WARP_PIPE_DOWN = 1
WARP_VINE_TOP = 2
WARP_SIDE_PIPE = 3
WARP_TO_WORLD = 4
WARP_FALL_RETURN = 5

AREA_RETURN_SENTINEL = 254
AREA_EXIT_COURTYARD = 253


def emit_header(out_path, labels, world_areas, area_names, enemy_names):
    # Map area index 0..33 -> (area_type, area_name, enemy_name)
    ptr_map = {}
    ah_bounds = [0x00, 0x03, 0x19, 0x1C, len(area_names)]
    eh_offs = [0x1F, 0x06, 0x1C, 0x00]
    for at in range(4):
        ah = ah_bounds[at]
        ah_next = ah_bounds[at + 1]
        eh = eh_offs[at]
        for ai in range(ah, ah_next):
            ei = eh + (ai - ah)
            ptr_map[ai] = (at, area_names[ai], enemy_names[ei])

    def area_idx_from_ptr(ptr):
        _, ai, _ = area_ptr_to_indices(ptr & 0x7F)
        return ai

    # Sub-area pointer constants
    AI_BONUS_UG = area_idx_from_ptr(0x42)   # 27: L_UndergroundArea3
    AI_CLOUD_DAY = area_idx_from_ptr(0x2B)  # 14: L_GroundArea12
    AI_CLOUD_NGT = area_idx_from_ptr(0x34)  # 23: L_GroundArea21
    AI_WATER_52 = area_idx_from_ptr(0x00)   # 0:  L_WaterArea1
    AI_WATER_84 = area_idx_from_ptr(0x02)   # 2:  L_WaterArea3
    AI_WARP_42 = area_idx_from_ptr(0x2F)    # 18: L_GroundArea16
    AI_STAGE_11 = area_idx_from_ptr(0x25)   # 8:  L_GroundArea6 (courtyard exit col=179)
    AI_CASTLE_84 = area_idx_from_ptr(0x65)  # 33: L_CastleArea6

    # Explicit per-area warps (src_area -> list of (src_col, src_row, kind, dest_area, dest_col, dest_row, ret_col))
    # ret_col is used by sub-area WARP_SIDE_PIPE / WARP_FALL_RETURN to emerge back in parent area.
    warps_by_area = {i: [] for i in range(len(area_names))}
    # bonus_ug 5 room side pipes -> return to parent saved column
    for c_exit in (13, 45, 77, 109, 141):
        warps_by_area[AI_BONUS_UG].append((c_exit, 9, WARP_SIDE_PIPE, AREA_RETURN_SENTINEL, 0, 0, 0))
    # cloud_day & cloud_ngt falling off bottom -> return gracefully
    warps_by_area[AI_CLOUD_DAY].append((0, 14, WARP_FALL_RETURN, AREA_RETURN_SENTINEL, 0, 0, 0))
    warps_by_area[AI_CLOUD_NGT].append((0, 14, WARP_FALL_RETURN, AREA_RETURN_SENTINEL, 0, 0, 0))
    # water_52 side pipe exit -> return to parent
    warps_by_area[AI_WATER_52].append((62, 6, WARP_SIDE_PIPE, AREA_RETURN_SENTINEL, 0, 0, 0))
    # warp_42 (8, 7, 6) pipes
    warps_by_area[AI_WARP_42].append((50, 9, WARP_TO_WORLD, 8, 2, 10, 0))
    warps_by_area[AI_WARP_42].append((54, 9, WARP_TO_WORLD, 7, 2, 10, 0))
    warps_by_area[AI_WARP_42].append((58, 9, WARP_TO_WORLD, 6, 2, 10, 0))
    # 1-1 down pipe -> bonus_ug room 0 -> return col 163
    warps_by_area[AI_STAGE_11].append((57, 8, WARP_PIPE_DOWN, AI_BONUS_UG, 2, 2, 163))
    # 1-2 (ai=25): down pipe 103 -> bonus_ug room 1 (col 34), ret 115; side pipe 166 -> courtyard; warp pipes 178/182/186 -> W4/W3/W2
    AI_12 = area_idx_from_ptr(0x40)
    warps_by_area[AI_12].append((103, 9, WARP_PIPE_DOWN, AI_BONUS_UG, 34, 2, 115))
    warps_by_area[AI_12].append((166, 6, WARP_SIDE_PIPE, AREA_EXIT_COURTYARD, 179, 9, 0))
    warps_by_area[AI_12].append((178, 9, WARP_TO_WORLD, 4, 2, 10, 0))
    warps_by_area[AI_12].append((182, 9, WARP_TO_WORLD, 3, 2, 10, 0))
    warps_by_area[AI_12].append((186, 9, WARP_TO_WORLD, 2, 2, 10, 0))
    # 2-1 (ai=11)
    AI_21 = area_idx_from_ptr(0x28)
    warps_by_area[AI_21].append((103, 8, WARP_PIPE_DOWN, AI_BONUS_UG, 2, 2, 115))
    warps_by_area[AI_21].append((83, 4, WARP_VINE_TOP, AI_CLOUD_DAY, 2, 10, 145))
    # 2-2 & 7-2 (ai=1): end side pipe
    AI_22 = area_idx_from_ptr(0x01)
    warps_by_area[AI_22].append((190, 6, WARP_SIDE_PIPE, AREA_EXIT_COURTYARD, 179, 9, 0))
    # 3-1 (ai=7)
    AI_31 = area_idx_from_ptr(0x24)
    warps_by_area[AI_31].append((38, 8, WARP_PIPE_DOWN, AI_BONUS_UG, 66, 2, 52))
    warps_by_area[AI_31].append((131, 4, WARP_VINE_TOP, AI_CLOUD_NGT, 2, 10, 158))
    # 4-1 (ai=5)
    AI_41 = area_idx_from_ptr(0x22)
    warps_by_area[AI_41].append((132, 8, WARP_PIPE_DOWN, AI_BONUS_UG, 98, 2, 148))
    # 4-2 (ai=26)
    AI_42 = area_idx_from_ptr(0x41)
    warps_by_area[AI_42].append((84, 9, WARP_PIPE_DOWN, AI_BONUS_UG, 130, 2, 98))
    warps_by_area[AI_42].append((64, 4, WARP_VINE_TOP, AI_WARP_42, 2, 10, 0))
    warps_by_area[AI_42].append((187, 6, WARP_SIDE_PIPE, AREA_EXIT_COURTYARD, 179, 9, 0))
    warps_by_area[AI_42].append((214, 9, WARP_TO_WORLD, 5, 2, 10, 0))
    # 5-1 (ai=13)
    AI_51 = area_idx_from_ptr(0x2A)
    warps_by_area[AI_51].append((156, 6, WARP_PIPE_DOWN, AI_BONUS_UG, 130, 2, 168))
    # 5-2 (ai=20)
    AI_52 = area_idx_from_ptr(0x31)
    warps_by_area[AI_52].append((55, 9, WARP_PIPE_DOWN, AI_WATER_52, 2, 3, 86))
    warps_by_area[AI_52].append((85, 4, WARP_VINE_TOP, AI_CLOUD_DAY, 2, 10, 130))
    # 6-2 (ai=6)
    AI_62 = area_idx_from_ptr(0x23)
    warps_by_area[AI_62].append((19, 8, WARP_PIPE_DOWN, AI_BONUS_UG, 130, 2, 36))
    warps_by_area[AI_62].append((56, 7, WARP_PIPE_DOWN, AI_WATER_52, 2, 3, 84))
    warps_by_area[AI_62].append((153, 9, WARP_PIPE_DOWN, AI_BONUS_UG, 98, 2, 165))
    warps_by_area[AI_62].append((81, 4, WARP_VINE_TOP, AI_CLOUD_NGT, 2, 10, 130))
    # 7-1 (ai=22)
    AI_71 = area_idx_from_ptr(0x33)
    warps_by_area[AI_71].append((93, 9, WARP_PIPE_DOWN, AI_BONUS_UG, 2, 2, 106))
    # 8-1 (ai=19)
    AI_81 = area_idx_from_ptr(0x30)
    warps_by_area[AI_81].append((104, 8, WARP_PIPE_DOWN, AI_BONUS_UG, 34, 2, 116))
    # 8-2 (ai=21)
    AI_82 = area_idx_from_ptr(0x32)
    warps_by_area[AI_82].append((156, 8, WARP_PIPE_DOWN, AI_BONUS_UG, 130, 2, 168))
    # 8-4 (ai=33) castle maze + underwater room (ai=2)
    warps_by_area[AI_CASTLE_84].append((51, 10, WARP_PIPE_DOWN, AI_CASTLE_84, 18, 9, 0))
    warps_by_area[AI_CASTLE_84].append((81, 7, WARP_PIPE_DOWN, AI_CASTLE_84, 114, 9, 0))
    warps_by_area[AI_CASTLE_84].append((132, 10, WARP_PIPE_DOWN, AI_CASTLE_84, 18, 9, 0))
    warps_by_area[AI_CASTLE_84].append((152, 5, WARP_PIPE_DOWN, AI_CASTLE_84, 194, 9, 0))
    warps_by_area[AI_CASTLE_84].append((212, 6, WARP_PIPE_DOWN, AI_CASTLE_84, 18, 9, 0))
    warps_by_area[AI_CASTLE_84].append((228, 7, WARP_PIPE_DOWN, AI_WATER_84, 3, 9, 0))
    warps_by_area[AI_WATER_84].append((68, 6, WARP_SIDE_PIPE, AI_CASTLE_84, 258, 9, 0))

    rle_blob = []
    area_recs = []
    all_enemies = []
    all_specials = []
    all_warps = []

    for ai in range(len(area_names)):
        at, a_name, e_name = ptr_map[ai]
        dec = decode_area(labels[a_name], at, is_world8=(ai in (2, 19, 33)), no_piranha=(ai in (AI_STAGE_11, AI_WARP_42)))
        ens, specs, _ = decode_enemies(labels[e_name], dec["width"], 0, dec["grid"])

        # Clear Piranha Plants past Warp Zone scroll lock on 1-2 (c >= 175) and 4-2 (c >= 195)
        pir_list = []
        for pc, pr in dec["piranhas"]:
            if ai == AI_12 and pc >= 175:
                continue
            if ai == AI_42 and pc >= 195:
                continue
            pir_list.append((pc, pr, EN_PIRANHA))

        merged_ens = sorted(ens + pir_list, key=lambda x: (x[0], x[1]))
        merged_specs = sorted(specs, key=lambda x: (x[0], x[1]))

        # Validate every cannon, flagpole, ground integrity
        w = dec["width"]
        assert 20 <= w <= 420, (ai, a_name, w)

        flat = [t for r in range(14) for t in dec["grid"][r]]
        rle_off = len(rle_blob)
        i = 0
        while i < len(flat):
            v = flat[i]
            cnt = 1
            while i + cnt < len(flat) and flat[i + cnt] == v and cnt < 255:
                cnt += 1
            rle_blob.append(cnt)
            rle_blob.append(v)
            i += cnt
        rle_len = len(rle_blob) - rle_off

        en_off = len(all_enemies)
        all_enemies.extend(merged_ens)
        en_cnt = len(merged_ens)

        sp_off = len(all_specials)
        all_specials.extend(merged_specs)
        sp_cnt = len(merged_specs)

        w_list = warps_by_area.get(ai, [])
        wp_off = len(all_warps)
        all_warps.extend(w_list)
        wp_cnt = len(w_list)

        area_recs.append({
            "rle_off": rle_off,
            "rle_len": rle_len,
            "width": w,
            "area_type": at,
            "bg_scenery": dec["bg_scenery"],
            "bg_color": dec["bg_color"],
            "cloud": 1 if dec["cloud_override"] else 0,
            "flag_col": dec["flagpole"] if dec["flagpole"] is not None else 0xFFFF,
            "axe_col": dec["axe"] if dec["axe"] is not None else 0xFFFF,
            "timer": dec["timer"],
            "en_off": en_off,
            "en_cnt": en_cnt,
            "sp_off": sp_off,
            "sp_cnt": sp_cnt,
            "wp_off": wp_off,
            "wp_cnt": wp_cnt,
        })

    # Stage table (32 stages: World 1-1 .. 8-4)
    stage_recs = []
    for w in range(1, 9):
        ptrs = [p for p in world_areas[w] if p != 0x29]
        for s_idx, ptr in enumerate(ptrs, 1):
            ai = area_idx_from_ptr(ptr)
            night = 1 if (w in (3, 6)) else 0
            snow = 1 if ((w == 5 and s_idx in (1, 2)) or (w == 6 and s_idx == 3)) else 0
            stage_recs.append((w, s_idx, ai, night, snow, area_recs[ai]["bg_color"]))

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("#ifndef __MARIO_LEVELS_H\n#define __MARIO_LEVELS_H\n\n#include <stdint.h>\n\n")
        f.write("#define SMB_AREA_COUNT %dU\n" % len(area_recs))
        f.write("#define SMB_STAGE_COUNT %dU\n" % len(stage_recs))
        f.write("#define SMB_COURTYARD_AREA_ID %dU\n" % AI_STAGE_11)
        f.write("#define SMB_WARP_RET_SENTINEL 254U\n")
        f.write("#define SMB_WARP_EXIT_COURTYARD 253U\n\n")
        f.write("enum {\n"
                "    WARP_PIPE_DOWN = 1,\n"
                "    WARP_VINE_TOP = 2,\n"
                "    WARP_SIDE_PIPE = 3,\n"
                "    WARP_TO_WORLD = 4,\n"
                "    WARP_FALL_RETURN = 5\n"
                "};\n\n")
        f.write("typedef struct {\n"
                "    uint16_t rle_off;\n"
                "    uint16_t rle_len;\n"
                "    uint16_t width;\n"
                "    uint16_t flag_col;\n"
                "    uint16_t axe_col;\n"
                "    uint16_t timer;\n"
                "    uint16_t en_off;\n"
                "    uint16_t sp_off;\n"
                "    uint16_t wp_off;\n"
                "    uint8_t  area_type;\n"
                "    uint8_t  bg_scenery;\n"
                "    uint8_t  bg_color;\n"
                "    uint8_t  cloud;\n"
                "    uint8_t  en_cnt;\n"
                "    uint8_t  sp_cnt;\n"
                "    uint8_t  wp_cnt;\n"
                "} SmbAreaDesc;\n\n")
        f.write("typedef struct {\n"
                "    uint16_t col;\n"
                "    uint8_t  row;\n"
                "    uint8_t  kind;\n"
                "} SmbEnemySpawn;\n\n")
        f.write("typedef struct {\n"
                "    uint16_t col;\n"
                "    uint8_t  row;\n"
                "    uint8_t  kind;\n"
                "    uint8_t  arg;\n"
                "} SmbSpecialObj;\n\n")
        f.write("typedef struct {\n"
                "    uint16_t src_col;\n"
                "    uint8_t  src_row;\n"
                "    uint8_t  kind;\n"
                "    uint8_t  dest_area;\n"
                "    uint16_t dest_col;\n"
                "    uint8_t  dest_row;\n"
                "    uint16_t ret_col;\n"
                "} SmbWarpDef;\n\n")
        f.write("typedef struct {\n"
                "    uint8_t world;\n"
                "    uint8_t stage;\n"
                "    uint8_t area_id;\n"
                "    uint8_t night;\n"
                "    uint8_t snow;\n"
                "} SmbStageInfo;\n\n")

        f.write("static const uint8_t SMB_AREA_RLE[%d] = {\n" % len(rle_blob))
        for off in range(0, len(rle_blob), 24):
            chunk = ", ".join("0x%02x" % b for b in rle_blob[off:off + 24])
            f.write("    %s,\n" % chunk)
        f.write("};\n\n")

        f.write("static const SmbEnemySpawn SMB_ENEMIES[%d] = {\n" % len(all_enemies))
        for col, row, kind in all_enemies:
            f.write("    { %3dU, %2dU, %2dU },\n" % (col, row, kind))
        f.write("};\n\n")

        f.write("static const SmbSpecialObj SMB_SPECIALS[%d] = {\n" % len(all_specials))
        for col, row, kind, arg in all_specials:
            f.write("    { %3dU, %2dU, %2dU, 0x%02xU },\n" % (col, row, kind, arg))
        f.write("};\n\n")

        f.write("static const SmbWarpDef SMB_WARPS[%d] = {\n" % len(all_warps))
        for sc, sr, k, da, dc, dr, rc in all_warps:
            f.write("    { %3dU, %2dU, %dU, %3dU, %3dU, %2dU, %3dU },\n" % (sc, sr, k, da, dc, dr, rc))
        f.write("};\n\n")

        f.write("static const SmbAreaDesc SMB_AREAS[%d] = {\n" % len(area_recs))
        for rec in area_recs:
            f.write("    { %5dU, %4dU, %3dU, %5dU, %5dU, %3dU, %3dU, %2dU, %2dU, %dU, %dU, %dU, %dU, %2dU, %2dU, %dU },\n" % (
                rec["rle_off"], rec["rle_len"], rec["width"], rec["flag_col"], rec["axe_col"], rec["timer"],
                rec["en_off"], rec["sp_off"], rec["wp_off"], rec["area_type"], rec["bg_scenery"], rec["bg_color"],
                rec["cloud"], rec["en_cnt"], rec["sp_cnt"], rec["wp_cnt"]
            ))
        f.write("};\n\n")

        f.write("static const SmbStageInfo SMB_STAGES[SMB_STAGE_COUNT] = {\n")
        for w, s_idx, ai, night, snow, _ in stage_recs:
            f.write("    { %dU, %dU, %2dU, %dU, %dU }, /* %d-%d */\n" % (w, s_idx, ai, night, snow, w, s_idx))
        f.write("};\n\n#endif\n")

    print("Generated %s: RLE=%d B, enemies=%d, specials=%d, warps=%d, stages=%d" % (
        out_path, len(rle_blob), len(all_enemies), len(all_specials), len(all_warps), len(stage_recs)
    ))


def main():
    if not os.path.exists(SMBDIS_PATH):
        print(f"Missing {SMBDIS_PATH}", file=sys.stderr)
        sys.exit(1)
    labels, world_areas, area_names, enemy_names = parse_smbdis_labels(SMBDIS_PATH)

    if len(sys.argv) >= 3 and sys.argv[1] == "--gen-header":
        emit_header(sys.argv[2], labels, world_areas, area_names, enemy_names)
        return

    print(f"Loaded {len(area_names)} area defs, {len(enemy_names)} enemy defs")
    for w in range(1, 9):
        stages = [p for p in world_areas[w] if p != 0x29]
        assert len(stages) == 4, (w, ptrs)
        for s_idx, ptr in enumerate(stages, 1):
            at, ai, ei = area_ptr_to_indices(ptr)
            an = area_names[ai]
            en = enemy_names[ei]
            dec = decode_area(labels[an], at, is_world8=(w == 8), no_piranha=(w == 1 and s_idx == 1))
            ens, specs, trans = decode_enemies(labels[en], dec["width"], w - 1)
            total_pir = len(dec["piranhas"])
            print(f"  {w}-{s_idx} (ptr=0x{ptr:02x} {an}/{en}): width={dec['width']:3d} theme={at} "
                  f"piranha_pipes={total_pir:2d} enemies={len(ens):2d} specs={len(specs):2d} "
                  f"flag={dec['flagpole']} axe={dec['axe']} trans={trans}")


if __name__ == "__main__":
    main()
