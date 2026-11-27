#!/usr/bin/env python3
"""PC Wi-Fi Multiplayer Client (2P-4P) for ZHIoT STM32 Super Mario.

Joins the ESP8285 SoftAP ("MARIO-MP", password "12345678", host 192.168.4.1:9000)
over UDP port 9000 using the exact WifiClientPkt (20B) / WifiHostSnap (58B) wire
protocol from Inc/wifi_net.h & Src/wifi_net.c.

Usage:
    1. Connect PC Wi-Fi to SSID "MARIO-MP" (pass: 12345678).
    2. Start Host on an STM32 board (press K1 on Title screen -> WIFI LOBBY).
    3. Run:  python3 pc_mp_client.py [--host 192.168.4.1] [--port 9000]
    4. Press K3 on Host STM32 to start the 2-4P game!

Self-test (no Wi-Fi switch required):
    python3 pc_mp_client.py --self-test
"""

import os
import random
import re
import socket
import struct
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import pc_keyboard_ctrl as ctrl  # reuse sprite loader, palette & constants

WIFI_UDP_PORT = 9000
DEFAULT_HOST_IP = "192.168.4.1"

MAGIC_HOST_SNAP = 0x4D48  # 'MH'
MAGIC_CLIENT_PKT = 0x4D43  # 'MC'

NET_EV_NONE = 0
NET_EV_TILE = 1
NET_EV_ENEMY_KILL = 2
NET_EV_STAGE_CLEAR = 3
NET_EV_COIN_SCORE = 4
NET_EV_BOSS_AXE = 5
NET_EV_WARP = 6

LEVELS_H_PATH = os.path.join(SCRIPT_DIR, "Inc", "mario_levels.h")

MAP_ROWS = ctrl.MAP_ROWS
MAP_COLS = ctrl.MAP_COLS
TILE_SIZE = ctrl.TILE_SIZE
SCREEN_W = ctrl.SCREEN_W
SCREEN_H = ctrl.SCREEN_H
HUD_H = ctrl.HUD_H
SCALE = ctrl.SCALE

HOST_SNAP_FMT = "<HBBBBHI4I4(hhBB)"  # wait: Python struct doesn't support nested parens


def xor_checksum(data: bytes) -> int:
    c = 0
    for b in data:
        c ^= b
    return c


def pack_client_pkt(
    seq: int,
    uid: int,
    x: int,
    y: int,
    spr: int,
    flags: int,
    ev_seq: int = 0,
    ev_type: int = 0,
    ev_row: int = 0,
    ev_col: int = 0,
    ev_val: int = 0,
) -> bytes:
    body = struct.pack(
        "<HBBIhhBBBBBBB",
        MAGIC_CLIENT_PKT,
        seq & 0xFF,
        0,
        uid & 0xFFFFFFFF,
        int(x),
        int(y),
        spr & 0xFF,
        flags & 0xFF,
        ev_seq & 0xFF,
        ev_type & 0xFF,
        ev_row & 0xFF,
        ev_col & 0xFF,
        ev_val & 0xFF,
    )
    return body + bytes([xor_checksum(body)])


def unpack_client_pkt(raw: bytes):
    if len(raw) != 20:
        return None
    if xor_checksum(raw[:19]) != raw[19]:
        return None
    fields = struct.unpack("<HBBIhhBBBBBBBB", raw)
    if fields[0] != MAGIC_CLIENT_PKT:
        return None
    return {
        "seq": fields[1],
        "uid": fields[3],
        "x": fields[4],
        "y": fields[5],
        "spr": fields[6],
        "flags": fields[7],
        "ev_seq": fields[8],
        "ev_type": fields[9],
        "ev_row": fields[10],
        "ev_col": fields[11],
        "ev_val": fields[12],
    }


def pack_host_snap(
    seq: int,
    room_state: int,
    level: int,
    lives: int,
    coins: int,
    score: int,
    slot_uid,
    players,
    ev_seq: int = 0,
    ev_type: int = 0,
    ev_row: int = 0,
    ev_col: int = 0,
    ev_val: int = 0,
) -> bytes:
    parts = [
        struct.pack(
            "<HBBBBHI",
            MAGIC_HOST_SNAP,
            seq & 0xFF,
            room_state & 0xFF,
            level & 0xFF,
            lives & 0xFF,
            coins & 0xFFFF,
            score & 0xFFFFFFFF,
        ),
        struct.pack("<4I", *(u & 0xFFFFFFFF for u in slot_uid)),
    ]
    for px, py, pspr, pflags in players:
        parts.append(struct.pack("<hhBB", int(px), int(py), pspr & 0xFF, pflags & 0xFF))
    parts.append(struct.pack("<BBBBB", ev_seq & 0xFF, ev_type & 0xFF, ev_row & 0xFF, ev_col & 0xFF, ev_val & 0xFF))
    body = b"".join(parts)
    return body + bytes([xor_checksum(body)])


def unpack_host_snap(raw: bytes):
    if len(raw) != 58:
        return None
    if xor_checksum(raw[:57]) != raw[57]:
        return None
    magic, seq, room_state, level, lives, coins, score = struct.unpack_from("<HBBBBHI", raw, 0)
    if magic != MAGIC_HOST_SNAP:
        return None
    slot_uid = list(struct.unpack_from("<4I", raw, 12))
    players = []
    off = 28
    for _ in range(4):
        px, py, pspr, pflags = struct.unpack_from("<hhBB", raw, off)
        players.append((px, py, pspr, pflags))
        off += 6
    ev_seq, ev_type, ev_row, ev_col, ev_val = struct.unpack_from("<BBBBB", raw, off)
    return {
        "seq": seq,
        "room_state": room_state,
        "level": level,
        "lives": lives,
        "coins": coins,
        "score": score,
        "slot_uid": slot_uid,
        "players": players,
        "ev_seq": ev_seq,
        "ev_type": ev_type,
        "ev_row": ev_row,
        "ev_col": ev_col,
        "ev_val": ev_val,
    }


_SMB_CACHE = None


def _load_levels_from_header():
    global _SMB_CACHE
    if _SMB_CACHE is not None:
        return _SMB_CACHE
    txt = open(LEVELS_H_PATH, "r", encoding="utf-8").read()
    rle = [int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]+", re.search(r"SMB_AREA_RLE\[.*?\]\s*=\s*\{(.*?)\};", txt, re.S).group(1))]
    area_rows = [
        [int(n) for n in re.findall(r"(\d+)U", ln)]
        for ln in re.search(r"SMB_AREAS\[.*?\]\s*=\s*\{(.*?)\};", txt, re.S).group(1).strip().splitlines()
        if ln.strip()
    ]
    stage_rows = [
        [int(n) for n in re.findall(r"(\d+)U", ln)]
        for ln in re.search(r"SMB_STAGES\[SMB_STAGE_COUNT\]\s*=\s*\{(.*?)\};", txt, re.S).group(1).strip().splitlines()
        if ln.strip()
    ]
    _SMB_CACHE = (rle, area_rows, stage_rows)
    return _SMB_CACHE


def get_stage_meta(lvl: int):
    _rle, area_rows, stage_rows = _load_levels_from_header()
    st_idx = max(0, min(len(stage_rows) - 1, int(lvl) - 1))
    w, s, area_id, night, snow = stage_rows[st_idx]
    ar = area_rows[area_id]
    return {"world": w, "stage": s, "area_id": area_id, "night": night, "snow": snow,
            "theme": ar[9], "bg_scenery": ar[10], "cols": ar[2]}


def build_level_map(lvl: int):
    """Build tile map for stage 1..32 directly from Inc/mario_levels.h."""
    rle, area_rows, stage_rows = _load_levels_from_header()
    st_idx = max(0, min(len(stage_rows) - 1, int(lvl) - 1))
    area_id = stage_rows[st_idx][2]
    ar = area_rows[area_id]
    roff, rlen, cols = ar[0], ar[1], ar[2]
    flat = []
    for i in range(0, rlen, 2):
        flat.extend([rle[roff + i + 1]] * rle[roff + i])
    return [flat[r * cols:(r + 1) * cols] for r in range(MAP_ROWS)]


def is_solid(t: int) -> bool:
    return t in (
        ctrl.T_GROUND, ctrl.T_BRICK, ctrl.T_QBLOCK, ctrl.T_USED,
        ctrl.T_PIPE_TL, ctrl.T_PIPE_TR, ctrl.T_PIPE_BL, ctrl.T_PIPE_BR,
        ctrl.T_PIPE_WARP_TL, ctrl.T_PIPE_WARP_TR, ctrl.T_STONE,
        ctrl.T_QBLOCK_MUSH, ctrl.T_QBLOCK_FLOWER,
        ctrl.T_BRICK_POWER, ctrl.T_BRICK_VINE, ctrl.T_BRICK_STAR,
        ctrl.T_BRICK_10COIN, ctrl.T_BRICK_1UP, ctrl.T_BRIDGE,
        ctrl.T_CANNON_TOP, ctrl.T_CANNON_MID, ctrl.T_CANNON_BOT,
        ctrl.T_SPRING_TOP, ctrl.T_SPRING_BOT, ctrl.T_CLOUD_PLAT,
        ctrl.T_MUSH_PLAT_L, ctrl.T_MUSH_PLAT_M, ctrl.T_MUSH_PLAT_R,
        ctrl.T_SIDE_PIPE_T, ctrl.T_SIDE_PIPE_B,
    )


class UdpClientSession:
    def __init__(self, host_ip: str = DEFAULT_HOST_IP, port: int = WIFI_UDP_PORT, bind_port: int = WIFI_UDP_PORT):
        self.host_ip = host_ip
        self.port = port
        self.uid = random.randint(0x10000001, 0x7FFFFFFF)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.bind(("0.0.0.0", bind_port))
        self.sock.setblocking(False)
        self.tx_seq = 0
        self.my_slot = None
        self.last_snap = None
        self.last_snap_ms = 0.0
        self.last_ev_seq = 0
        self.out_ev_seq = 0
        self.out_ev_type = 0
        self.out_ev_row = 0
        self.out_ev_col = 0
        self.out_ev_val = 0
        self.out_ev_repeat = 0
        self.last_tx_ms = 0.0

    def push_event(self, ev_type: int, row: int = 0, col: int = 0, val: int = 0):
        self.out_ev_seq = (self.out_ev_seq + 1) & 0xFF
        if self.out_ev_seq == 0:
            self.out_ev_seq = 1
        self.out_ev_type = ev_type
        self.out_ev_row = row
        self.out_ev_col = col
        self.out_ev_val = val
        self.out_ev_repeat = 8

    def poll_rx(self):
        events = []
        while True:
            try:
                data, _addr = self.sock.recvfrom(256)
            except BlockingIOError:
                break
            except OSError:
                break
            snap = unpack_host_snap(data)
            if snap is None:
                continue
            self.last_snap = snap
            self.last_snap_ms = time.monotonic()
            for idx, suid in enumerate(snap["slot_uid"]):
                if suid == self.uid:
                    self.my_slot = idx
                    break
            ev_seq = snap["ev_seq"]
            if ev_seq != 0 and ev_seq != self.last_ev_seq:
                self.last_ev_seq = ev_seq
                events.append((snap["ev_type"], snap["ev_row"], snap["ev_col"], snap["ev_val"]))
        return events

    def maybe_send(self, x: int, y: int, spr: int, flags: int):
        now = time.monotonic()
        if (now - self.last_tx_ms) < 0.04:
            return
        self.last_tx_ms = now
        self.tx_seq = (self.tx_seq + 1) & 0xFF
        pkt = pack_client_pkt(
            self.tx_seq,
            self.uid,
            x,
            y,
            spr,
            flags,
            self.out_ev_seq if self.out_ev_repeat > 0 else 0,
            self.out_ev_type if self.out_ev_repeat > 0 else 0,
            self.out_ev_row if self.out_ev_repeat > 0 else 0,
            self.out_ev_col if self.out_ev_repeat > 0 else 0,
            self.out_ev_val if self.out_ev_repeat > 0 else 0,
        )
        if self.out_ev_repeat > 0:
            self.out_ev_repeat -= 1
        try:
            self.sock.sendto(pkt, (self.host_ip, self.port))
        except OSError:
            pass

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def try_spawn_fireball(world):
    fbs = world.setdefault("fireballs", [])
    if sum(1 for fb in fbs if fb[4] == 0) >= 2:
        return False
    fbs.append([
        (world["x_fp"] >> 8) + (10 if world["dir"] > 0 else -4),
        (world["y_fp"] >> 8) + 2,
        4 if world["dir"] > 0 else -4,
        1,
        0,
    ])
    return True


def step_player_fixed_point(world, held, push_ev=None, reset_cb=None):
    """Step 1 frame of 8.8 fixed-point Mario physics matching Src/mario_game.c."""
    if world.get("transform_ticks", 0) > 0:
        world["transform_ticks"] -= 1
        return

    tmap = world["tmap"]
    cur_cols = len(tmap[0])
    if "L" in held and "R" not in held:
        world["vx_fp"] = max(-640, world["vx_fp"] - 110)
        world["dir"] = -1
    elif "R" in held and "L" not in held:
        world["vx_fp"] = min(640, world["vx_fp"] + 110)
        world["dir"] = 1
    else:
        world["vx_fp"] = int((world["vx_fp"] * 3) / 4)
        if -30 < world["vx_fp"] < 30:
            world["vx_fp"] = 0

    j_held = "J" in held
    j_pressed = j_held and not world["prev_jump"]
    world["prev_jump"] = j_held

    if j_pressed and world["on_ground"]:
        world["vy_fp"] = -1240
        world["on_ground"] = False
        if world.get("power", 0) >= 2 and world.get("fire_cd", 0) == 0:
            if try_spawn_fireball(world):
                world["fire_cd"] = 16
    if not j_held and world["vy_fp"] < -210:
        world["vy_fp"] = -210
    if world.get("fire_cd", 0) > 0:
        world["fire_cd"] -= 1
    if "F" in held and world.get("power", 0) >= 2 and world.get("fire_cd", 0) == 0:
        if try_spawn_fireball(world):
            world["fire_cd"] = 16

    world["vy_fp"] = min(1980, world["vy_fp"] + (41 if world["vy_fp"] < 0 else 54))

    # Horizontal move + tile collision (Tall Mario uses 24px height: py-8..py+16)
    head_off = 8 if world.get("power", 0) >= 1 else 0
    world["x_fp"] += world["vx_fp"]
    px = world["x_fp"] >> 8
    py = world["y_fp"] >> 8
    if px < 0:
        px = 0
        world["x_fp"] = 0
        world["vx_fp"] = 0
    if px > cur_cols * TILE_SIZE - TILE_SIZE:
        px = cur_cols * TILE_SIZE - TILE_SIZE
        world["x_fp"] = px << 8
        world["vx_fp"] = 0

    r_top = (py - head_off + 2) // TILE_SIZE
    r_mid = (py + 6) // TILE_SIZE
    r_bot = (py + TILE_SIZE - 2) // TILE_SIZE
    if world["vx_fp"] > 0:
        c_right = (px + TILE_SIZE - 2) // TILE_SIZE
        if (0 <= r_top < MAP_ROWS and 0 <= c_right < cur_cols and is_solid(tmap[r_top][c_right])) or (
            0 <= r_mid < MAP_ROWS and 0 <= c_right < cur_cols and is_solid(tmap[r_mid][c_right])
        ) or (0 <= r_bot < MAP_ROWS and 0 <= c_right < cur_cols and is_solid(tmap[r_bot][c_right])):
            px = c_right * TILE_SIZE - (TILE_SIZE - 1)
            world["x_fp"] = px << 8
            world["vx_fp"] = 0
    elif world["vx_fp"] < 0:
        c_left = (px + 1) // TILE_SIZE
        if (0 <= r_top < MAP_ROWS and 0 <= c_left < cur_cols and is_solid(tmap[r_top][c_left])) or (
            0 <= r_mid < MAP_ROWS and 0 <= c_left < cur_cols and is_solid(tmap[r_mid][c_left])
        ) or (0 <= r_bot < MAP_ROWS and 0 <= c_left < cur_cols and is_solid(tmap[r_bot][c_left])):
            px = (c_left + 1) * TILE_SIZE - 1
            world["x_fp"] = px << 8
            world["vx_fp"] = 0

    # Vertical move + tile collision
    world["y_fp"] += world["vy_fp"]
    py = world["y_fp"] >> 8
    if py < head_off:
        py = head_off
        world["y_fp"] = py << 8
        world["vy_fp"] = 0
    c_l = (px + 3) // TILE_SIZE
    c_r = (px + TILE_SIZE - 4) // TILE_SIZE
    if world["vy_fp"] >= 0:
        r_foot = (py + TILE_SIZE) // TILE_SIZE
        if 0 <= r_foot < MAP_ROWS and (
            (0 <= c_l < cur_cols and is_solid(tmap[r_foot][c_l]))
            or (0 <= c_r < cur_cols and is_solid(tmap[r_foot][c_r]))
        ):
            py = r_foot * TILE_SIZE - TILE_SIZE
            world["y_fp"] = py << 8
            world["vy_fp"] = 0
            world["on_ground"] = True
        else:
            world["on_ground"] = False
    else:
        r_head = (py - head_off) // TILE_SIZE
        if 0 <= r_head < MAP_ROWS:
            hit_col = None
            if 0 <= c_l < cur_cols and is_solid(tmap[r_head][c_l]):
                hit_col = c_l
            elif 0 <= c_r < cur_cols and is_solid(tmap[r_head][c_r]):
                hit_col = c_r
            if hit_col is not None:
                t = tmap[r_head][hit_col]
                if t in (ctrl.T_QBLOCK, ctrl.T_QBLOCK_MUSH, ctrl.T_QBLOCK_FLOWER):
                    tmap[r_head][hit_col] = ctrl.T_USED
                    if push_ev:
                        ev_r = (r_head & 0x0F) | (((hit_col >> 8) & 1) << 4)
                        push_ev(NET_EV_TILE, ev_r, hit_col & 0xFF, ctrl.T_USED)
                        if t == ctrl.T_QBLOCK:
                            push_ev(NET_EV_COIN_SCORE, 1, 0, 2)
                    if t == ctrl.T_QBLOCK_MUSH:
                        kind = 2 if world.get("power", 0) >= 1 else 1
                        world.setdefault("items", []).append([hit_col * TILE_SIZE, r_head * TILE_SIZE, 1 if kind == 1 else 0, 0, kind, 8])
                    elif t == ctrl.T_QBLOCK_FLOWER:
                        world.setdefault("items", []).append([hit_col * TILE_SIZE, r_head * TILE_SIZE, 0, 0, 2, 8])
                elif t == ctrl.T_BRICK and world.get("power", 0) >= 1:
                    tmap[r_head][hit_col] = ctrl.T_EMPTY
                    if push_ev:
                        ev_r = (r_head & 0x0F) | (((hit_col >> 8) & 1) << 4)
                        push_ev(NET_EV_TILE, ev_r, hit_col & 0xFF, ctrl.T_EMPTY)
                        push_ev(NET_EV_COIN_SCORE, 0, 0, 1)
                py = (r_head + 1) * TILE_SIZE + head_off
                world["y_fp"] = py << 8
                world["vy_fp"] = 0

    if py > MAP_ROWS * TILE_SIZE:
        if reset_cb:
            reset_cb()
        return

    cx = (px + 8) // TILE_SIZE
    cy = (py + 8) // TILE_SIZE
    if 0 <= cy < MAP_ROWS and 0 <= cx < cur_cols:
        t = tmap[cy][cx]
        if t == ctrl.T_COIN:
            tmap[cy][cx] = ctrl.T_EMPTY
            if push_ev:
                ev_r = (cy & 0x0F) | (((cx >> 8) & 1) << 4)
                push_ev(NET_EV_TILE, ev_r, cx & 0xFF, ctrl.T_EMPTY)
                push_ev(NET_EV_COIN_SCORE, 1, 0, 1)
        elif t in (ctrl.T_FLAG_TOP, ctrl.T_FLAG_POLE, ctrl.T_AXE) and not world["clear_sent"]:
            world["clear_sent"] = True
            if push_ev:
                push_ev(NET_EV_STAGE_CLEAR, world["level"], 0, 0)

    cam = px - (SCREEN_W // 2 - TILE_SIZE // 2)
    world["cam_x"] = max(0, min(max(0, cur_cols * TILE_SIZE - SCREEN_W), cam))

    # Update items (Mushroom & Flower)
    kept_items = []
    for it in world.get("items", []):
        ix, iy, ivx, ivy, ikind, rise = it
        if rise > 0:
            iy -= 2
            rise -= 1
        elif ikind == 1:
            nx = ix + ivx
            mr = (iy + 8) // TILE_SIZE
            mc = (nx + TILE_SIZE - 1) // TILE_SIZE if ivx > 0 else (nx // TILE_SIZE)
            if nx <= 0 or (0 <= mr < MAP_ROWS and 0 <= mc < cur_cols and is_solid(tmap[mr][mc])):
                ivx = -ivx
            else:
                ix = nx
            ivy = min(5, ivy + 1)
            ny = iy + ivy
            fr = (ny + TILE_SIZE) // TILE_SIZE
            fc_l = (ix + 2) // TILE_SIZE
            fc_r = (ix + TILE_SIZE - 3) // TILE_SIZE
            if 0 <= fr < MAP_ROWS and ((0 <= fc_l < cur_cols and is_solid(tmap[fr][fc_l])) or (0 <= fc_r < cur_cols and is_solid(tmap[fr][fc_r]))):
                iy = fr * TILE_SIZE - TILE_SIZE
                ivy = 0
            else:
                iy = ny
                if iy > MAP_ROWS * TILE_SIZE:
                    continue
        if px + TILE_SIZE - 2 > ix + 1 and px + 2 < ix + TILE_SIZE - 1 and py + TILE_SIZE - 1 > iy + 1 and (py - head_off + 1) < iy + TILE_SIZE:
            world["prev_power"] = world.get("power", 0)
            if ikind == 2:
                world["power"] = 2
            elif world.get("power", 0) < 1:
                world["power"] = 1
            world["transform_ticks"] = 24
            if push_ev:
                push_ev(NET_EV_COIN_SCORE, 0, 0, 10)
            continue
        kept_items.append([ix, iy, ivx, ivy, ikind, rise])
    world["items"] = kept_items

    # Update fireballs (with 5-frame impact puff)
    kept_fbs = []
    for fb in world.get("fireballs", []):
        fx, fy, fvx, fvy, puff = fb
        if puff > 0:
            if puff > 1:
                kept_fbs.append([fx, fy, fvx, fvy, puff - 1])
            continue
        nx = fx + fvx
        fr = (fy + 8) // TILE_SIZE
        fc = (nx + 8) // TILE_SIZE
        if nx < cam - 16 or nx > cam + SCREEN_W + 16:
            continue
        if 0 <= fr < MAP_ROWS and 0 <= fc < cur_cols and is_solid(tmap[fr][fc]):
            kept_fbs.append([fx, fy, fvx, fvy, 5])
            continue
        fx = nx
        fvy = min(5, fvy + 1)
        ny = fy + fvy
        fbot = (ny + 12) // TILE_SIZE
        if fvy > 0 and 0 <= fbot < MAP_ROWS and 0 <= (fx + 8) // TILE_SIZE < cur_cols and is_solid(tmap[fbot][(fx + 8) // TILE_SIZE]):
            fy = fbot * TILE_SIZE - 12
            fvy = -4
        else:
            fy = ny
            if fy > MAP_ROWS * TILE_SIZE:
                continue
        kept_fbs.append([fx, fy, fvx, fvy, 0])
    world["fireballs"] = kept_fbs

    if world["vx_fp"] != 0:
        world["anim"] += 1
    else:
        world["anim"] = 0


def run_client_gui(host_ip: str, port: int):
    import tkinter as tk

    cols, pal_map, spr_defs = ctrl.load_sprites_from_header(ctrl.SPRITES_H_PATH)
    sess = UdpClientSession(host_ip=host_ip, port=port)

    root = tk.Tk()
    root.title(f"Super Mario STM32 - PC Wi-Fi Player ({host_ip}:{port})")
    cw = SCREEN_W * SCALE
    ch = SCREEN_H * SCALE
    root.geometry(f"{cw}x{ch + 34}")
    root.configure(bg="#11151C")
    root.resizable(False, False)

    canvas = tk.Canvas(root, width=cw, height=ch, bg="#000000", highlightthickness=0)
    canvas.pack()

    bar_var = tk.StringVar(value=f"UID=0x{sess.uid:08X} | Connecting to MARIO-MP ({host_ip}:{port}) | WASD/Arrows + Space")
    tk.Label(
        root,
        textvariable=bar_var,
        fg="#7EE787",
        bg="#11151C",
        font=("Monospace", 9),
        anchor="w",
        padx=8,
    ).pack(fill="x", pady=4)

    tk_sprites = [ctrl.make_tk_sprite(tk, spr_defs[name], pal_map, flip_x=False) for name in ctrl.SPRITE_ORDER]
    tk_fireball_flip = ctrl.make_tk_sprite(tk, spr_defs["SPR_FIREBALL"], pal_map, flip_x=True)
    tk_player_norm = []
    tk_player_flip = []
    tk_player_tall_norm = []
    tk_player_tall_flip = []
    for slot in range(5):
        spm = ctrl.build_slot_pal_map(cols, pal_map, slot)
        tk_player_norm.append([ctrl.make_tk_sprite(tk, spr_defs[ctrl.SPRITE_ORDER[i]], spm, flip_x=False) for i in range(4)])
        tk_player_flip.append([ctrl.make_tk_sprite(tk, spr_defs[ctrl.SPRITE_ORDER[i]], spm, flip_x=True) for i in range(4)])
        tk_player_tall_norm.append([ctrl.make_tk_sprite(tk, spr_defs[ctrl.SPRITE_ORDER[i]], spm, flip_x=False, dst_h=24) for i in range(4)])
        tk_player_tall_flip.append([ctrl.make_tk_sprite(tk, spr_defs[ctrl.SPRITE_ORDER[i]], spm, flip_x=True, dst_h=24) for i in range(4)])

    sky_day = ctrl.rgb565_to_hex(cols["COL_SKY_DAY"])
    sky_ngt = ctrl.rgb565_to_hex(cols["COL_SKY_NGT"])
    col_cloud = ctrl.rgb565_to_hex(cols["COL_CLOUD"])
    col_hill_lt = ctrl.rgb565_to_hex(cols["COL_HILL_LT"])
    col_hill_dk = ctrl.rgb565_to_hex(cols["COL_HILL_DK"])
    col_gold = ctrl.rgb565_to_hex(cols["COL_GOLD"])
    col_red = ctrl.rgb565_to_hex(cols["COL_RED"])

    held = set()
    pending_release = {}
    world = {
        "room_state": ctrl.STATE_LOBBY,
        "level": 1,
        "tmap": build_level_map(1),
        "x_fp": 64 << 8,
        "y_fp": 160 << 8,
        "vx_fp": 0,
        "vy_fp": 0,
        "dir": 1,
        "on_ground": False,
        "prev_jump": False,
        "power": 0,
        "fire_cd": 0,
        "items": [],
        "fireballs": [],
        "anim": 0,
        "cam_x": 0,
        "frame": 0,
        "clear_sent": False,
    }

    def reset_local_spawn(keep_power=True):
        slot = sess.my_slot if sess.my_slot is not None else 1
        world["x_fp"] = ((3 + slot) * TILE_SIZE) << 8
        world["y_fp"] = (10 * TILE_SIZE) << 8
        world["vx_fp"] = 0
        world["vy_fp"] = 0
        world["dir"] = 1
        world["on_ground"] = False
        world["prev_jump"] = False
        if not keep_power:
            world["power"] = 0
        world["fire_cd"] = 0
        world["items"] = []
        world["fireballs"] = []
        world["cam_x"] = 0
        world["clear_sent"] = False

    def map_key(ev):
        ks = (ev.keysym or "").lower()
        if ks in ("a", "left"):
            return "L"
        if ks in ("d", "right"):
            return "R"
        if ks in ("w", "up", "space"):
            return "J"
        if ks in ("f", "j", "shift_l", "shift_r"):
            return "F"
        return None

    def on_press(ev):
        ks = (ev.keysym or "").lower()
        if ks in ("escape", "q"):
            root.quit()
            return
        k = map_key(ev)
        if not k:
            return
        if k in pending_release:
            root.after_cancel(pending_release.pop(k))
        held.add(k)

    def finish_release(k):
        pending_release.pop(k, None)
        held.discard(k)

    def on_release(ev):
        k = map_key(ev)
        if not k:
            return
        if k in pending_release:
            root.after_cancel(pending_release.pop(k))
        pending_release[k] = root.after(25, lambda key=k: finish_release(key))

    def on_focus_out(_ev):
        for aid in list(pending_release.values()):
            root.after_cancel(aid)
        pending_release.clear()
        held.clear()

    def step_physics():
        step_player_fixed_point(world, held, push_ev=sess.push_event, reset_cb=reset_local_spawn)

    def tile_cid(t, frame):
        table = {
            ctrl.T_GROUND: ctrl.C_GROUND, ctrl.T_BRICK: ctrl.C_BRICK,
            ctrl.T_QBLOCK: ctrl.C_QBLOCK, ctrl.T_QBLOCK_MUSH: ctrl.C_QBLOCK,
            ctrl.T_QBLOCK_FLOWER: ctrl.C_QBLOCK, ctrl.T_BRICK_POWER: ctrl.C_BRICK,
            ctrl.T_BRICK_VINE: ctrl.C_BRICK, ctrl.T_BRICK_STAR: ctrl.C_BRICK,
            ctrl.T_BRICK_10COIN: ctrl.C_BRICK, ctrl.T_BRICK_1UP: ctrl.C_BRICK,
            ctrl.T_USED: ctrl.C_USED, ctrl.T_PIPE_TL: ctrl.C_PIPE_TL,
            ctrl.T_PIPE_WARP_TL: ctrl.C_PIPE_TL, ctrl.T_PIPE_TR: ctrl.C_PIPE_TR,
            ctrl.T_PIPE_WARP_TR: ctrl.C_PIPE_TR, ctrl.T_PIPE_BL: ctrl.C_PIPE_BL,
            ctrl.T_PIPE_BR: ctrl.C_PIPE_BR,
            ctrl.T_COIN: ctrl.C_COIN2 if ((frame // 10) & 1) else ctrl.C_COIN1,
            ctrl.T_FLAG_TOP: ctrl.C_FLAG_TOP, ctrl.T_FLAG_POLE: ctrl.C_FLAG_POLE,
            ctrl.T_STONE: ctrl.C_STONE, ctrl.T_BRIDGE: ctrl.C_BRIDGE,
            ctrl.T_WATER_SURF: ctrl.C_WATER_SURF, ctrl.T_WATER_FILL: ctrl.C_WATER_FILL,
            ctrl.T_LAVA_SURF: ctrl.C_LAVA_SURF, ctrl.T_LAVA_FILL: ctrl.C_LAVA_FILL,
            ctrl.T_CANNON_TOP: ctrl.C_CANNON_TOP, ctrl.T_CANNON_MID: ctrl.C_CANNON_BOT,
            ctrl.T_CANNON_BOT: ctrl.C_CANNON_BOT, ctrl.T_AXE: ctrl.C_AXE,
            ctrl.T_CHAIN: ctrl.C_CHAIN, ctrl.T_SPRING_TOP: ctrl.C_SPRING_UP,
            ctrl.T_SPRING_BOT: ctrl.C_SPRING_DN, ctrl.T_CLOUD_PLAT: ctrl.C_CLOUD_PLAT,
            ctrl.T_MUSH_PLAT_L: ctrl.C_MUSH_L, ctrl.T_MUSH_PLAT_M: ctrl.C_MUSH_M,
            ctrl.T_MUSH_PLAT_R: ctrl.C_MUSH_R, ctrl.T_MUSH_STEM: ctrl.C_MUSH_STEM,
            ctrl.T_SIDE_PIPE_T: ctrl.C_SIDE_PIPE_T, ctrl.T_SIDE_PIPE_B: ctrl.C_SIDE_PIPE_B,
            ctrl.T_CASTLE_WALL: ctrl.C_CASTLE_WALL, ctrl.T_CASTLE_ARCH: ctrl.C_CASTLE_ARCH,
            ctrl.T_CASTLE_BATT: ctrl.C_CASTLE_BATT, ctrl.T_ROPE: ctrl.C_ROPE,
        }
        return table.get(t, ctrl.C_STONE)

    def render():
        frame = world["frame"]
        snap = sess.last_snap
        cam_x = world["cam_x"]
        meta = get_stage_meta(world["level"])
        th = meta["theme"]
        night = meta["night"]
        canvas.delete("all")
        if th == 0:
            bg = ctrl.rgb565_to_hex(cols["COL_UW_BG"])
        elif th in (2, 3):
            bg = "#000000"
        elif night:
            bg = "#080818"
        else:
            bg = sky_day
        canvas.create_rectangle(0, HUD_H * SCALE, cw, ch, fill=bg, outline="")

        if th == 1 and meta["bg_scenery"] > 0:
            c_scroll = (cam_x // 3) % 144
            for k in range(-1, 4):
                cx = k * 144 - c_scroll + 36
                cy = HUD_H + 39
                canvas.create_oval((cx - 24) * SCALE, (cy + 5) * SCALE, (cx + 24) * SCALE, (cy + 19) * SCALE, fill=col_cloud if not night else "#4A4E5A", outline="")
                canvas.create_oval((cx - 14) * SCALE, cy * SCALE, (cx + 14) * SCALE, (cy + 16) * SCALE, fill=col_cloud if not night else "#4A4E5A", outline="")
            h_scroll = (cam_x // 2) % 192
            bot_y = (HUD_H + 192) * SCALE
            top_y = (HUD_H + 164) * SCALE
            hl = "#006818" if night else col_hill_lt
            for k in range(-1, 3):
                hx = k * 192 - h_scroll + 36
                canvas.create_polygon((hx - 37) * SCALE, bot_y, (hx - 10) * SCALE, top_y + 4 * SCALE, (hx + 10) * SCALE, top_y + 4 * SCALE, (hx + 37) * SCALE, bot_y, fill=hl, outline="")

        tmap = world["tmap"]
        cur_cols = len(tmap[0])
        col_s = max(0, cam_x // TILE_SIZE)
        col_e = min(cur_cols - 1, (cam_x + SCREEN_W) // TILE_SIZE + 1)
        for r in range(MAP_ROWS):
            wy = (HUD_H + r * TILE_SIZE) * SCALE
            for c in range(col_s, col_e + 1):
                t = tmap[r][c]
                if t not in (ctrl.T_EMPTY, ctrl.T_QBLOCK_HIDDEN_COIN, ctrl.T_QBLOCK_HIDDEN_1UP):
                    cid = ctrl.C_GROUND_SUB if (t == ctrl.T_GROUND and r > 0 and tmap[r - 1][c] == ctrl.T_GROUND) else tile_cid(t, frame)
                    canvas.create_image((c * TILE_SIZE - cam_x) * SCALE, wy, anchor="nw", image=tk_sprites[cid])

        st = world["room_state"]
        for ix, iy, _ivx, _ivy, ikind, _rise in world["items"]:
            canvas.create_image((ix - cam_x) * SCALE, (HUD_H + iy) * SCALE, anchor="nw",
                                image=tk_sprites[ctrl.C_FLOWER if ikind == 2 else ctrl.C_MUSHROOM])
        for fx, fy, _fvx, _fvy, puff in world["fireballs"]:
            fimg = tk_sprites[ctrl.C_FIRE_PUFF] if puff > 0 else (tk_fireball_flip if ((frame // 3) & 1) else tk_sprites[ctrl.C_FIREBALL])
            canvas.create_image((fx - cam_x) * SCALE, (HUD_H + fy) * SCALE, anchor="nw", image=fimg)

        if snap and st not in (ctrl.STATE_TITLE, ctrl.STATE_LOBBY):
            for idx, (rx, ry, rspr, rflags) in enumerate(snap["players"]):
                if idx == sess.my_slot or snap["slot_uid"][idx] == 0:
                    continue
                if rflags & 0x02:
                    rsid = rspr if rspr < 4 else 0
                    rpwr = (rflags >> 3) & 0x03
                    pslot = 4 if rpwr >= 2 else idx
                    if rpwr >= 1:
                        img = tk_player_tall_flip[pslot][rsid] if (rflags & 0x01) else tk_player_tall_norm[pslot][rsid]
                        y_off = -8
                    else:
                        img = tk_player_flip[pslot][rsid] if (rflags & 0x01) else tk_player_norm[pslot][rsid]
                        y_off = 0
                    sx = (rx - cam_x) * SCALE
                    sy = (HUD_H + ry + y_off) * SCALE
                    canvas.create_image(sx, sy, anchor="nw", image=img)
                    canvas.create_text(sx + 16, sy - 6, text=f"{idx + 1}P", fill=ctrl.PLAYER_TAG_HEX[idx], font=("Monospace", 8, "bold"))

            mslot = sess.my_slot if sess.my_slot is not None else 1
            spr = 3 if not world["on_ground"] else ((1 if ((world["anim"] // 4) & 1) else 2) if world["vx_fp"] != 0 else 0)
            vis_pwr = world.get("prev_power", 0) if (world.get("transform_ticks", 0) > 0 and ((frame // 3) & 1)) else world.get("power", 0)
            pslot = 4 if vis_pwr >= 2 else mslot
            if vis_pwr >= 1:
                img = tk_player_tall_flip[pslot][spr] if (world["dir"] < 0) else tk_player_tall_norm[pslot][spr]
                y_off = -8
            else:
                img = tk_player_flip[pslot][spr] if (world["dir"] < 0) else tk_player_norm[pslot][spr]
                y_off = 0
            sx = ((world["x_fp"] >> 8) - cam_x) * SCALE
            sy = (HUD_H + (world["y_fp"] >> 8) + y_off) * SCALE
            canvas.create_image(sx, sy, anchor="nw", image=img)
            canvas.create_text(sx + 16, sy - 6, text=f"{mslot + 1}P", fill=ctrl.PLAYER_TAG_HEX[mslot], font=("Monospace", 8, "bold"))

        canvas.create_rectangle(0, 0, cw, HUD_H * SCALE, fill="#000000", outline="")
        if snap:
            cnt = sum(1 for u in snap["slot_uid"] if u != 0)
            pnum = (sess.my_slot + 1) if sess.my_slot is not None else 0
            pwr_s = "FIR " if world.get("power", 0) >= 2 else ("BIG " if world.get("power", 0) == 1 else "")
            wn = (snap["level"] - 1) // 4 + 1
            sn = (snap["level"] - 1) % 4 + 1
            hud = f"WORLD {wn}-{sn}  P{pnum}/{cnt}  MARIO {snap['score']:06d}  *{snap['coins']:02d}  L:{snap['lives']} {pwr_s}PC"
        else:
            hud = f"PC CLIENT -> SEARCHING HOST {host_ip}:{port}..."
        canvas.create_text(10, (HUD_H * SCALE) // 2, anchor="w", text=hud, fill="#FFFFFF", font=("Monospace", 10, "bold"))

        if snap is None or st == ctrl.STATE_LOBBY:
            canvas.create_text(cw // 2, (HUD_H + 40) * SCALE, text="PC WIFI CLIENT [MARIO-MP]", fill=col_gold, font=("Monospace", 14, "bold"))
            if snap is None:
                canvas.create_text(cw // 2, (HUD_H + 80) * SCALE, text="WAITING FOR HOST UDP :9000...", fill="#FFFFFF", font=("Monospace", 11, "bold"))
            else:
                canvas.create_text(cw // 2, (HUD_H + 74) * SCALE, text=f"ASSIGNED SLOT: {sess.my_slot + 1}P" if sess.my_slot is not None else "JOINING...", fill="#FFFFFF", font=("Monospace", 12, "bold"))
                p_line = "  ".join(f"{i + 1}P:{'OK' if snap['slot_uid'][i] else '--'}" for i in range(4))
                canvas.create_text(cw // 2, (HUD_H + 106) * SCALE, text=p_line, fill=col_gold, font=("Monospace", 12, "bold"))
                canvas.create_text(cw // 2, (HUD_H + 138) * SCALE, text="WAIT FOR STM32 HOST TO PRESS K3", fill="#FFFFFF", font=("Monospace", 11, "bold"))
        elif st == ctrl.STATE_GAME_OVER:
            canvas.create_text(cw // 2, (HUD_H + 106) * SCALE, text="GAME OVER", fill=col_red, font=("Monospace", 22, "bold"))
        elif st == ctrl.STATE_LEVEL_CLEAR:
            canvas.create_text(cw // 2, (HUD_H + 106) * SCALE, text="STAGE CLEAR!", fill=col_gold, font=("Monospace", 22, "bold"))
        elif st == ctrl.STATE_WIN:
            canvas.create_text(cw // 2, (HUD_H + 90) * SCALE, text="YOU WIN!", fill=col_gold, font=("Monospace", 22, "bold"))

    def tick():
        world["frame"] += 1
        cur_cols = len(world["tmap"][0])
        for ev_type, ev_r_raw, ev_c_lo, val in sess.poll_rx():
            r = ev_r_raw & 0x0F
            c = ev_c_lo | (((ev_r_raw >> 4) & 1) << 8)
            if ev_type == NET_EV_TILE and 0 <= r < MAP_ROWS and 0 <= c < cur_cols:
                world["tmap"][r][c] = val
        snap = sess.last_snap
        if snap is not None:
            if snap["level"] != world["level"] or (
                snap["room_state"] == ctrl.STATE_PLAYING and world["room_state"] != ctrl.STATE_PLAYING
            ):
                world["level"] = snap["level"] if 1 <= snap["level"] <= 32 else 1
                world["tmap"] = build_level_map(world["level"])
                meta = get_stage_meta(world["level"])
                reset_local_spawn(keep_power=(snap["room_state"] == ctrl.STATE_PLAYING and world["room_state"] == ctrl.STATE_LEVEL_CLEAR))
                if meta["theme"] == 3:
                    world["x_fp"] = (2 * TILE_SIZE) << 8
                    world["y_fp"] = (5 * TILE_SIZE) << 8
            world["room_state"] = snap["room_state"]

        if world["room_state"] == ctrl.STATE_PLAYING:
            step_physics()

        spr = 3 if not world["on_ground"] else ((1 if ((world["anim"] // 4) & 1) else 2) if world["vx_fp"] != 0 else 0)
        flags = (0x01 if world["dir"] < 0 else 0x00) | 0x06 | ((world.get("power", 0) & 0x03) << 3)
        sess.maybe_send(world["x_fp"] >> 8, world["y_fp"] >> 8, spr, flags)
        render()
        if (world["frame"] % 30) == 0:
            age = (time.monotonic() - sess.last_snap_ms) if sess.last_snap_ms else 99.9
            link = f"OK {age:.1f}s" if age < 2.0 else "SEARCH"
            slot_s = f"{sess.my_slot + 1}P" if sess.my_slot is not None else "--"
            bar_var.set(f"Host {host_ip} | {link} | {slot_s} | AD=Move W/Spc=Jump F=Fire Esc=Quit")
        root.after(16, tick)

    root.bind("<KeyPress>", on_press)
    root.bind("<KeyRelease>", on_release)
    root.bind("<FocusOut>", on_focus_out)
    root.after(16, tick)
    root.focus_force()
    try:
        root.mainloop()
    finally:
        sess.close()


def run_self_test():
    # 1) Packet encode/decode round-trip + XOR checksum validation
    cpkt = pack_client_pkt(7, 0x12345678, 120, 144, 2, 0x07, 3, NET_EV_TILE, 8, 10, ctrl.T_USED)
    assert len(cpkt) == 20
    cup = unpack_client_pkt(cpkt)
    assert cup == {
        "seq": 7, "uid": 0x12345678, "x": 120, "y": 144, "spr": 2, "flags": 7,
        "ev_seq": 3, "ev_type": NET_EV_TILE, "ev_row": 8, "ev_col": 10, "ev_val": ctrl.T_USED,
    }
    bad_cpkt = bytearray(cpkt)
    bad_cpkt[-1] ^= 0xFF
    assert unpack_client_pkt(bytes(bad_cpkt)) is None

    hs = pack_host_snap(
        12, ctrl.STATE_PLAYING, 1, 3, 5, 1200,
        [0xAAAA0001, 0x12345678, 0, 0],
        [(48, 160, 0, 6), (120, 144, 2, 7), (0, 0, 0, 0), (0, 0, 0, 0)],
        4, NET_EV_STAGE_CLEAR, 1, 0, 0,
    )
    assert len(hs) == 58
    hup = unpack_host_snap(hs)
    assert hup is not None and hup["score"] == 1200 and hup["slot_uid"][1] == 0x12345678
    assert hup["players"][1] == (120, 144, 2, 7)

    # 2) Real localhost UDP socket handshake between simulated Host & UdpClientSession
    host_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    host_sock.bind(("127.0.0.1", 0))
    host_port = host_sock.getsockname()[1]

    sess = UdpClientSession(host_ip="127.0.0.1", port=host_port, bind_port=0)
    client_port = sess.sock.getsockname()[1]
    sess.last_tx_ms = 0.0
    sess.push_event(NET_EV_TILE, 8, 10, ctrl.T_USED)
    sess.maybe_send(96, 160, 1, 0x06)

    data, addr = host_sock.recvfrom(128)
    decoded = unpack_client_pkt(data)
    assert decoded is not None and decoded["uid"] == sess.uid and decoded["ev_type"] == NET_EV_TILE
    reply = pack_host_snap(
        1, ctrl.STATE_PLAYING, 1, 3, 1, 200,
        [0xCAFE0001, sess.uid, 0, 0],
        [(50, 160, 0, 6), (96, 160, 1, 6), (0, 0, 0, 0), (0, 0, 0, 0)],
        9, NET_EV_TILE, 8, 14, ctrl.T_EMPTY,
    )
    host_sock.sendto(reply, ("127.0.0.1", client_port))
    time.sleep(0.02)
    evs = sess.poll_rx()
    assert sess.my_slot == 1
    assert evs == [(NET_EV_TILE, 8, 14, ctrl.T_EMPTY)]
    sess.close()
    host_sock.close()

    # 3) Variable jump height parity test (short tap vs full hold clearing 4-tile 64px pipes)
    def measure_jump(hold_frames):
        w = {
            "level": 1, "tmap": build_level_map(1),
            "x_fp": (3 * TILE_SIZE) << 8, "y_fp": (12 * TILE_SIZE) << 8,
            "vx_fp": 0, "vy_fp": 0, "dir": 1, "on_ground": True,
            "prev_jump": False, "anim": 0, "cam_x": 0, "clear_sent": False,
        }
        start_y = w["y_fp"] >> 8
        min_y = start_y
        for f in range(80):
            h = {"J"} if f < hold_frames else set()
            step_player_fixed_point(w, h)
            min_y = min(min_y, w["y_fp"] >> 8)
            if f > 2 and w["on_ground"]:
                break
        return start_y - min_y

    short_h = measure_jump(2)
    long_h = measure_jump(45)
    assert 8 <= short_h <= 20, f"unexpected short jump height {short_h}"
    assert 66 <= long_h <= 76, f"unexpected long jump height {long_h}"

    # 4) Classic 2-fireball cap test + 32-level header RLE integrity check
    fw = {"x_fp": 48 << 8, "y_fp": 160 << 8, "dir": 1, "fireballs": []}
    assert try_spawn_fireball(fw) is True
    assert try_spawn_fireball(fw) is True
    assert try_spawn_fireball(fw) is False
    for lvl_idx in range(1, 33):
        tm = build_level_map(lvl_idx)
        assert len(tm) == MAP_ROWS and len(tm[0]) >= 64
    print(f"PASS: pc_mp_client.py wire + UDP + jump physics (short={short_h}px, long={long_h}px) + 2-fireball cap + 32 stages RLE")


def main():
    args = sys.argv[1:]
    if "--self-test" in args:
        run_self_test()
        return
    host_ip = DEFAULT_HOST_IP
    port = WIFI_UDP_PORT
    for i, a in enumerate(args):
        if a == "--host" and i + 1 < len(args):
            host_ip = args[i + 1]
        elif a == "--port" and i + 1 < len(args):
            port = int(args[i + 1])
    run_client_gui(host_ip, port)


if __name__ == "__main__":
    main()
