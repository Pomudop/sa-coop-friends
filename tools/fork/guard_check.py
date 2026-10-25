#!/usr/bin/env python3
"""
Keeps the fork safe to publish: no game files in git, the original author's copyright intact (see LEGAL.md).

    python -I tools/fork/guard_check.py

Looks at the files git tracks (git ls-files), so a game folder lying around untracked is fine. Exits 1 with the list
of problems.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# Rockstar's files and anything built from them: executables and plugins, archives, scripts, models, textures,
# animations, cutscenes, texts, audio streams, symbols, saves. The mod reads all of this from the player's own game.
GAME_EXT = {
    ".exe", ".dll", ".asi", ".img", ".scm", ".dff", ".txd", ".col", ".ifp", ".cut", ".gxt", ".fxp",
    ".adf", ".wav", ".mp3", ".ogg", ".pdb",
}
GAME_NAMES = {"gta_sa.exe", "gta.dat", "default.dat", "handling.cfg", "main.scm", "script.img", "american.gxt"}
MAX_SIZE = 5 * 1024 * 1024  # nothing in the project comes close; a bigger file is almost surely not ours
AUTHOR_COPYRIGHT = "Copyright (c) 2026 Karimov-Akbar"
REQUIRED = ["LICENSE", "NOTICE.md", "LEGAL.md"]


def tracked():
    out = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, check=True).stdout
    return [p for p in out.decode("utf-8").split("\0") if p]


def main():
    problems = []
    for rel in tracked():
        path = ROOT / rel
        name = path.name.lower()
        if path.suffix.lower() in GAME_EXT or name in GAME_NAMES:
            problems.append(f"{rel}: game file or binary, it must not be in git (LEGAL.md)")
        elif name.startswith("gtasasf") and name.endswith(".b"):
            problems.append(f"{rel}: game save, it must not be in git")
        elif path.is_file() and path.stat().st_size > MAX_SIZE:
            problems.append(f"{rel}: {path.stat().st_size // 1024} KB, larger than {MAX_SIZE // 1024 // 1024} MB")
    for rel in REQUIRED:
        if not (ROOT / rel).is_file():
            problems.append(f"{rel}: missing")
    license_file = ROOT / "LICENSE"
    if license_file.is_file() and AUTHOR_COPYRIGHT not in license_file.read_text(encoding="utf-8", errors="replace"):
        problems.append(f"LICENSE: the original author's line \"{AUTHOR_COPYRIGHT}\" is gone (MIT requires it)")

    if problems:
        print("guard_check: problems found")
        for p in problems:
            print("  " + p)
        return 1
    print("guard_check: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
