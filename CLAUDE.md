# SA:COOP Friends (fork of SA:COOP) — notes for Claude

Fork of github.com/Karimov-Akbar/sa-coop (MIT): a GTA San Andreas 1.0 US co-op mod. The fork aims at up to 4 friends,
reliable connections and fewer drops to the menu. The owner writes in Russian: answer in Russian, keep docs in Russian
like the upstream docs, code comments in English like the upstream code.

Read first, in this order:
0. `docs/fork/HANDOFF.md` — where the work stands now: the PR chain, what waits for a test in the game, how to run
   the multi-copy stand, what is next. All current work is on branch `four-players` (`main` has not got it yet).
   Update its "Состояние" and "Дальше" at the end of a session.
1. `docs/fork/KNOWLEDGE.md` — everything learned so far: architecture facts, traffic numbers, verified findings,
   community PRs, upstream issues, decisions, current state.
2. `docs/fork/ROADMAP.md` — phases, priorities, decisions.
3. `docs/fork/BACKLOG.md` — tasks with code links (lines are of upstream `636b515`) and acceptance criteria.
4. `docs/fork/GETTING_STARTED.md` — build, multi-copy test stand, workflow, releases.
5. `docs/ARCHITECTURE.md` — the author's detailed design notes (Russian).

Rules:
- Build: `powershell -ExecutionPolicy Bypass -File tools/get-toolchain.ps1`, then `python tools/build.py dist`.
  Checks: `python tools/fsd_check.py`, `python tools/i18n_check.py`.
- `COOP_VERSION_STR` must be digits and dots only (build.py parses it). Fork versions are `0.3.0.N`.
- Any protocol change bumps `kForkProtocolRevision` in `Version.h` (protocol = 1000 + 19 + revision, BACKLOG INF-03).
- Follow the author's style: FSD layers, `TR("ru", "en")` for every game text, comments explain why and name engine
  functions next to addresses, network data goes through `Reader` and is validated before touching the engine.
- Claude cannot run the game. Network/server changes are verified with the bot (`coop-bot.exe`, scenarios in
  `src/bot/features/scenarios/Scenarios.cpp`): `python tools/build.py server bot`, then
  `wine build/release/bin/coop-bot.exe` on Linux (Wine with wine32) or `tools/fork/bot-tests.ps1` on Windows. A network
  bug gets a scenario first. Game changes are verified by the owner on the multi-copy stand. Say which was done.
- Building on Linux: llvm-mingw `20261006` `ucrt-ubuntu-22.04-x86_64`, with wrapper scripts named
  `toolchain/llvm-mingw/bin/i686-w64-mingw32-{clang,clang++,windres}.exe` (symlinks break clang++'s driver mode).
- The fork goes its own way (owner's decision 25.10): never merge upstream, never send changes upstream. Watch the
  author's releases and port good fixes with `git cherry-pick -x` or by hand citing the commit (INF-09). Changes are
  targeted: fix and extend the author's design, do not rewrite it.
- Fork docs live in `docs/fork/`; `docs/ARCHITECTURE.md` is the author's and stays untouched.
- Update `docs/fork/KNOWLEDGE.md` and the backlog when something new is learned or a task is done.
- Legal (LEGAL.md, NOTICE.md): never commit game files, data extracted from them, decompiled or leaked Rockstar code
  or `gta_sa.pdb` symbols; keep `LICENSE` and the author's history untouched; call the project a fork and link the
  original; credit community PR authors. `python -I tools/fork/guard_check.py` must pass.
- The fork's front page is `.github/README.md`; the author's `README.md` stays as upstream has it.
