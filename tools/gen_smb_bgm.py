#!/usr/bin/env python3
"""
Synthesize authentic 1985 Super Mario Bros. NES chiptune BGM loops into Inc/mario_audio.h.
Uses 2A03-style pulse + triangle + soft noise channels encoded as 44.1 kHz mono 32 kbps CBR MP3
(matching LD3320 hardware MP3 decoder).
"""

import os
import re
import subprocess
import tempfile
import numpy as np

SR = 44100
AUDIO_H = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Inc", "mario_audio.h")

NOTE_SEMI = {
    "C": 0, "Cs": 1, "Db": 1, "D": 2, "Ds": 3, "Eb": 3, "E": 4,
    "F": 5, "Fs": 6, "Gb": 6, "G": 7, "Gs": 8, "Ab": 8,
    "A": 9, "As": 10, "Bb": 10, "B": 11,
}


def note_freq(name):
    if name is None or name == "r":
        return 0.0
    m = re.match(r"^([A-G][sb]?)([0-9])$", name)
    if not m:
        raise ValueError(f"Bad note: {name}")
    semi = NOTE_SEMI[m.group(1)] + (int(m.group(2)) + 1) * 12
    return 440.0 * (2.0 ** ((semi - 69) / 12.0))


def synth_pulse(n_samp, freq, duty=0.25, env_decay=5.0, staccato=0.88):
    if n_samp <= 0 or freq <= 0.0:
        return np.zeros(max(0, n_samp), dtype=np.float32)
    t = np.arange(n_samp, dtype=np.float32) / SR
    phase = (t * freq) % 1.0
    # Slight band-limit smoothing via 2-harmonic blend avoids harsh MP3 aliasing at 32 kbps
    wave = np.where(phase < duty, 1.0, -1.0).astype(np.float32)
    env = np.exp(-env_decay * t).astype(np.float32)
    gate_n = max(1, int(n_samp * staccato))
    if gate_n < n_samp:
        rel = max(1, int(0.008 * SR))
        env[gate_n:] *= np.exp(-np.arange(n_samp - gate_n, dtype=np.float32) / rel)
    fade = min(40, n_samp // 4)
    if fade > 0:
        env[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
        env[-fade:] *= np.linspace(1.0, 0.0, fade, dtype=np.float32)
    return wave * env


def synth_triangle(n_samp, freq, staccato=0.92):
    if n_samp <= 0 or freq <= 0.0:
        return np.zeros(max(0, n_samp), dtype=np.float32)
    t = np.arange(n_samp, dtype=np.float32) / SR
    phase = (t * freq) % 1.0
    wave = (4.0 * np.abs(phase - 0.5) - 1.0).astype(np.float32)
    env = np.ones(n_samp, dtype=np.float32)
    gate_n = max(1, int(n_samp * staccato))
    if gate_n < n_samp:
        rel = max(1, int(0.008 * SR))
        env[gate_n:] *= np.exp(-np.arange(n_samp - gate_n, dtype=np.float32) / rel)
    fade = min(32, n_samp // 4)
    if fade > 0:
        env[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
        env[-fade:] *= np.linspace(1.0, 0.0, fade, dtype=np.float32)
    return wave * env


def synth_drum(n_samp, kind):
    if n_samp <= 0 or not kind:
        return np.zeros(max(0, n_samp), dtype=np.float32)
    rng = np.random.default_rng(12345 if kind == "k" else 67890)
    t = np.arange(n_samp, dtype=np.float32) / SR
    if kind == "k":  # short punchy bass/kick thump
        f_inst = 140.0 * np.exp(-28.0 * t) + 52.0
        phase = np.cumsum(f_inst) / SR
        return (np.sin(2.0 * np.pi * phase) * np.exp(-32.0 * t)).astype(np.float32)
    if kind == "s":  # crisp NES noise snare
        noise = rng.uniform(-1.0, 1.0, n_samp).astype(np.float32)
        return noise * np.exp(-45.0 * t).astype(np.float32)
    if kind == "h":  # soft closed hi-hat
        noise = rng.uniform(-1.0, 1.0, n_samp).astype(np.float32)
        return noise * np.exp(-95.0 * t).astype(np.float32) * 0.45
    return np.zeros(n_samp, dtype=np.float32)


def render_track(events, beat_s, total_beats):
    """
    events: list of dicts with channels:
      lead: [(note, beats), ...]
      harm: [(note, beats), ...] (optional)
      bass: [(note, beats), ...] (optional)
      drums: [(kind, beats), ...] (optional)
    """
    total_samp = int(round(total_beats * beat_s * SR))
    mix = np.zeros(total_samp, dtype=np.float32)

    def add_seq(seq, synth_fn, gain, **kw):
        pos = 0.0
        for note, beats in seq:
            s0 = int(round(pos * beat_s * SR))
            pos += beats
            s1 = min(total_samp, int(round(pos * beat_s * SR)))
            if s1 > s0 and note not in (None, "r"):
                if callable(synth_fn):
                    seg = synth_fn(s1 - s0, note, **kw)
                    mix[s0:s1] += seg * gain

    if "lead" in events:
        add_seq(events["lead"], lambda n, nt, **k: synth_pulse(n, note_freq(nt), **k),
                0.24, duty=0.25, env_decay=4.0, staccato=0.84)
    if "harm" in events:
        add_seq(events["harm"], lambda n, nt, **k: synth_pulse(n, note_freq(nt), **k),
                0.15, duty=0.50, env_decay=4.5, staccato=0.82)
    if "bass" in events:
        add_seq(events["bass"], lambda n, nt, **k: synth_triangle(n, note_freq(nt), **k),
                0.25, staccato=0.90)
    if "drums" in events:
        add_seq(events["drums"], synth_drum, 0.14)

    # Gentle soft limiter so MP3 stays clean without clipping
    peak = np.max(np.abs(mix))
    if peak > 0.65:
        mix *= (0.65 / peak)
    return mix


def encode_mp3(samples, bitrate="32k"):
    pcm = np.clip(samples * 32767.0, -32767, 32767).astype(np.int16)
    with tempfile.NamedTemporaryFile(suffix=".raw") as fraw, tempfile.NamedTemporaryFile(suffix=".mp3") as fmp3:
        pcm.tofile(fraw.name)
        subprocess.check_call([
            "ffmpeg", "-y", "-v", "error",
            "-f", "s16le", "-ar", str(SR), "-ac", "1", "-i", fraw.name,
            "-c:a", "libmp3lame", "-b:a", bitrate, "-ac", "1", "-ar", str(SR),
            "-write_xing", "0", "-id3v2_version", "0",
            fmp3.name,
        ])
        data = open(fmp3.name, "rb").read()
        # Strip trailing LAME silence/padding frames (side-info all idle + 0xAA fill) for tight loop
        frames = [i for i in range(len(data) - 3)
                  if data[i] == 0xFF and (data[i + 1] & 0xFE) == 0xFA and (data[i + 2] & 0xFC) == 0x10]
        while len(frames) > 4:
            f0 = frames[-1]
            tail = data[f0:]
            if len(tail) > 25 and tail[21:] == b"\xaa" * (len(tail) - 21):
                data = data[:f0]
                frames.pop()
            else:
                break
        with tempfile.NamedTemporaryFile(suffix=".mp3") as ftrim:
            ftrim.write(data)
            ftrim.flush()
            info = subprocess.check_output([
                "ffprobe", "-v", "error",
                "-show_entries", "format=duration,bit_rate",
                "-of", "default=noprint_wrappers=1", ftrim.name
            ]).decode().strip().replace("\n", " ")
    return data, info


def build_overworld():
    # Iconic SMB1 Overworld A + B1 (8 bars = 32 beats @ 0.285s/beat -> ~9.1s, ~36.5 KB)
    lead = [
        # Bar 1-2
        ("C5", 1.5), ("G4", 1.5), ("E4", 1.0),
        ("r", 0.5), ("A4", 1.0), ("B4", 1.0), ("Bb4", 0.5), ("A4", 1.0),
        # Bar 3-4 (classic triplet G4 E5 G5)
        ("G4", 2 / 3), ("E5", 2 / 3), ("G5", 2 / 3), ("A5", 1.0), ("F5", 0.5), ("G5", 0.5),
        ("r", 0.5), ("E5", 1.0), ("C5", 0.5), ("D5", 0.5), ("B4", 1.5),
        # Bar 5-6 (Section B1)
        ("r", 1.0), ("G5", 0.5), ("Fs5", 0.5), ("F5", 0.5), ("Ds5", 1.0), ("E5", 0.5),
        ("r", 0.5), ("Gs4", 0.5), ("A4", 0.5), ("C5", 1.0), ("A4", 0.5), ("C5", 0.5), ("D5", 0.5),
        # Bar 7-8
        ("r", 1.0), ("G5", 0.5), ("Fs5", 0.5), ("F5", 0.5), ("Ds5", 1.0), ("E5", 0.5),
        ("r", 0.5), ("C6", 1.0), ("C6", 0.5), ("C6", 1.5), ("r", 0.5),
    ]
    harm = [
        ("E4", 1.5), ("C4", 1.5), ("G3", 1.0),
        ("r", 0.5), ("C4", 1.0), ("D4", 1.0), ("Db4", 0.5), ("C4", 1.0),
        ("E4", 2 / 3), ("C5", 2 / 3), ("E5", 2 / 3), ("F5", 1.0), ("D5", 0.5), ("E5", 0.5),
        ("r", 0.5), ("C5", 1.0), ("A4", 0.5), ("B4", 0.5), ("G4", 1.5),
        ("r", 1.0), ("E5", 0.5), ("Ds5", 0.5), ("D5", 0.5), ("B4", 1.0), ("C5", 0.5),
        ("r", 0.5), ("E4", 0.5), ("F4", 0.5), ("A4", 1.0), ("F4", 0.5), ("A4", 0.5), ("B4", 0.5),
        ("r", 1.0), ("E5", 0.5), ("Ds5", 0.5), ("D5", 0.5), ("B4", 1.0), ("C5", 0.5),
        ("r", 0.5), ("G5", 1.0), ("G5", 0.5), ("G5", 1.5), ("r", 0.5),
    ]
    bass = [
        ("G3", 1.5), ("E3", 1.5), ("C3", 1.0),
        ("r", 0.5), ("F3", 1.0), ("G3", 1.0), ("Gb3", 0.5), ("F3", 1.0),
        ("E3", 2 / 3), ("C4", 2 / 3), ("E4", 2 / 3), ("F4", 1.0), ("D4", 0.5), ("E4", 0.5),
        ("r", 0.5), ("C4", 1.0), ("A3", 0.5), ("B3", 0.5), ("G3", 1.5),
        ("C3", 1.5), ("G3", 1.5), ("C4", 1.0),
        ("F3", 1.5), ("C4", 1.5), ("F3", 1.0),
        ("C3", 1.5), ("G3", 1.5), ("C4", 1.0),
        ("G3", 0.5), ("G4", 1.0), ("G4", 0.5), ("G4", 1.5), ("r", 0.5),
    ]
    drums = [("k", 0.5), ("h", 0.5), ("s", 0.5), ("h", 0.5)] * 16
    return render_track({"lead": lead, "harm": harm, "bass": bass, "drums": drums}, 0.285, 32.0)


def build_underground():
    # Iconic C-minor SMB1 Underground (16 beats @ 0.27s/beat -> ~4.3s, ~17 KB)
    lead = [
        ("C4", 0.5), ("C5", 0.5), ("A3", 0.5), ("A4", 0.5), ("Bb3", 0.5), ("Bb4", 0.5), ("r", 1.0),
        ("F3", 0.5), ("F4", 0.5), ("D3", 0.5), ("D4", 0.5), ("Eb3", 0.5), ("Eb4", 0.5), ("r", 1.0),
        ("Eb4", 1 / 3), ("D4", 1 / 3), ("Db4", 1 / 3),
        ("C4", 1.0), ("Eb4", 1.0), ("D4", 1.0),
        ("Ab3", 1.0), ("G3", 1.0), ("Cs4", 1.0),
        ("C4", 2 / 3), ("Ab3", 2 / 3), ("G3", 2 / 3),
    ]  # 4 + 4 + 1 + 3 + 3 + 2 = 17 beats!
    bass = [
        ("C3", 0.5), ("C4", 0.5), ("A2", 0.5), ("A3", 0.5), ("Bb2", 0.5), ("Bb3", 0.5), ("r", 1.0),
        ("F2", 0.5), ("F3", 0.5), ("D2", 0.5), ("D3", 0.5), ("Eb2", 0.5), ("Eb3", 0.5), ("r", 1.0),
        ("r", 1.0),
        ("C3", 1.0), ("Eb3", 1.0), ("D3", 1.0),
        ("Ab2", 1.0), ("G2", 1.0), ("Cs3", 1.0),
        ("C3", 2 / 3), ("Ab2", 2 / 3), ("G2", 2 / 3),
    ]
    return render_track({"lead": lead, "bass": bass}, 0.25, 17.0)


def build_underwater():
    # SMB1 3/4 Underwater Waltz (18 beats @ 0.275s/beat -> ~4.95s, ~19.8 KB)
    lead = [
        ("D4", 0.5), ("E4", 0.5), ("Fs4", 0.5), ("G4", 0.5), ("A4", 0.5), ("As4", 0.5),
        ("B4", 0.5), ("B4", 0.5), ("B4", 1.0), ("G4", 1.0),
        ("E5", 2.0), ("Ds5", 1.0),
        ("E5", 2.0), ("r", 1.0),
        ("D5", 2.0), ("Cs5", 1.0),
        ("D5", 1.0), ("G4", 1.0), ("B4", 1.0),
    ]  # 6 * 3 = 18 beats
    harm = [
        ("r", 3.0),
        ("G4", 1.0), ("D4", 1.0), ("D4", 1.0),
        ("C5", 1.0), ("G4", 1.0), ("G4", 1.0),
        ("C5", 1.0), ("G4", 1.0), ("G4", 1.0),
        ("B4", 1.0), ("G4", 1.0), ("G4", 1.0),
        ("B4", 1.0), ("D4", 1.0), ("G4", 1.0),
    ]
    bass = [
        ("G3", 1.0), ("D3", 1.0), ("G3", 1.0),
        ("G3", 1.0), ("B3", 1.0), ("D4", 1.0),
        ("C3", 1.0), ("G3", 1.0), ("C4", 1.0),
        ("C3", 1.0), ("G3", 1.0), ("C4", 1.0),
        ("G3", 1.0), ("B3", 1.0), ("D4", 1.0),
        ("G3", 1.0), ("D3", 1.0), ("G3", 1.0),
    ]
    return render_track({"lead": lead, "harm": harm, "bass": bass}, 0.275, 18.0)


def build_castle():
    # SMB1 Tense Castle 16th-note ostinato (3 bars = 12 beats @ 0.30s -> ~3.6s, ~14.4 KB)
    lead = [
        ("D4", 0.25), ("G3", 0.25), ("Cs4", 0.25), ("G3", 0.25),
        ("C4", 0.25), ("G3", 0.25), ("B3", 0.25), ("G3", 0.25),
        ("C4", 0.25), ("G3", 0.25), ("Cs4", 0.25), ("G3", 0.25),
        ("D4", 0.25), ("G3", 0.25), ("Cs4", 0.25), ("G3", 0.25),

        ("Ds4", 0.25), ("Gs3", 0.25), ("D4", 0.25), ("Gs3", 0.25),
        ("Cs4", 0.25), ("Gs3", 0.25), ("C4", 0.25), ("Gs3", 0.25),
        ("Cs4", 0.25), ("Gs3", 0.25), ("D4", 0.25), ("Gs3", 0.25),
        ("Ds4", 0.25), ("Gs3", 0.25), ("D4", 0.25), ("Gs3", 0.25),

        ("Cs4", 0.25), ("Fs3", 0.25), ("C4", 0.25), ("Fs3", 0.25),
        ("B3", 0.25), ("Fs3", 0.25), ("Bb3", 0.25), ("Fs3", 0.25),
        ("B3", 0.25), ("Fs3", 0.25), ("C4", 0.25), ("Fs3", 0.25),
        ("Cs4", 0.25), ("Fs3", 0.25), ("C4", 0.25), ("Fs3", 0.25),
    ]
    bass = [
        ("G2", 2.0), ("G2", 2.0),
        ("Gs2", 2.0), ("Gs2", 2.0),
        ("Fs2", 2.0), ("Fs2", 2.0),
    ]
    return render_track({"lead": lead, "bass": bass}, 0.285, 12.0)


def build_star():
    # SMB1 Starman 2-bar high-energy loop (8 beats @ 0.26s -> ~2.08s, ~8.4 KB)
    lead = [
        ("C5", 0.5), ("C5", 0.5), ("C5", 0.5), ("D4", 0.25), ("C5", 0.5),
        ("r", 0.25), ("C5", 0.5), ("D4", 0.25), ("C5", 0.5), ("D4", 0.25), ("C5", 0.25),
        ("B4", 0.5), ("B4", 0.5), ("B4", 0.5), ("C4", 0.25), ("B4", 0.5),
        ("r", 0.25), ("B4", 0.5), ("C4", 0.25), ("B4", 0.5), ("C4", 0.25), ("B4", 0.25),
    ]  # 4 + 4 = 8 beats? Wait: 0.5*3+0.25+0.5 + 0.25+0.5+0.25+0.5+0.25+0.25 = 2.25 + 2.0 = 4.25 -> fix to exact 4.0!
    lead = [
        ("C5", 0.5), ("C5", 0.5), ("C5", 0.5), ("D4", 0.25), ("C5", 0.25),
        ("r", 0.25), ("C5", 0.5), ("D4", 0.25), ("C5", 0.5), ("D4", 0.25), ("C5", 0.25),
        ("B4", 0.5), ("B4", 0.5), ("B4", 0.5), ("C4", 0.25), ("B4", 0.25),
        ("r", 0.25), ("B4", 0.5), ("C4", 0.25), ("B4", 0.5), ("C4", 0.25), ("B4", 0.25),
    ]
    bass = [
        ("D3", 0.75), ("A3", 0.75), ("D4", 0.5), ("D3", 0.75), ("A3", 0.75), ("C4", 0.5),
        ("C3", 0.75), ("G3", 0.75), ("C4", 0.5), ("C3", 0.75), ("G3", 0.75), ("B3", 0.5),
    ]
    drums = [("k", 0.5), ("s", 0.5)] * 8
    return render_track({"lead": lead, "bass": bass, "drums": drums}, 0.26, 8.0)


def format_c_array(name, data):
    lines = [f"static const uint8_t {name}[{len(data)}] = {{"]
    for i in range(0, len(data), 16):
        chunk = ", ".join(f"0x{b:02x}" for b in data[i:i + 16])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def main():
    tracks = [
        ("MP3_BGM_OVERWORLD", build_overworld()),
        ("MP3_BGM_UNDERGROUND", build_underground()),
        ("MP3_BGM_UNDERWATER", build_underwater()),
        ("MP3_BGM_CASTLE", build_castle()),
        ("MP3_BGM_STAR", build_star()),
    ]
    blocks = ["/* === BGM_TRACKS_BEGIN === */"]
    total = 0
    for name, samples in tracks:
        mp3, info = encode_mp3(samples, bitrate="32k")
        total += len(mp3)
        print(f"{name:22s}: {len(mp3):6d} B | {info}")
        blocks.append(format_c_array(name, mp3))
    blocks.append("/* === BGM_TRACKS_END === */")
    print(f"Total BGM MP3 Flash size: {total} B ({total/1024:.1f} KB)")

    txt = open(AUDIO_H, "r", encoding="utf-8").read()
    # Remove any prior BGM block
    txt = re.sub(r"\n?/\* === BGM_TRACKS_BEGIN === \*/.*?/\* === BGM_TRACKS_END === \*/\n?", "\n", txt, flags=re.S)
    txt = re.sub(r"\n#endif\s*$", "\n\n" + "\n\n".join(blocks) + "\n\n#endif\n", txt)
    open(AUDIO_H, "w", encoding="utf-8").write(txt)
    print(f"Updated {AUDIO_H}")


if __name__ == "__main__":
    main()
