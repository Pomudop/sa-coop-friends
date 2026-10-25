# Передача дел: где мы сейчас

Короткая сводка для владельца и для Claude в новой сессии (облачной или локальной). Читать первым, потом
[KNOWLEDGE.md](KNOWLEDGE.md). Обновлять в конце каждой рабочей сессии: раздел «Состояние» и «Дальше».

Последнее обновление: 25.10.2026, локальная сессия Claude (ПК владельца).

## Где что лежит
- **Репозиторий:** [Pomudop/sa-coop-friends](https://github.com/Pomudop/sa-coop-friends) — форк
  [Karimov-Akbar/sa-coop](https://github.com/Karimov-Akbar/sa-coop) (MIT). Форк идёт своим путём: версии автора не
  сливаем, удачные исправления переносим вручную (`CLAUDE.md`, INF-09).
- **Вся работа — в ветке `four-players`.** В `main` пока только база знаний и правовые файлы: заявки ещё не слиты.
- **Заявки идут цепочкой**, каждая основана на предыдущей, CI у всех зелёный:

| PR | Ветка | Что | Проверено |
|---|---|---|---|
| [#1](https://github.com/Pomudop/sa-coop-friends/pull/1) | `inf-03-10-fork-identity` | Свой протокол и версия 0.3.0.1, форк виден в F9, логах, файлах, релизе | сборка; F9 — **стенд** |
| [#2](https://github.com/Pomudop/sa-coop-friends/pull/2) | `inf-05-bot` | Бот `coop-bot.exe` для сервера | бот, CI |
| [#3](https://github.com/Pomudop/sa-coop-friends/pull/3) | `net-01-02-hello-order` | NET-01, NET-02; решение «свой путь» | бот |
| [#4](https://github.com/Pomudop/sa-coop-friends/pull/4) | `net-04-relay-errors` | NET-04: причина сбоя ретранслятора хосту, код без 15 с | бот |
| [#5](https://github.com/Pomudop/sa-coop-friends/pull/5) | `net-05-turn-refresh` | NET-05: повтор продления TURN, новый адрес | бот |
| [#6](https://github.com/Pomudop/sa-coop-friends/pull/6) | `stb-01-vehicle-alive` | STB-01: удалённые машины после шага игры | сборка; **стенд** |
| [#7](https://github.com/Pomudop/sa-coop-friends/pull/7) | `ux-01-chat-f6` | UX-01: чат на F6 | сборка; **стенд** |
| [#8](https://github.com/Pomudop/sa-coop-friends/pull/8) | `stb-02-queue-drop-old` | STB-02: без «перемотки» после зависания | сборка; **стенд** |
| [#9](https://github.com/Pomudop/sa-coop-friends/pull/9) | `four-players` | Игра на четверых (MP-01, 02, 03, 06, 07, 08, 16, NET-03) и стенд `stand.ps1` | бот; игра — **стенд** |
| — | `conn-01-radmin` (от `four-players`) | CONN-01: адреса Radmin VPN и локальной сети в F9, `IP:порт`, `stand.ps1 -HostAddress` | бот через Radmin; F9 — **стенд** |

## Состояние
- **Сделано:** этап 0 (кроме INF-02 и INF-08), все P0 этапа 1, часть P1 (NET-02, UX-01, STB-02), основа этапа 3.
  Подробно — «Сделано …» под каждой задачей в [BACKLOG.md](BACKLOG.md) и [CHANGELOG.md](CHANGELOG.md).
- **Ничего из игровой части в игре не запускалось.** Сервер и протокол проверены ботом (17 сценариев), код игры —
  только сборкой и чтением. Пометка «ждёт стенда» в бэклоге = нужна проверка владельцем.
- **Открытые вопросы:** `GetPlayerInfoForThisPlayerPed` отдаёт всем копиям одну `CPlayerInfo` слота 1 (деньги и
  т. п.) — нужна ли своя, покажет стенд (MP-01). Места в катсценах для 3–4 игроков (MP-10) не сделаны.

## Как проверить на стенде (владельцу)
1. Скачать сборку: [заявка #9](https://github.com/Pomudop/sa-coop-friends/pull/9) → Checks → build → архив
   **SA-COOP**, распаковать, например, в `D:\Downloads\SA-COOP`. Или собрать самому: `python tools/build.py dist`.
2. Запустить стенд (игра — чистая 1.0 US, копии ставятся на тот же диск):
   ```powershell
   powershell -ExecutionPolicy Bypass -File tools/fork/stand.ps1 -Game "D:\Games\GTA San Andreas" -Count 4 -Save "D:\saves\GTASAsf1.b" -Package "D:\Downloads\SA-COOP"
   ```
   Тормозит — `-Width 640 -Height 360` или `-Count 3`. Только подготовить, без запуска, — `-NoLaunch`.
3. Пройти чек-лист и записать, что вышло:
   - [ ] F9: «Friends v0.3.0.1» не наезжает на RU/EN (#1).
   - [ ] Чат открывается на F6, чит AIWPRTON даёт танк (#7).
   - [ ] Свернуть одну копию на 10–20 с и развернуть — без «перемотки» людей и машин (#8).
   - [ ] Вчетвером у каждого своё оружие и одежда, стрельба без «кулаков» (#9, MP-01).
   - [ ] Игрок выходит (закрыть окно) за рулём машины с NPC — у остальных одна машина, без дублей (#9, MP-03).
   - [ ] Двое поджигают по машине — у третьего горят обе; потушили — гаснет у всех (#9, MP-06).
   - [ ] Гараж открывает один напарник — машины выезжают только у него (#9, MP-07).
   - [ ] Трое погибают в миссии — возрождаются рядом с хостом в разных местах (#9, MP-16).
   - [ ] Миссии «Home Invasion» и «Ryder» — без вылетов (#6).
4. Логи: `P<N>\coop\coop.log` (и `coop.old.log`, `crash.log`), сервер — `P1\coop\server-data\server.log`.

## Для Claude
- **Начало:** `git fetch origin && git switch four-players` — там всё. Прочитать этот файл, `CLAUDE.md`,
  [KNOWLEDGE.md](KNOWLEDGE.md) (разделы 9 и 10), задачи с пометкой «ждёт стенда» в [BACKLOG.md](BACKLOG.md).
- **Сборка на Windows:** `powershell -ExecutionPolicy Bypass -File tools/get-toolchain.ps1`, затем
  `python tools/build.py dist`. Проверки: `python tools/fsd_check.py`, `python tools/i18n_check.py`
  (в PowerShell перед ним `$env:PYTHONIOENCODING = "utf-8"`), `python -I tools/fork/guard_check.py`.
- **Бот:** `powershell -ExecutionPolicy Bypass -File tools/fork/bot-tests.ps1` (все сценарии ~90 с, из них
  `relay-refresh` ~70 с); `--only имя,имя`, `--list`. Сетевой баг — сначала сценарий в
  `src/bot/features/scenarios/Scenarios.cpp`, потом исправление.
- **Локально (на ПК владельца) можно то, что облако не может:** запускать стенд (`tools/fork/stand.ps1`), читать
  `coop.log` всех копий, смотреть окна. Ошибку из лога — с задачей из бэклога и проверкой на стенде.
- **В облаке:** игры нет; сборка под Linux (llvm-mingw `ucrt-ubuntu-22.04-x86_64` и обёртки `.exe`, см. `CLAUDE.md`),
  бот и сервер — под Wine (`wine32`).
- **Правила:** ветка на задачу, коммиты по-русски с номером задачи, PR в форк `Pomudop/sa-coop-friends` (не автору!),
  в описании — что сделано и как проверено (бот / сборка / стенд). Протокол меняется → `kForkProtocolRevision` +1.
  После проверки на стенде — в бэклоге заменить «ждёт стенда» на «проверено на стенде <дата>».
- **Слияние цепочки** — только с согласия владельца: по порядку #1 → #9, «Create a merge commit»; после каждого
  слияния удалить ветку слитой заявки — GitHub сам переключит следующую на `main`.

## Журнал сессий
Каждая сессия Claude, облачная или на ПК владельца, дописывает сюда строку **сразу после** заметного действия:
что сделала, где (ветка, коммит, PR) и что проверила. Так следующая сессия знает контекст, даже если предыдущая
оборвалась.

| Когда | Где | Что сделано | Проверка |
|---|---|---|---|
| 24.10 | ПК | Разбор проекта, разбор анализа Астры, ROADMAP / BACKLOG / GETTING_STARTED / KNOWLEDGE, CLAUDE.md | чтение кода |
| 25.10 | ПК | Форк `Pomudop/sa-coop-friends`, коммит `25a982f` с базой знаний и атрибуцией, Actions и Issues включены | — |
| 25.10 | облако | PR #1–#9: идентичность форка, бот, NET-01/02/04/05, STB-01/02, UX-01, четыре игрока, стенд, HANDOFF | бот под Wine, CI |
| 25.10 | ПК | INF-02: компилятор скачан, `build.py dist` и бот собраны на Windows; fsd, i18n, guard — ok | бот 17/17 на Windows |
| 25.10 | ПК | Claude Code на ПК: PATH, вход, `claude remote-control` в `D:\dev` | — |
| 25.10 | ПК | CONN-01: Radmin VPN и локальная сеть в F9, `IP:порт`, `stand.ps1 -HostAddress`, способ А в инструкции | бот через адрес Radmin |
| 25.10 | ПК | Игру для стенда не нашли: нужен путь к GTA SA 1.0 US | — |

## Дальше
0. Стенд на ПК владельца: нужен путь к GTA SA 1.0 US и сохранение (`stand.ps1 -Game ... -HostAddress 26.x.x.x` для Radmin).
1. Проверка на стенде по чек-листу выше и исправления по логам.
2. «Боты-игроки»: бот заходит в настоящую игру третьим и четвёртым игроком (ходит у хоста, стреляет) — проверка MP-01
   на двух копиях игры. Нужны форматы `G_PLAYER_ONFOOT`, `G_PLAYER_LOOK` из `src/client/entities/player/`.
3. Этап 3: MP-04 (у AmXLoVE есть ветка `pr-population-grid`, remote `amx`), MP-09, MP-11, MP-12, MP-05, MP-17.
4. Этап 1, P1: NET-06 … NET-09, STB-05, GAME-01. Этап 0: INF-02 (сборка на Windows = CI), INF-08 (бэклог в Issues).
