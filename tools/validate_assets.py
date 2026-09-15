from pathlib import Path
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "assets/characters/rayden_clean.png": (384, 384, (6,)),
    "assets/enemies/punk_clean.png": (512, 384, (6,)),
    "assets/enemies/charger_clean.png": (512, 384, (6,)),
    "assets/enemies/brute_clean.png": (512, 384, (6,)),
    "assets/enemies/enforcer_clean.png": (512, 384, (6,)),
    # The authored cinematic background is intentionally RGB: it has no transparency requirement.
    "assets/backgrounds/old_steel_yard_clean.png": (1280, 720, (2, 6)),
}
ENEMY_ATLAS_NAMES = (
    "punk", "charger", "brute", "enforcer",
    "chemical_soldier", "urban_ninja", "mutant", "armored_guard",
)
EXPECTED_ENEMY_CLIPS = {
    "idle": [0, 1, 2, 3],
    "walk": [4, 5, 6, 7],
    "attack": [8, 9],
    "defeat": [10, 11],
}
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def png_info(path: Path):
    raw = path.read_bytes()
    assert raw.startswith(PNG_SIGNATURE), f"{path}: not a PNG"
    length = struct.unpack(">I", raw[8:12])[0]
    assert raw[12:16] == b"IHDR" and length >= 13, f"{path}: missing IHDR"
    width, height, bit_depth, color_type = struct.unpack(">IIBB", raw[16:26])
    return width, height, bit_depth, color_type


def main():
    manifest = json.loads((ROOT / "data" / "sprite_manifest.json").read_text(encoding="utf-8"))
    atlases = manifest["atlases"]
    clips = manifest["clips"]["enemy_default"]

    for name in ENEMY_ATLAS_NAMES:
        atlas = atlases[name]
        assert atlas["grid"] == [4, 3], f"{name}: enemy atlas must be 4x3"
        assert atlas["cell"] == [128, 128], f"{name}: enemy cells must be 128x128"
        assert atlas["pivot"] == [64, 126], f"{name}: enemy pivot must be [64,126]"

    for clip_name, expected_frames in EXPECTED_ENEMY_CLIPS.items():
        assert clips[clip_name]["frames"] == expected_frames, (
            f"enemy_default/{clip_name}: got {clips[clip_name]['frames']}, expected {expected_frames}"
        )

    missing = []
    for rel, (w, h, color_types) in EXPECTED.items():
        p = ROOT / rel
        if not p.is_file():
            missing.append(rel)
            continue
        width, height, depth, color_type = png_info(p)
        assert (width, height) == (w, h), f"{rel}: got {width}x{height}, expected {w}x{h}"
        assert depth == 8, f"{rel}: expected 8-bit channels"
        assert color_type in color_types, f"{rel}: unexpected PNG color type {color_type}; expected one of {color_types}"

    for name, atlas in atlases.items():
        assert "path" in atlas, f"manifest atlas missing path: {name}"
        assert atlas["path"].endswith(".png"), f"{name}: clean runtime atlas must be PNG"

    if missing:
        print("WARN: authored art is not checked into this branch yet:")
        for rel in missing:
            print(f"  - {rel}")
        print("The runtime keeps its procedural fallback; the distributable Art Pack supplies the authored files.")
    else:
        print("OK: authored runtime assets + manifest validated")


if __name__ == "__main__":
    main()
