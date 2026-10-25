#include "shared/lib/server-process/ServerProcess.h"

#include <filesystem>
#include <vector>

#include "common/lib/util/Util.h"

namespace coop::bot {

bool ServerProcess::Start(const std::wstring& exe, const std::wstring& dir, uint16_t port, const ServerOptions& opt) {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);  // a previous run's profiles and world would change what the server does
    EnsureDirectory(dir);
    std::string ini = Format("[server]\r\nname = coop-bot test\r\nport = %u\r\npassword = %s\r\nmax_players = %d\r\n"
                             "host = %s\r\ninternet = %d\r\nturn = %s\r\n",
                             port, opt.password.c_str(), opt.maxPlayers, opt.hostName.c_str(), opt.turn.empty() ? 0 : 1,
                             opt.turn.c_str());
    WriteFileBytes(dir + L"server.ini", ini.data(), ini.size(), false);
    m_log = dir + L"data\\server.log";

    std::wstring cmd = L"\"" + exe + L"\" --config \"" + dir + L"server.ini\" --data \"" + dir + L"data\"";
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    // a console of its own without a window: the server reads commands from its console and would take the bot's
    if (!CreateProcessW(exe.c_str(), cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si,
                        &m_proc)) {
        m_proc = {};
        return false;
    }
    // the log line "... запущен на порту N" comes once the UDP port is open; a server that failed waits for Enter
    for (uint64_t end = NowMs() + 15000; NowMs() < end; Sleep(50)) {
        std::vector<uint8_t> log;
        if (ReadFileBytes(m_log, log) && std::string(log.begin(), log.end()).find(Format("на порту %u", port)) != std::string::npos)
            return true;
        if (WaitForSingleObject(m_proc.hProcess, 0) == WAIT_OBJECT_0) break;
    }
    return false;
}

void ServerProcess::Stop() {
    if (!m_proc.hProcess) return;
    TerminateProcess(m_proc.hProcess, 0);
    WaitForSingleObject(m_proc.hProcess, 5000);
    CloseHandle(m_proc.hProcess);
    CloseHandle(m_proc.hThread);
    m_proc = {};
}

std::string ServerProcess::LogTail(int lines) const {
    std::vector<uint8_t> log;
    if (!ReadFileBytes(m_log, log)) return "(server.log не найден)";
    std::string text(log.begin(), log.end());
    size_t pos = text.size();
    for (int n = 0; n <= lines && pos > 0; ++n) {
        size_t nl = text.rfind('\n', pos - 1);
        if (nl == std::string::npos) return text;
        pos = nl;
    }
    return text.substr(pos + 1);
}

}  // namespace coop::bot
