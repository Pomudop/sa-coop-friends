// coop-bot.exe — the server test bot of SA:COOP Friends (docs/fork/BACKLOG.md, INF-05).
//
//   coop-bot.exe [--server <coop-server.exe>] [--port N] [--work <dir>] [--only a,b] [--connect host:port] [--list]
//
// Starts a fresh coop-server.exe for each scenario (by default the one next to the bot), plays it with fake players and
// checks what the server does. Exit code: 0 when every scenario passed (known bugs failing as expected included),
// 1 when one failed, 2 on a wrong command line. No game is needed: this is how server and protocol changes are checked,
// locally and in CI.
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "common/config/version/Version.h"
#include "common/lib/util/Util.h"
#include "features/scenarios/Scenarios.h"

using namespace coop;
using namespace coop::bot;

static void Usage() {
    printf("coop-bot.exe [--server <coop-server.exe>] [--port N] [--work <папка>] [--only a,b] [--connect адрес:порт] "
           "[--list]\n");
}

int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IONBF, 0);  // CI shows the lines as they come, not after the run

    Settings s;
    s.serverExe = ModuleDir() + L"coop-server.exe";
    wchar_t tmp[MAX_PATH];
    s.workDir = std::wstring(tmp, GetTempPathW(MAX_PATH, tmp)) + L"coop-bot\\";
    std::vector<std::string> only;
    bool list = false;
    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        bool hasValue = i + 1 < argc;
        if (a == L"--list") list = true;
        else if (a == L"--server" && hasValue) s.serverExe = argv[++i];
        else if (a == L"--port" && hasValue) s.port = (uint16_t)_wtoi(argv[++i]);
        else if (a == L"--work" && hasValue) {
            s.workDir = argv[++i];
            if (!s.workDir.empty() && s.workDir.back() != L'\\') s.workDir += L'\\';
        } else if (a == L"--only" && hasValue) only = Split(WideToUtf8(argv[++i]), ',');
        else if (a == L"--connect" && hasValue) {
            std::string hp = WideToUtf8(argv[++i]);
            size_t colon = hp.rfind(':');
            s.connect = hp.substr(0, colon);
            s.connectPort = colon == std::string::npos ? kDefaultPort : (uint16_t)atoi(hp.c_str() + colon + 1);
        } else {
            Usage();
            return 2;
        }
    }

    auto& scenarios = ScenarioList();
    if (list) {
        for (auto& sc : scenarios)
            printf("%-30s %s%s%s\n", sc.name, sc.what, sc.knownBug ? ", известный баг " : "", sc.knownBug ? sc.knownBug : "");
        return 0;
    }
    for (auto& name : only) {
        bool known = false;
        for (auto& sc : scenarios) known |= name == sc.name;
        if (!known) {
            printf("Нет сценария %s (список: --list)\n", name.c_str());
            return 2;
        }
    }
    if (s.connect.empty() && !FileExists(s.serverExe)) {
        printf("Не найден сервер %s (укажите --server)\n", WideToUtf8(s.serverExe).c_str());
        return 2;
    }

    printf("%s %s, протокол %u: тесты сервера\n", COOP_NAME, COOP_VERSION_STR, kProtocolVersion);
    int passed = 0, failed = 0, known = 0, skipped = 0;
    for (size_t i = 0; i < scenarios.size(); ++i) {
        auto& sc = scenarios[i];
        if (!only.empty() && std::find(only.begin(), only.end(), sc.name) == only.end()) continue;
        uint64_t t0 = NowMs();
        Result res = RunScenario(i, s);
        const char* mark = "OK";
        switch (res.outcome) {
            case Outcome::Pass: ++passed; break;
            case Outcome::Fail: mark = "ОШИБКА"; ++failed; break;
            case Outcome::KnownBug: mark = "известный баг"; ++known; break;
            case Outcome::Fixed: mark = "ИСПРАВЛЕНО?"; ++failed; break;
            case Outcome::Skipped: mark = "пропущен"; ++skipped; break;
            case Outcome::NoServer: mark = "СЕРВЕР НЕ ЗАПУСТИЛСЯ"; ++failed; break;
        }
        printf("[%s] %s — %s%s%s (%.1f с)\n", mark, sc.name, sc.what, sc.knownBug ? ", " : "", sc.knownBug ? sc.knownBug : "",
               (NowMs() - t0) / 1000.0);
        for (auto& n : res.notes) printf("    %s\n", n.c_str());
        for (auto& f : res.failures) printf("    - %s\n", f.c_str());
        if (res.outcome == Outcome::Fixed)
            printf("    Сценарий прошёл: похоже, %s исправлен. Снимите пометку известного бага в Scenarios.cpp.\n",
                   sc.knownBug);
        if (!res.serverLog.empty()) printf("    server.log:\n%s\n", res.serverLog.c_str());
    }
    printf("Итого: прошли %d, ошибок %d, известных багов %d, пропущено %d\n", passed, failed, known, skipped);
    return failed ? 1 : 0;
}
