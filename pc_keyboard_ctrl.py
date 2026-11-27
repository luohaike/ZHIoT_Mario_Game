#!/usr/bin/env python3
"""
PC Keyboard Controller + Live 2x Screen Mirror (640x480) for ZHIoT_Mario_Game.
Zero external dependencies (pure Python stdlib + tkinter).

Uses the already-connected ST-LINK (OpenOCD SWD non-halting background memory
access) to BOTH send zero-lag keyboard inputs and mirror the STM32 game screen
in real time at ~50-60 FPS.

Controls (focus the window to play):
  A / Left Arrow   : Move Left
  D / Right Arrow  : Move Right
  W / Space / Up   : Jump (tap = short hop, hold = high jump) / Start / Retry
  M                : Toggle Mute (SND/MUT)
  Esc / Q          : Quit
"""

import glob
import os
import re
import select
import socket
import struct
import subprocess
import sys
import termios
import time
import tty

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ELF_PATH = os.path.join(SCRIPT_DIR, "build", "ZHIoT_Mario_Game.elf")
SPRITES_H_PATH = os.path.join(SCRIPT_DIR, "Inc", "mario_sprites.h")

SCREEN_W = 320
SCREEN_H = 240
HUD_H = 16
TILE_SIZE = 16
MAP_ROWS = 14
MAP_COLS = 400
SCALE = 2

# Flash Sprite indices matching Src/mario_game.c
C_MARIO_STAND = 0
C_MARIO_RUN1 = 1
C_MARIO_RUN2 = 2
C_MARIO_JUMP = 3
C_GOOMBA1 = 4
C_GOOMBA2 = 5
C_GROUND = 6
C_BRICK = 7
C_QBLOCK = 8
C_USED = 9
C_PIPE_TL = 10
C_PIPE_TR = 11
C_PIPE_BL = 12
C_PIPE_BR = 13
C_COIN1 = 14
C_COIN2 = 15
C_FLAG_TOP = 16
C_FLAG_POLE = 17
C_STONE = 18
C_MUSHROOM = 19
C_FLOWER = 20
C_FIREBALL = 21
C_FIRE_PUFF = 22
C_PIRANHA1 = 23
C_PIRANHA2 = 24
C_KOOPA1 = 25
C_KOOPA2 = 26
C_SHELL = 27
C_PARATROOPA1 = 28
C_PARATROOPA2 = 29
C_BUZZY1 = 30
C_BUZZY2 = 31
C_SPINY1 = 32
C_SPINY2 = 33
C_LAKITU = 34
C_HAMMER_BRO1 = 35
C_HAMMER_BRO2 = 36
C_HAMMER = 37
C_BULLET = 38
C_CANNON_TOP = 39
C_CANNON_BOT = 40
C_CHEEP1 = 41
C_CHEEP2 = 42
C_BLOOBER1 = 43
C_BLOOBER2 = 44
C_BOWSER_TL = 45
C_BOWSER_TR = 46
C_BOWSER_BL = 47
C_BOWSER_BR = 48
C_BOWSER_FIRE = 49
C_TOAD = 50
C_PEACH = 51
C_STAR = 52
C_AXE = 53
C_CHAIN = 54
C_BRIDGE = 55
C_WATER_SURF = 56
C_WATER_FILL = 57
C_LAVA_SURF = 58
C_LAVA_FILL = 59
C_SPRING_UP = 60
C_SPRING_DN = 61
C_CLOUD_PLAT = 62
C_MUSH_L = 63
C_MUSH_M = 64
C_MUSH_R = 65
C_MUSH_STEM = 66
C_SIDE_PIPE_T = 67
C_SIDE_PIPE_B = 68
C_CASTLE_WALL = 69
C_CASTLE_BATT = 70
C_CASTLE_ARCH = 71
C_ROPE = 72
C_VINE = 73
C_PLAT = 74
C_GROUND_SUB = 75

SPRITE_ORDER = [
    "SPR_MARIO_STAND", "SPR_MARIO_RUN1", "SPR_MARIO_RUN2", "SPR_MARIO_JUMP",
    "SPR_GOOMBA_1", "SPR_GOOMBA_2", "SPR_GROUND", "SPR_BRICK",
    "SPR_QBLOCK", "SPR_USED", "SPR_PIPE_TL", "SPR_PIPE_TR",
    "SPR_PIPE_BL", "SPR_PIPE_BR", "SPR_COIN_1", "SPR_COIN_2",
    "SPR_FLAG_TOP", "SPR_FLAG_POLE", "SPR_BLOCK_STONE", "SPR_MUSHROOM",
    "SPR_FLOWER", "SPR_FIREBALL", "SPR_FIRE_PUFF",
    "SPR_PIRANHA1", "SPR_PIRANHA2", "SPR_KOOPA1", "SPR_KOOPA2",
    "SPR_SHELL", "SPR_PARATROOPA1", "SPR_PARATROOPA2", "SPR_BUZZY1",
    "SPR_BUZZY2", "SPR_SPINY1", "SPR_SPINY2", "SPR_LAKITU",
    "SPR_HAMMER_BRO1", "SPR_HAMMER_BRO2", "SPR_HAMMER", "SPR_BULLET",
    "SPR_CANNON_TOP", "SPR_CANNON_BOT", "SPR_CHEEP1", "SPR_CHEEP2",
    "SPR_BLOOBER1", "SPR_BLOOBER2", "SPR_BOWSER_TL", "SPR_BOWSER_TR",
    "SPR_BOWSER_BL", "SPR_BOWSER_BR", "SPR_BOWSER_FIRE", "SPR_TOAD",
    "SPR_PEACH", "SPR_STAR", "SPR_AXE", "SPR_CHAIN",
    "SPR_BRIDGE", "SPR_WATER_SURF", "SPR_WATER_FILL", "SPR_LAVA_SURF",
    "SPR_LAVA_FILL", "SPR_SPRING_UP", "SPR_SPRING_DN", "SPR_CLOUD_PLAT",
    "SPR_MUSH_L", "SPR_MUSH_M", "SPR_MUSH_R", "SPR_MUSH_STEM",
    "SPR_SIDE_PIPE_T", "SPR_SIDE_PIPE_B", "SPR_CASTLE_WALL", "SPR_CASTLE_BATT",
    "SPR_CASTLE_ARCH", "SPR_ROPE", "SPR_VINE", "SPR_PLAT", "SPR_GROUND_SUB",
]

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

EN_GOOMBA = 1
EN_KOOPA_G = 2
EN_KOOPA_R = 3
EN_PARATROOPA_J = 4
EN_PARATROOPA_F = 5
EN_PARATROOPA_H = 6
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
EN_SHELL = 19
EN_HAMMER = 20
EN_BOWSER_FIRE = 21

STATE_TITLE = 0
STATE_LOBBY = 1
STATE_PLAYING = 2
STATE_DEAD_WAIT = 3
STATE_LEVEL_CLEAR = 4
STATE_GAME_OVER = 5
STATE_WIN = 6
STATE_PRE_STAGE = 7

WIFI_MODE_SOLO = 0
WIFI_MODE_HOST = 1
WIFI_MODE_JOIN = 2

WIFI_STAGE_TEXT = {
    0: "OFF",
    1: "INIT ESP8285",
    2: "SET WIFI MODE",
    3: "AP / JOIN MARIO-MP",
    4: "SETUP UDP",
    5: "OPEN SOCKET 9000",
    6: "READY / WAIT ACK",
    7: "CONNECTED!",
    8: "WIFI ERR / NO ESP",
}

PLAYER_TAG_HEX = ["#FF5555", "#55FF55", "#FFD700", "#55FFFF"]


def rgb565_to_hex(c):
    r = ((c >> 11) & 0x1F) * 255 // 31
    g = ((c >> 5) & 0x3F) * 255 // 63
    b = (c & 0x1F) * 255 // 31
    return f"#{r:02x}{g:02x}{b:02x}"


def load_sprites_from_header(path):
    txt = open(path, "r", encoding="utf-8").read()
    cols = {}
    for m in re.finditer(r"#define\s+(COL_\w+)\s+0x([0-9A-Fa-f]+)U?", txt):
        cols[m.group(1)] = int(m.group(2), 16)

    pal_map = {
        ".": None,
        "R": rgb565_to_hex(cols["COL_RED"]),
        "r": rgb565_to_hex(cols["COL_DARKRED"]),
        "S": rgb565_to_hex(cols["COL_SKIN"]),
        "B": rgb565_to_hex(cols["COL_BLUE"]),
        "N": rgb565_to_hex(cols["COL_BROWN"]),
        "Y": rgb565_to_hex(cols["COL_GOLD"]),
        "y": rgb565_to_hex(cols["COL_GOLD_DK"]),
        "W": rgb565_to_hex(cols["COL_WHITE"]),
        "K": rgb565_to_hex(cols["COL_BLACK"]),
        "G": rgb565_to_hex(cols["COL_PIPE_MD"]),
        "g": rgb565_to_hex(cols["COL_PIPE_LT"]),
        "d": rgb565_to_hex(cols["COL_PIPE_DK"]),
        "C": rgb565_to_hex(cols["COL_BRICK"]),
        "c": rgb565_to_hex(cols["COL_BRICK_DK"]),
        "D": rgb565_to_hex(cols["COL_GROUND"]),
        "L": rgb565_to_hex(cols["COL_GRASS"]),
        "A": rgb565_to_hex(cols["COL_GRAY"]),
        "a": rgb565_to_hex(cols["COL_DARKGRAY"]),
        "U": rgb565_to_hex(cols["COL_WATER_LT"]),
        "u": rgb565_to_hex(cols["COL_WATER_DK"]),
        "O": rgb565_to_hex(cols["COL_LAVA_LT"]),
        "P": rgb565_to_hex(cols["COL_PINK"]),
    }

    sprites = {}
    for m in re.finditer(r"static const Sprite16\s+(SPR_\w+)\s*=\s*\{\{(.*?)\}\};", txt, re.S):
        name = m.group(1)
        rows = re.findall(r'"([^"]{16})"', m.group(2))
        if len(rows) == 16:
            sprites[name] = rows
    return cols, pal_map, sprites


def build_slot_pal_map(cols, base_pal_map, slot):
    """Return a color map adjusted to the given player slot (0..3), matching sprite_palette_slot."""
    pm = dict(base_pal_map)
    if slot == 1:
        pm["R"] = rgb565_to_hex(cols["COL_P2_LT"])
        pm["r"] = rgb565_to_hex(cols["COL_P2_DK"])
    elif slot == 2:
        pm["R"] = rgb565_to_hex(cols["COL_P3_LT"])
        pm["r"] = rgb565_to_hex(cols["COL_P3_DK"])
        pm["B"] = rgb565_to_hex(cols["COL_DARKRED"])
    elif slot == 3:
        pm["R"] = rgb565_to_hex(cols["COL_P4_LT"])
        pm["r"] = rgb565_to_hex(cols["COL_P4_DK"])
        pm["B"] = rgb565_to_hex(cols["COL_PIPE_DK"])
    elif slot == 4:
        pm["R"] = rgb565_to_hex(cols["COL_WHITE"])
        pm["r"] = rgb565_to_hex(cols["COL_GRAY"])
        pm["B"] = rgb565_to_hex(cols["COL_RED"])
    return pm


class UartBackend:
    def __init__(self, port):
        fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = termios.B115200
        attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        self.fd = fd
        self.can_mirror = False
        self.name = f"UART2 ({port} @ 115200)"

    def send_mask(self, mask):
        os.write(self.fd, bytes([0x80 | (mask & 0x1F)]))

    def read_logs(self):
        try:
            data = os.read(self.fd, 256)
            if data:
                return data.decode("utf-8", errors="ignore").replace("\n", "\r\n")
        except OSError:
            pass
        return ""

    def read_mirror(self):
        return None

    def read_tile_map(self, cols=218):
        return None

    def close(self):
        try:
            os.write(self.fd, bytes([0x80]))
            os.close(self.fd)
        except OSError:
            pass


class SwdBackend:
    def __init__(self, elf_path):
        nm_out = subprocess.check_output(["arm-none-eabi-nm", "-n", elf_path], text=True)
        syms = {}
        for line in nm_out.splitlines():
            parts = line.split()
            if len(parts) >= 3 and parts[2] in ("g_swd_key_cmd", "g_mirror_state", "tile_map"):
                syms[parts[2]] = "0x" + parts[0]
        if "g_swd_key_cmd" not in syms:
            raise RuntimeError("Symbol g_swd_key_cmd not found in ELF")
        self.kaddr = syms["g_swd_key_cmd"]
        self.maddr = syms.get("g_mirror_state")
        self.taddr = syms.get("tile_map")
        self.can_mirror = bool(self.maddr and self.taddr)

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as tmp_s:
            tmp_s.bind(("127.0.0.1", 0))
            self.tcl_port = tmp_s.getsockname()[1]

        self.log_path = os.path.join("/tmp", f"mario_openocd_{os.getpid()}.log")
        self.log_fp = open(self.log_path, "w+", encoding="utf-8", errors="ignore")
        self.proc = subprocess.Popen(
            [
                "openocd",
                "-f", "interface/stlink.cfg",
                "-f", "target/stm32l4x.cfg",
                "-c", f"bindto 127.0.0.1; tcl_port {self.tcl_port}; gdb_port disabled; telnet_port disabled; init; poll off",
            ],
            stdout=subprocess.DEVNULL,
            stderr=self.log_fp,
        )
        sock = None
        deadline = time.monotonic() + 5.0
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                self.log_fp.seek(0)
                err = self.log_fp.read()
                self.log_fp.close()
                raise RuntimeError(f"OpenOCD failed to start:\n{err}")
            try:
                sock = socket.create_connection(("127.0.0.1", self.tcl_port), timeout=1.0)
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            self.proc.terminate()
            self.log_fp.close()
            raise RuntimeError("Timed out connecting to OpenOCD TCL server")
        self.sock = sock
        self.fd = -1
        self.name = f"ST-LINK SWD ({self.kaddr})"

    def _tcl(self, command):
        try:
            self.sock.sendall((command + "\x1a").encode("ascii"))
            buf = b""
            while b"\x1a" not in buf:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                buf += chunk
            return buf.split(b"\x1a", 1)[0].decode("ascii", errors="ignore").strip()
        except OSError:
            return ""

    def send_mask(self, mask):
        self._tcl(f"write_memory {self.kaddr} 8 {{0x{(0x80 | (mask & 0x7F)):02x}}}")

    def read_mirror(self):
        if not self.maddr:
            return None
        resp = self._tcl(f"read_memory {self.maddr} 32 93")
        tokens = resp.split()
        if len(tokens) != 93:
            return None
        raw = struct.pack("<93I", *(int(x, 16) for x in tokens))
        hdr = struct.unpack_from("<IIBBBBBBHHHHHIhhhBBBBBB", raw, 0)
        (magic, frame, state, level, area_id, theme_flags, lives, sound,
         coins, map_rev, cur_cols, time_left, title_world, score,
         cam_x, px, py, p_spr, p_flags, fps, ev_row, ev_col, ev_val) = hdr
        if magic != 0x4D415232:
            return None
        enemies = []
        off = 40
        for _ in range(20):
            ex, ey, ekind, eflags = struct.unpack_from("<hhBB", raw, off)
            enemies.append((ex, ey, ekind, eflags))
            off += 6
        platforms = []
        for _ in range(8):
            px_i, py_i, pkind, pactive = struct.unpack_from("<hhBB", raw, off)
            platforms.append((px_i, py_i, pkind, pactive))
            off += 6
        firebars = []
        for _ in range(8):
            fx_i, fy_i, fangle, flen = struct.unpack_from("<hhBB", raw, off)
            firebars.append((fx_i, fy_i, fangle, flen))
            off += 6
        popups = []
        for _ in range(8):
            qx, qy, qkind, qtimer = struct.unpack_from("<hhBB", raw, off)
            popups.append((qx, qy, qkind, qtimer))
            off += 6
        items = []
        for _ in range(4):
            ix, iy, ikind = struct.unpack_from("<hhB", raw, off)
            items.append((ix, iy, ikind))
            off += 5
        fireballs = []
        for _ in range(4):
            fx, fy, factive = struct.unpack_from("<hhB", raw, off)
            fireballs.append((fx, fy, factive))
            off += 5
        net_role, net_stage, my_slot, net_count = struct.unpack_from("<BBBB", raw, off)
        off += 4
        peers = []
        for _ in range(3):
            rx, ry, rspr, rflags, ractive, rslot = struct.unpack_from("<hhBBBB", raw, off)
            peers.append((rx, ry, rspr, rflags, ractive, rslot))
            off += 8
        return {
            "frame": frame,
            "state": state,
            "level": level,
            "area_id": area_id,
            "theme_flags": theme_flags,
            "lives": lives,
            "sound": sound,
            "coins": coins,
            "map_rev": map_rev,
            "cur_cols": cur_cols,
            "time_left": time_left,
            "title_world": title_world,
            "score": score,
            "cam_x": cam_x,
            "px": px,
            "py": py,
            "p_spr": p_spr,
            "p_flags": p_flags,
            "fps": fps,
            "ev_row": ev_row,
            "ev_col": ev_col,
            "ev_val": ev_val,
            "enemies": enemies,
            "platforms": platforms,
            "firebars": firebars,
            "popups": popups,
            "items": items,
            "fireballs": fireballs,
            "net_role": net_role,
            "net_stage": net_stage,
            "my_slot": my_slot,
            "net_count": net_count,
            "peers": peers,
        }

    def read_tile_map(self, cols=218):
        if not self.taddr:
            return None
        cols = max(20, min(MAP_MAX_COLS if "MAP_MAX_COLS" in globals() else MAP_COLS, int(cols)))
        grid = []
        # tile_map has stride MAP_COLS (400) per row in C memory: read row-by-row compactly
        words_per_row = (cols + 3) // 4
        for r in range(MAP_ROWS):
            row_addr = hex(int(self.taddr, 16) + r * MAP_COLS)
            resp = self._tcl(f"read_memory {row_addr} 32 {words_per_row}")
            tokens = resp.split()
            if len(tokens) != words_per_row:
                return None
            raw = struct.pack(f"<{words_per_row}I", *(int(x, 16) for x in tokens))
            grid.append(bytearray(raw[:cols]))
        return grid

    def read_logs(self):
        return ""

    def close(self):
        try:
            self.send_mask(0)
        except OSError:
            pass
        try:
            self.sock.sendall(b"shutdown\x1a")
            self.sock.close()
        except OSError:
            pass
        try:
            self.proc.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            self.proc.kill()
        try:
            self.log_fp.close()
            if os.path.exists(self.log_path):
                os.remove(self.log_path)
        except OSError:
            pass


def open_backend(args):
    serial_args = [a for a in args if a != "--cli"]
    if serial_args:
        return UartBackend(serial_args[0])
    # Prefer SWD when mirror ELF exists and no explicit UART port was forced,
    # or if /dev/ttyUSB* exists without SWD.
    if os.path.exists(ELF_PATH):
        try:
            return SwdBackend(ELF_PATH)
        except Exception:
            pass
    usb_ports = sorted(glob.glob("/dev/ttyUSB*"))
    if usb_ports:
        return UartBackend(usb_ports[0])
    return SwdBackend(ELF_PATH)


def make_tk_sprite(tk_mod, rows, pal_map, flip_x=False, dst_h=TILE_SIZE):
    w = TILE_SIZE * SCALE
    h = dst_h * SCALE
    img = tk_mod.PhotoImage(width=w, height=h)
    for r in range(dst_h):
        src_r = (r * TILE_SIZE) // dst_h
        row_str = rows[src_r]
        for c in range(TILE_SIZE):
            src_c = (TILE_SIZE - 1 - c) if flip_x else c
            color = pal_map.get(row_str[src_c])
            x0 = c * SCALE
            y0 = r * SCALE
            if color is None:
                for dy in range(SCALE):
                    for dx in range(SCALE):
                        img.transparency_set(x0 + dx, y0 + dy, True)
            else:
                img.put(color, to=(x0, y0, x0 + SCALE, y0 + SCALE))
    return img


def run_tk_gui(backend):
    import tkinter as tk

    cols, pal_map, spr_defs = load_sprites_from_header(SPRITES_H_PATH)

    root = tk.Tk()
    root.title("Super Mario STM32 - PC Mirror + Controller")
    cw = SCREEN_W * SCALE
    ch = SCREEN_H * SCALE
    root.geometry(f"{cw}x{ch + 34}")
    root.configure(bg="#11151C")
    root.resizable(False, False)

    canvas = tk.Canvas(root, width=cw, height=ch, bg="#000000", highlightthickness=0)
    canvas.pack()

    bar_var = tk.StringVar(value=f"Connected: {backend.name}")
    tk.Label(
        root,
        textvariable=bar_var,
        fg="#7EE787",
        bg="#11151C",
        font=("Monospace", -11),
        anchor="w",
        padx=8,
    ).pack(fill="x", pady=4)

    # Pre-build 2x PhotoImage cache per theme (0=Water, 1=Overworld, 2=Underground, 3=Castle)
    def build_theme_pal(th):
        pm = dict(pal_map)
        if th == 2:
            pm["C"] = rgb565_to_hex(cols["COL_UG_BRK"])
            pm["c"] = rgb565_to_hex(cols["COL_UG_DK"])
            pm["D"] = rgb565_to_hex(cols["COL_UG_BRK"])
            pm["G"] = rgb565_to_hex(cols["COL_UG_DK"])
            pm["L"] = "#55ffff"
            pm["A"] = rgb565_to_hex(cols["COL_UG_BRK"])
            pm["a"] = rgb565_to_hex(cols["COL_UG_DK"])
        elif th == 3:
            pm["C"] = rgb565_to_hex(cols["COL_CAST_WL"])
            pm["c"] = rgb565_to_hex(cols["COL_CAST_DK"])
            pm["D"] = rgb565_to_hex(cols["COL_CAST_WL"])
            pm["G"] = rgb565_to_hex(cols["COL_CAST_DK"])
            pm["L"] = "#adadad"
        elif th == 0:
            pm["D"] = "#28b838"
            pm["G"] = "#28b838"
            pm["L"] = "#4cf868"
            pm["C"] = "#048820"
            pm["c"] = "#024410"
        return pm

    tk_theme_sprites = {}
    for th in (0, 1, 2, 3):
        tpm = build_theme_pal(th)
        tk_theme_sprites[th] = [make_tk_sprite(tk, spr_defs[name], tpm, flip_x=False) for name in SPRITE_ORDER]
    tk_sprites = tk_theme_sprites[1]
    tk_fireball_flip = make_tk_sprite(tk, spr_defs["SPR_FIREBALL"], pal_map, flip_x=True)

    # Enemy red-swap & flip variants + 1-UP green mushroom
    red_pm = dict(pal_map)
    red_pm["G"] = rgb565_to_hex(cols["COL_RED"])
    red_pm["g"] = "#ff6800"
    red_pm["d"] = rgb565_to_hex(cols["COL_DARKRED"])
    up1_pm = dict(pal_map)
    up1_pm["R"] = rgb565_to_hex(cols["COL_PIPE_MD"])
    up1_pm["r"] = rgb565_to_hex(cols["COL_PIPE_DK"])
    tk_1up = make_tk_sprite(tk, spr_defs["SPR_MUSHROOM"], up1_pm, flip_x=False)
    tk_sprites_flip = {}
    for sid in (C_KOOPA1, C_KOOPA2, C_PARATROOPA1, C_PARATROOPA2, C_BUZZY1, C_BUZZY2, C_SPINY1, C_SPINY2, C_HAMMER_BRO1, C_HAMMER_BRO2, C_BULLET, C_CHEEP1, C_CHEEP2, C_LAKITU):
        tk_sprites_flip[sid] = make_tk_sprite(tk, spr_defs[SPRITE_ORDER[sid]], pal_map, flip_x=True)
    tk_red_norm = {sid: make_tk_sprite(tk, spr_defs[SPRITE_ORDER[sid]], red_pm, flip_x=False) for sid in (C_KOOPA1, C_KOOPA2, C_SHELL, C_PARATROOPA1, C_PARATROOPA2, C_CHEEP1, C_CHEEP2)}
    tk_red_flip = {sid: make_tk_sprite(tk, spr_defs[SPRITE_ORDER[sid]], red_pm, flip_x=True) for sid in (C_KOOPA1, C_KOOPA2, C_PARATROOPA1, C_PARATROOPA2, C_CHEEP1, C_CHEEP2)}
    tk_player_norm = []
    tk_player_flip = []
    tk_player_tall_norm = []
    tk_player_tall_flip = []
    for slot in range(5):
        spm = build_slot_pal_map(cols, pal_map, slot)
        tk_player_norm.append([
            make_tk_sprite(tk, spr_defs[SPRITE_ORDER[i]], spm, flip_x=False)
            for i in range(4)
        ])
        tk_player_flip.append([
            make_tk_sprite(tk, spr_defs[SPRITE_ORDER[i]], spm, flip_x=True)
            for i in range(4)
        ])
        tk_player_tall_norm.append([
            make_tk_sprite(tk, spr_defs[SPRITE_ORDER[i]], spm, flip_x=False, dst_h=24)
            for i in range(4)
        ])
        tk_player_tall_flip.append([
            make_tk_sprite(tk, spr_defs[SPRITE_ORDER[i]], spm, flip_x=True, dst_h=24)
            for i in range(4)
        ])

    sky_day = rgb565_to_hex(cols["COL_SKY_DAY"])
    sky_ngt = rgb565_to_hex(cols["COL_SKY_NGT"])
    col_cloud = rgb565_to_hex(cols["COL_CLOUD"])
    col_cloud_sh = rgb565_to_hex(cols["COL_CLOUD_SH"])
    col_hill_lt = rgb565_to_hex(cols["COL_HILL_LT"])
    col_hill_dk = rgb565_to_hex(cols["COL_HILL_DK"])
    col_gold = rgb565_to_hex(cols["COL_GOLD"])
    col_red = rgb565_to_hex(cols["COL_RED"])

    held = set()
    pending_release = {}
    state = {"last_mask": -1, "last_hb": 0.0, "map_rev": -1, "tile_map": None, "pc_fps": 0, "fps_t0": time.monotonic(), "fps_n": 0}

    def map_key(event):
        ks = (event.keysym or "").lower()
        if ks in ("a", "left", "1"):
            return "L"
        if ks in ("d", "right", "2"):
            return "R"
        if ks in ("w", "up", "space", "return", "3"):
            return "J"
        if ks in ("m", "4"):
            return "M"
        if ks in ("f", "j", "control_l"):
            return "F"
        if ks in ("s", "down"):
            return "D"
        if ks in ("shift_l", "shift_r"):
            return "B"
        return None

    def compute_mask():
        m = 0
        if "L" in held:
            m |= 0x01
        if "R" in held:
            m |= 0x02
        if "J" in held:
            m |= 0x04
        if "M" in held:
            m |= 0x08
        if "F" in held:
            m |= 0x10
        if "D" in held:
            m |= 0x20
        if "B" in held:
            m |= 0x40
        return m

    def push_keys(force=False):
        now = time.monotonic()
        m = compute_mask()
        if force or m != state["last_mask"] or (m != 0 and now - state["last_hb"] >= 0.06):
            backend.send_mask(m)
            state["last_mask"] = m
            state["last_hb"] = now

    def on_key_press(event):
        ks = (event.keysym or "").lower()
        if ks in ("escape", "q"):
            root.quit()
            return
        k = map_key(event)
        if not k:
            return
        if k in pending_release:
            root.after_cancel(pending_release.pop(k))
        if k not in held:
            held.add(k)
            push_keys(force=True)

    def finish_release(k):
        pending_release.pop(k, None)
        if k in held:
            held.discard(k)
            push_keys(force=True)

    def on_key_release(event):
        k = map_key(event)
        if not k:
            return
        if k in pending_release:
            root.after_cancel(pending_release.pop(k))
        pending_release[k] = root.after(25, lambda key=k: finish_release(key))

    def on_focus_out(_event):
        for aid in list(pending_release.values()):
            root.after_cancel(aid)
        pending_release.clear()
        held.clear()
        push_keys(force=True)

    TILE_MAP_SPR = {
        T_GROUND: C_GROUND, T_BRICK: C_BRICK, T_QBLOCK: C_QBLOCK,
        T_QBLOCK_MUSH: C_QBLOCK, T_QBLOCK_FLOWER: C_QBLOCK,
        T_BRICK_POWER: C_BRICK, T_BRICK_VINE: C_BRICK, T_BRICK_STAR: C_BRICK,
        T_BRICK_10COIN: C_BRICK, T_BRICK_1UP: C_BRICK,
        T_USED: C_USED, T_PIPE_TL: C_PIPE_TL, T_PIPE_WARP_TL: C_PIPE_TL,
        T_PIPE_TR: C_PIPE_TR, T_PIPE_WARP_TR: C_PIPE_TR,
        T_PIPE_BL: C_PIPE_BL, T_PIPE_BR: C_PIPE_BR,
        T_FLAG_TOP: C_FLAG_TOP, T_FLAG_POLE: C_FLAG_POLE, T_STONE: C_STONE,
        T_BRIDGE: C_BRIDGE, T_WATER_SURF: C_WATER_SURF, T_WATER_FILL: C_WATER_FILL,
        T_LAVA_SURF: C_LAVA_SURF, T_LAVA_FILL: C_LAVA_FILL,
        T_CANNON_TOP: C_CANNON_TOP, T_CANNON_MID: C_CANNON_BOT, T_CANNON_BOT: C_CANNON_BOT,
        T_AXE: C_AXE, T_CHAIN: C_CHAIN, T_SPRING_TOP: C_SPRING_UP, T_SPRING_BOT: C_SPRING_DN,
        T_CLOUD_PLAT: C_CLOUD_PLAT, T_MUSH_PLAT_L: C_MUSH_L, T_MUSH_PLAT_M: C_MUSH_M,
        T_MUSH_PLAT_R: C_MUSH_R, T_MUSH_STEM: C_MUSH_STEM,
        T_SIDE_PIPE_T: C_SIDE_PIPE_T, T_SIDE_PIPE_B: C_SIDE_PIPE_B,
        T_CASTLE_WALL: C_CASTLE_WALL, T_CASTLE_ARCH: C_CASTLE_ARCH,
        T_CASTLE_BATT: C_CASTLE_BATT, T_ROPE: C_ROPE,
    }

    SIN64 = [
        0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126,
        127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
        0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126,
        -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12,
    ]

    def tile_to_cache_id(tile, frame):
        if tile == T_COIN:
            return C_COIN2 if ((frame // 10) & 1) else C_COIN1
        return TILE_MAP_SPR.get(tile, C_STONE)

    def render_mirror(m):
        cur_cols = m["cur_cols"]
        if m["map_rev"] != state["map_rev"] or state["tile_map"] is None or len(state["tile_map"][0]) != cur_cols:
            ev_r_raw = m["ev_row"]
            ev_r = ev_r_raw & 0x0F
            ev_c = m["ev_col"] | (((ev_r_raw >> 4) & 1) << 8)
            if (
                state["tile_map"] is not None
                and len(state["tile_map"][0]) == cur_cols
                and m["map_rev"] == ((state["map_rev"] + 1) & 0xFFFF)
                and ev_r < MAP_ROWS
                and ev_c < cur_cols
            ):
                state["tile_map"][ev_r][ev_c] = m["ev_val"]
                state["map_rev"] = m["map_rev"]
            else:
                tm = backend.read_tile_map(cur_cols)
                if tm is not None:
                    state["tile_map"] = tm
                    state["map_rev"] = m["map_rev"]

        canvas.delete("all")
        st = m["state"]
        tf = m["theme_flags"]
        th = tf & 0x03
        night = bool((tf >> 2) & 1)
        snow = bool((tf >> 3) & 1)
        bg_sce = (tf >> 4) & 0x03
        cam_x = m["cam_x"]
        frame = m["frame"]
        tspr = tk_theme_sprites.get(th, tk_theme_sprites[1])

        if st in (STATE_PRE_STAGE, STATE_GAME_OVER):
            canvas.create_rectangle(0, HUD_H * SCALE, cw, ch, fill="#000000", outline="")
        else:
            if th == 0:
                bg = rgb565_to_hex(cols["COL_UW_BG"])
            elif th in (2, 3):
                bg = "#000000"
            elif night:
                bg = "#080818"
            elif snow:
                bg = "#408ce8"
            else:
                bg = sky_day
            canvas.create_rectangle(0, HUD_H * SCALE, cw, ch, fill=bg, outline="")

            if th == 1 and night:
                star_x = (22, 68, 114, 149, 193, 237, 281, 305, 44, 259)
                for idx, sx0 in enumerate(star_x):
                    sx = ((sx0 - (cam_x // 8)) % SCREEN_W) * SCALE
                    sy = (HUD_H + ((idx % 6) + 1) * 16 + (idx * 3 % 11)) * SCALE
                    canvas.create_rectangle(sx, sy, sx + SCALE, sy + SCALE, fill="#FFD700" if (idx & 1) else "#C8D0E0", outline="")

            if th == 1 and bg_sce > 0:
                cl_fill = "#4A4E5A" if night else "#FFFFFF"
                cl_sh = "#29303C" if night else "#B6E0FF"
                cl_rim = "#181C24" if night else "#3A78C8"
                hl_lt = "#006818" if night else ("#509840" if snow else col_hill_lt)
                hl_dk = "#003808" if night else ("#285820" if snow else col_hill_dk)
                hl_rim = "#001800" if night else "#004800"

                # Puffy rounded clouds (rows 2..3): outlines first, seamless interior fills on top
                c_scroll = (cam_x // 3) % 144
                for k in range(-1, 4):
                    cx = k * 144 - c_scroll + 36
                    cy = HUD_H + 2 * 16 + 7
                    canvas.create_oval((cx - 24) * SCALE, (cy + 5) * SCALE, (cx + 24) * SCALE, (cy + 19) * SCALE, fill=cl_rim, outline=cl_rim, width=SCALE)
                    canvas.create_oval((cx - 14) * SCALE, cy * SCALE, (cx + 14) * SCALE, (cy + 15) * SCALE, fill=cl_rim, outline=cl_rim, width=SCALE)
                    canvas.create_oval((cx - 23) * SCALE, (cy + 6) * SCALE, (cx + 23) * SCALE, (cy + 18) * SCALE, fill=cl_sh, outline="")
                    canvas.create_oval((cx - 23) * SCALE, (cy + 5) * SCALE, (cx + 23) * SCALE, (cy + 16) * SCALE, fill=cl_fill, outline="")
                    canvas.create_oval((cx - 13) * SCALE, (cy + 1) * SCALE, (cx + 13) * SCALE, (cy + 15) * SCALE, fill=cl_fill, outline="")

                # Grounded rounded hills sitting on row 12 (bot_y = HUD_H + 12 * 16) + bushes
                h_scroll = (cam_x // 2) % 192
                bot_y = (HUD_H + 12 * 16) * SCALE
                top_y = (HUD_H + 10 * 16 + 4) * SCALE
                for k in range(-1, 3):
                    hx = k * 192 - h_scroll + 36
                    canvas.create_polygon(
                        (hx - 37) * SCALE, bot_y,
                        (hx - 10) * SCALE, top_y + 4 * SCALE,
                        (hx - 4) * SCALE, top_y,
                        (hx + 4) * SCALE, top_y,
                        (hx + 10) * SCALE, top_y + 4 * SCALE,
                        (hx + 37) * SCALE, bot_y,
                        fill=hl_lt, outline=hl_rim, width=SCALE,
                    )
                    canvas.create_oval((hx - 7) * SCALE, (HUD_H + 10 * 16 + 14) * SCALE, (hx - 2) * SCALE, (HUD_H + 10 * 16 + 19) * SCALE, fill=hl_dk, outline="")
                    canvas.create_oval((hx + 5) * SCALE, (HUD_H + 11 * 16 + 7) * SCALE, (hx + 10) * SCALE, (HUD_H + 11 * 16 + 12) * SCALE, fill=hl_dk, outline="")
                    if bg_sce != 3:
                        bx = hx + 88
                        canvas.create_oval((bx - 16) * SCALE, (HUD_H + 11 * 16 + 5) * SCALE, (bx + 16) * SCALE, bot_y, fill=hl_lt, outline=hl_rim, width=SCALE)

        # Piranha plants behind pipes
        if st not in (STATE_PRE_STAGE, STATE_GAME_OVER):
            for ex, ey, ekind, eflags in m["enemies"]:
                if (eflags & 1) and ekind == EN_PIRANHA and (eflags & 8):
                    pspr = C_PIRANHA2 if ((frame // 8) & 1) else C_PIRANHA1
                    canvas.create_image((ex - cam_x) * SCALE, (HUD_H + ey) * SCALE, anchor="nw", image=tk_sprites[pspr])

        # Tiles
        tmap = state["tile_map"]
        if tmap is not None and st not in (STATE_PRE_STAGE, STATE_GAME_OVER):
            col_start = max(0, cam_x // TILE_SIZE)
            col_end = min(len(tmap[0]) - 1, (cam_x + SCREEN_W) // TILE_SIZE + 1)
            for r in range(MAP_ROWS):
                wy = (HUD_H + r * TILE_SIZE) * SCALE
                row_bytes = tmap[r]
                for c in range(col_start, col_end + 1):
                    t = row_bytes[c]
                    if t not in (T_EMPTY, T_QBLOCK_HIDDEN_COIN, T_QBLOCK_HIDDEN_1UP):
                        sx = (c * TILE_SIZE - cam_x) * SCALE
                        cid = C_GROUND_SUB if (t == T_GROUND and r > 0 and tmap[r - 1][c] == T_GROUND) else tile_to_cache_id(t, frame)
                        canvas.create_image(sx, wy, anchor="nw", image=tspr[cid])

        # Platforms & Firebars
        if st not in (STATE_PRE_STAGE, STATE_GAME_OVER):
            for px_i, py_i, _pk, pactive in m["platforms"]:
                if pactive:
                    sx = (px_i - cam_x) * SCALE
                    sy = (HUD_H + py_i) * SCALE
                    for k in range(3):
                        canvas.create_image(sx + k * 16 * SCALE, sy, anchor="nw", image=tk_sprites[C_PLAT])
            for fx_i, fy_i, fangle, flen in m["firebars"]:
                if flen > 0:
                    idx = (fangle >> 2) & 63
                    cosv = SIN64[(idx + 16) & 63]
                    sinv = SIN64[idx]
                    fimg = tk_fireball_flip if ((frame // 3) & 1) else tk_sprites[C_FIREBALL]
                    for b in range(flen):
                        bx = (fx_i + ((cosv * b * 6) >> 7) - 8 - cam_x) * SCALE
                        by = (HUD_H + fy_i + ((sinv * b * 6) >> 7) - 8) * SCALE
                        canvas.create_image(bx, by, anchor="nw", image=fimg)

        # Popups & Particles
        for qx, qy, qkind, qtimer in m["popups"]:
            if qtimer > 0:
                sx = (qx - cam_x) * SCALE
                sy = (HUD_H + qy) * SCALE
                if qkind == 1:
                    canvas.create_image(sx, sy, anchor="nw", image=tk_sprites[C_COIN2 if (qtimer & 2) else C_COIN1])
                elif qkind in (2, 3):
                    canvas.create_image(sx, sy, anchor="nw", image=tk_sprites[C_FIRE_PUFF])
                else:
                    pt = "1UP" if qkind == 7 else ("1000" if qkind == 6 else "200")
                    canvas.create_text(sx + 8, sy + 8, text=pt, fill="#FFFFFF", font=("Monospace", 8, "bold"))

        # Power-up items
        for ix, iy, ikind in m["items"]:
            if ikind > 0:
                sx = (ix - cam_x) * SCALE
                sy = (HUD_H + iy) * SCALE
                if -32 < sx < cw + 32:
                    if ikind == 5:
                        for vy in range(iy, -16, -16):
                            canvas.create_image(sx, (HUD_H + vy) * SCALE, anchor="nw", image=tk_sprites[C_VINE])
                    elif ikind == 4:
                        canvas.create_image(sx, sy, anchor="nw", image=tk_1up)
                    elif ikind == 3:
                        canvas.create_image(sx, sy, anchor="nw", image=tk_sprites[C_STAR])
                    else:
                        canvas.create_image(sx, sy, anchor="nw", image=tk_sprites[C_FLOWER if ikind == 2 else C_MUSHROOM])

        # Enemies
        anim2 = (frame // 10) & 1
        for ex, ey, ekind, eflags in m["enemies"]:
            if not (eflags & 1) or ekind == EN_PIRANHA:
                continue
            sx = (ex - cam_x) * SCALE
            sy = (HUD_H + ey) * SCALE
            if sx < -64 or sx > cw + 64:
                continue
            fx = bool(eflags & 2)
            if ekind == EN_BOWSER:
                canvas.create_image(sx, sy, anchor="nw", image=tk_sprites[C_BOWSER_TL])
                canvas.create_image(sx + 16 * SCALE, sy, anchor="nw", image=tk_sprites[C_BOWSER_TR])
                canvas.create_image(sx, sy + 16 * SCALE, anchor="nw", image=tk_sprites[C_BOWSER_BL])
                canvas.create_image(sx + 16 * SCALE, sy + 16 * SCALE, anchor="nw", image=tk_sprites[C_BOWSER_BR])
                continue
            if ekind == EN_GOOMBA:
                eimg = tk_sprites[C_GOOMBA2 if anim2 else C_GOOMBA1]
            elif ekind == EN_KOOPA_R:
                cid = C_KOOPA2 if anim2 else C_KOOPA1
                eimg = tk_red_flip[cid] if fx else tk_red_norm[cid]
            elif ekind == EN_KOOPA_G:
                cid = C_KOOPA2 if anim2 else C_KOOPA1
                eimg = tk_sprites_flip[cid] if fx else tk_sprites[cid]
            elif ekind == EN_SHELL:
                eimg = tk_sprites[C_SHELL]
            elif ekind in (EN_PARATROOPA_J, EN_PARATROOPA_F, EN_PARATROOPA_H):
                cid = C_PARATROOPA2 if anim2 else C_PARATROOPA1
                if ekind == EN_PARATROOPA_F:
                    eimg = tk_red_flip[cid] if fx else tk_red_norm[cid]
                else:
                    eimg = tk_sprites_flip[cid] if fx else tk_sprites[cid]
            elif ekind == EN_BUZZY:
                cid = C_BUZZY2 if anim2 else C_BUZZY1
                eimg = tk_sprites_flip[cid] if fx else tk_sprites[cid]
            elif ekind == EN_SPINY:
                cid = C_SPINY2 if anim2 else C_SPINY1
                eimg = tk_sprites_flip[cid] if fx else tk_sprites[cid]
            elif ekind == EN_LAKITU:
                eimg = tk_sprites_flip[C_LAKITU] if fx else tk_sprites[C_LAKITU]
            elif ekind == EN_HAMMER_BRO:
                cid = C_HAMMER_BRO2 if anim2 else C_HAMMER_BRO1
                eimg = tk_sprites_flip[cid] if fx else tk_sprites[cid]
            elif ekind == EN_HAMMER:
                eimg = tk_sprites[C_HAMMER]
            elif ekind == EN_BULLET:
                eimg = tk_sprites_flip[C_BULLET] if fx else tk_sprites[C_BULLET]
            elif ekind in (EN_CHEEP_GREY, EN_CHEEP_RED):
                cid = C_CHEEP2 if anim2 else C_CHEEP1
                if ekind == EN_CHEEP_RED:
                    eimg = tk_red_flip[cid] if fx else tk_red_norm[cid]
                else:
                    eimg = tk_sprites_flip[cid] if fx else tk_sprites[cid]
            elif ekind == EN_BLOOBER:
                eimg = tk_sprites[C_BLOOBER2 if anim2 else C_BLOOBER1]
            elif ekind == EN_PODOBOO:
                eimg = tk_sprites[C_FIREBALL]
            elif ekind == EN_BOWSER_FIRE:
                eimg = tk_sprites[C_BOWSER_FIRE]
            elif ekind == EN_TOAD_PEACH:
                eimg = tk_sprites[C_PEACH if m["level"] == 32 else C_TOAD]
            else:
                eimg = tk_sprites[C_GOOMBA1]
            canvas.create_image(sx, sy, anchor="nw", image=eimg)

        # Fireballs & puffs
        for fx, fy, factive in m["fireballs"]:
            if factive:
                sx = (fx - cam_x) * SCALE
                sy = (HUD_H + fy) * SCALE
                if -32 < sx < cw + 32:
                    fimg = tk_sprites[C_FIRE_PUFF] if factive == 2 else (tk_fireball_flip if ((frame // 3) & 1) else tk_sprites[C_FIREBALL])
                    canvas.create_image(sx, sy, anchor="nw", image=fimg)

        role = m["net_role"]
        my_slot = m["my_slot"] if m["my_slot"] < 4 else 0

        # Remote teammates
        if role != WIFI_MODE_SOLO and st not in (STATE_TITLE, STATE_LOBBY, STATE_PRE_STAGE):
            for rx, ry, rspr, rflags, ractive, rslot in m["peers"]:
                if ractive and (rflags & 0x02) and rslot < 4:
                    rsid = rspr if rspr < 4 else C_MARIO_STAND
                    r_pwr = (rflags >> 3) & 0x03
                    pal_slot = 4 if r_pwr >= 2 else rslot
                    if r_pwr >= 1:
                        rimg = tk_player_tall_flip[pal_slot][rsid] if (rflags & 0x01) else tk_player_tall_norm[pal_slot][rsid]
                        y_off = -8
                    else:
                        rimg = tk_player_flip[pal_slot][rsid] if (rflags & 0x01) else tk_player_norm[pal_slot][rsid]
                        y_off = 0
                    sx = (rx - cam_x) * SCALE
                    sy = (HUD_H + ry + y_off) * SCALE
                    canvas.create_image(sx, sy, anchor="nw", image=rimg)
                    canvas.create_text(sx + 16, sy - 6, text=f"{rslot + 1}P", fill=PLAYER_TAG_HEX[rslot], font=("Monospace", 8, "bold"))

        # Local player
        p_flags = m["p_flags"]
        p_pwr = (p_flags >> 3) & 0x03
        p_star = bool(p_flags & 0x20)
        p_crouch = bool(p_flags & 0x40)
        if st not in (STATE_LOBBY, STATE_PRE_STAGE, STATE_GAME_OVER) and (p_flags & 0x02):
            spr_id = m["p_spr"] if m["p_spr"] < 4 else C_MARIO_STAND
            c_slot = my_slot if role != WIFI_MODE_SOLO else 0
            pal_slot = ((frame // 2) & 3) if p_star else (4 if p_pwr >= 2 else c_slot)
            if p_pwr >= 1 and not p_crouch:
                img = tk_player_tall_flip[pal_slot][spr_id] if (p_flags & 0x01) else tk_player_tall_norm[pal_slot][spr_id]
                y_off = -8
            else:
                img = tk_player_flip[pal_slot][spr_id] if (p_flags & 0x01) else tk_player_norm[pal_slot][spr_id]
                y_off = 0
            sx = (m["px"] - cam_x) * SCALE
            sy = (HUD_H + m["py"] + y_off) * SCALE
            canvas.create_image(sx, sy, anchor="nw", image=img)
            if role != WIFI_MODE_SOLO:
                canvas.create_text(sx + 16, sy - 6, text=f"{c_slot + 1}P", fill=PLAYER_TAG_HEX[c_slot], font=("Monospace", 8, "bold"))

        # Classic 4-column HUD bar (pixel-sized font -13 avoids DPI scaling overlap)
        w_num = (m["level"] - 1) // 4 + 1
        s_num = (m["level"] - 1) % 4 + 1
        canvas.create_rectangle(0, 0, cw, HUD_H * SCALE, fill="#000000", outline="")
        hud_f = ("Monospace", -11, "bold")
        canvas.create_text(10, 9, anchor="nw", text=f"MARIO {m['score']:06d}", fill="#FFFFFF", font=hud_f)
        canvas.create_text(132, 9, anchor="nw", text=f"COIN *{m['coins']:02d}", fill="#FFD700", font=hud_f)
        canvas.create_text(224, 9, anchor="nw", text=f"WORLD {w_num}-{s_num}", fill="#FFFFFF", font=hud_f)
        tcol = "#FF4444" if m["time_left"] <= 100 else "#FFFFFF"
        canvas.create_text(324, 9, anchor="nw", text=f"TIME {m['time_left']:03d}", fill=tcol, font=hud_f)
        snd_txt = "BGM" if m["sound"] == 2 else ("SFX" if m["sound"] == 1 else "MUT")
        pwr_txt = "STR " if p_star else ("FIR " if p_pwr >= 2 else ("BIG " if p_pwr == 1 else ""))
        canvas.create_text(cw - 10, 9, anchor="ne", text=f"{pwr_txt}L:{m['lives']} {m['fps']}F {snd_txt}", fill="#7EE787", font=hud_f)

        # Overlays
        if st == STATE_TITLE:
            canvas.create_rectangle(84, (HUD_H + 20) * SCALE, cw - 84, (HUD_H + 60) * SCALE, fill="#B00000", outline=col_gold, width=3)
            canvas.create_text(cw // 2, (HUD_H + 40) * SCALE, text="SUPER MARIO BROS", fill=col_gold, font=("Monospace", 20, "bold"))
            canvas.create_text(cw // 2, (HUD_H + 5 * 16 + 8) * SCALE, text=f"START WORLD: {m['title_world']}-1  (Press S to cycle)", fill="#FFFFFF", font=("Monospace", 10, "bold"))
            canvas.create_text(cw // 2, (HUD_H + 7 * 16 + 8) * SCALE, text="K3/Spc:SOLO   K1:HOST   K2:JOIN   K4/M:SND", fill=col_gold, font=("Monospace", 10, "bold"))
            if (frame // 18) & 1:
                canvas.create_text(cw // 2, (HUD_H + 9 * 16 + 8) * SCALE, text="32 ORIGINAL LEVELS EDITION", fill="#FFFFFF", font=("Monospace", 10, "bold"))
        elif st == STATE_PRE_STAGE:
            canvas.create_text(cw // 2, (HUD_H + 5 * 16 + 8) * SCALE, text=f"WORLD   {w_num}-{s_num}", fill="#FFFFFF", font=("Monospace", 18, "bold"))
            canvas.create_image(cw // 2 - 48, (HUD_H + 7 * 16) * SCALE, anchor="nw", image=tk_player_norm[0][0])
            canvas.create_text(cw // 2 + 18, (HUD_H + 7 * 16 + 14) * SCALE, text=f"*   {m['lives']}", fill="#FFFFFF", font=("Monospace", 16, "bold"))
        elif st == STATE_LOBBY:
            canvas.create_text(cw // 2, (HUD_H + 2 * 16 + 8) * SCALE, text="WIFI LOBBY [MARIO-MP]", fill=col_gold, font=("Monospace", 13, "bold"))
            role_txt = "ROLE: HOST [1P RED]" if role == WIFI_MODE_HOST else "ROLE: JOIN [2-4P]"
            canvas.create_text(cw // 2, (HUD_H + 4 * 16 + 8) * SCALE, text=role_txt, fill="#FFFFFF", font=("Monospace", 11, "bold"))
            st_num = m["net_stage"]
            if st_num == 3:
                stg_txt = "START AP MARIO-MP" if role == WIFI_MODE_HOST else "JOIN MARIO-MP..."
            elif st_num == 6:
                stg_txt = "HOST READY" if role == WIFI_MODE_HOST else "WAIT HOST ACK..."
            else:
                stg_txt = WIFI_STAGE_TEXT.get(st_num, f"STAGE {st_num}")
            canvas.create_text(cw // 2, (HUD_H + 6 * 16 + 8) * SCALE, text=stg_txt, fill=col_gold, font=("Monospace", 12, "bold"))
            slots_up = [False, False, False, False]
            if role == WIFI_MODE_HOST:
                slots_up[0] = True
            elif st_num == 7:
                slots_up[my_slot] = True
            for _rx, _ry, _rspr, _rflags, ractive, rslot in m["peers"]:
                if ractive and rslot < 4:
                    slots_up[rslot] = True
            p_line = "  ".join(f"{i + 1}P:{'OK' if slots_up[i] else '--'}" for i in range(4))
            canvas.create_text(cw // 2, (HUD_H + 8 * 16 + 8) * SCALE, text=p_line, fill="#FFFFFF", font=("Monospace", 12, "bold"))
            hint = "K3/Spc: START GAME   K4/M: CANCEL" if role == WIFI_MODE_HOST else "WAIT HOST START     K4/M: CANCEL"
            canvas.create_text(cw // 2, (HUD_H + 10 * 16 + 8) * SCALE, text=hint, fill=col_gold, font=("Monospace", 10, "bold"))
        elif st == STATE_GAME_OVER:
            canvas.create_text(cw // 2, (HUD_H + 6 * 16 + 10) * SCALE, text="GAME OVER", fill=col_red, font=("Monospace", 22, "bold"))
            canvas.create_text(cw // 2, (HUD_H + 8 * 16 + 8) * SCALE, text="PRESS K3 OR W/SPACE TO CONTINUE", fill="#FFFFFF", font=("Monospace", 11, "bold"))
        elif st == STATE_LEVEL_CLEAR:
            if s_num == 4:
                canvas.create_text(cw // 2, (HUD_H + 4 * 16 + 8) * SCALE, text="THANK YOU MARIO!", fill="#FFFFFF", font=("Monospace", 15, "bold"))
                sub_t = "YOUR QUEST IS OVER!" if m["level"] == 32 else "OUR PRINCESS IS IN ANOTHER CASTLE!"
                canvas.create_text(cw // 2, (HUD_H + 6 * 16 + 8) * SCALE, text=sub_t, fill=col_gold, font=("Monospace", 11, "bold"))
            else:
                canvas.create_text(cw // 2, (HUD_H + 5 * 16 + 10) * SCALE, text="COURSE CLEAR!", fill=col_gold, font=("Monospace", 22, "bold"))
        elif st == STATE_WIN:
            canvas.create_text(cw // 2, (HUD_H + 4 * 16 + 10) * SCALE, text="CONGRATULATIONS!", fill=col_gold, font=("Monospace", 18, "bold"))
            canvas.create_text(cw // 2, (HUD_H + 6 * 16 + 8) * SCALE, text="ALL 32 ORIGINAL STAGES CLEARED!", fill="#FFFFFF", font=("Monospace", 11, "bold"))
            canvas.create_text(cw // 2, (HUD_H + 8 * 16 + 8) * SCALE, text="PRESS K3 OR W/SPACE TO REPLAY", fill=col_gold, font=("Monospace", 10, "bold"))

    def tick():
        if backend.fd >= 0:
            txt = backend.read_logs()
            if txt:
                sys.stdout.write(txt.replace("\r\n", "\n"))
                sys.stdout.flush()
        push_keys(force=False)
        if backend.can_mirror:
            m = backend.read_mirror()
            if m is not None:
                render_mirror(m)
                state["fps_n"] += 1
                now = time.monotonic()
                dt = now - state["fps_t0"]
                if dt >= 1.0:
                    state["pc_fps"] = int(state["fps_n"] / dt)
                    state["fps_n"] = 0
                    state["fps_t0"] = now
                    bar_var.set(f"MCU:{m['fps']}F PC:{state['pc_fps']}F | AD:Move W/Spc:Jump S:Pipe F:Fire Shf:Run M:Mute")
        root.after(16, tick)

    root.bind("<KeyPress>", on_key_press)
    root.bind("<KeyRelease>", on_key_release)
    root.bind("<FocusOut>", on_focus_out)
    root.after(16, tick)
    root.focus_force()
    try:
        root.mainloop()
    finally:
        backend.close()


def run_cli(backend):
    stdin_fd = sys.stdin.fileno()
    old_tty = termios.tcgetattr(stdin_fd)
    print(f"Connected via {backend.name} (CLI mode)")
    print("Controls: A/Left=Left | D/Right=Right | W/Space/Up=Jump/Start | S=Stop | M=Mute | Q=Quit")

    hold = {"L": 0.0, "R": 0.0, "J": 0.0, "M": 0.0}
    last_sent = -1
    last_hb = 0.0
    watch_fds = [stdin_fd] + ([backend.fd] if backend.fd >= 0 else [])

    try:
        tty.setraw(stdin_fd)
        while True:
            rlist, _, _ = select.select(watch_fds, [], [], 0.02)
            now = time.monotonic()

            if backend.fd >= 0 and backend.fd in rlist:
                txt = backend.read_logs()
                if txt:
                    sys.stdout.write(txt)
                    sys.stdout.flush()

            if stdin_fd in rlist:
                raw = os.read(stdin_fd, 32)
                if not raw:
                    break
                i = 0
                while i < len(raw):
                    b = raw[i]
                    if b == 3 or b in (ord("q"), ord("Q")):
                        backend.send_mask(0)
                        return
                    if b == 0x1B and i + 2 < len(raw) and raw[i + 1] == ord("["):
                        code = raw[i + 2]
                        if code == ord("D"):
                            hold["L"] = now + 0.55
                            hold["R"] = 0.0
                        elif code == ord("C"):
                            hold["R"] = now + 0.55
                            hold["L"] = 0.0
                        elif code == ord("A"):
                            hold["J"] = now + 0.25
                        i += 3
                        continue
                    ch = chr(b).lower()
                    if ch in ("a", "1"):
                        hold["L"] = now + 0.55
                        hold["R"] = 0.0
                    elif ch in ("d", "2"):
                        hold["R"] = now + 0.55
                        hold["L"] = 0.0
                    elif ch == "s":
                        hold["L"] = 0.0
                        hold["R"] = 0.0
                    elif ch in ("w", " ", "\r", "\n", "3"):
                        hold["J"] = now + 0.25
                    elif ch in ("m", "4"):
                        hold["M"] = now + 0.06
                    i += 1

            mask = 0
            if now < hold["L"]:
                mask |= 0x01
            if now < hold["R"]:
                mask |= 0x02
            if now < hold["J"]:
                mask |= 0x04
            if now < hold["M"]:
                mask |= 0x08

            if mask != last_sent or (mask != 0 and now - last_hb >= 0.06):
                backend.send_mask(mask)
                last_sent = mask
                last_hb = now
    finally:
        termios.tcsetattr(stdin_fd, termios.TCSADRAIN, old_tty)
        backend.close()
        print("\r\nExited PC controller.")


def main():
    use_cli = "--cli" in sys.argv[1:]
    backend = open_backend(sys.argv[1:])
    if not use_cli and (os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        try:
            run_tk_gui(backend)
            return
        except Exception as exc:
            print(f"GUI fallback to CLI ({exc})")
    run_cli(backend)


if __name__ == "__main__":
    main()
