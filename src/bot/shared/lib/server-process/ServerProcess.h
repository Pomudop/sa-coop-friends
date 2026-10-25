#pragma once
// A real coop-server.exe for one test: its own port, its own data folder and server.ini (internet = 0 unless the test
// is about it: no UPnP, STUN or invite codes on a test machine), stopped when the test is over.
#include <windows.h>

#include <cstdint>
#include <string>

namespace coop::bot {

struct ServerOptions {
    std::string password;
    int maxPlayers = 2;
    std::string hostName;  // server.ini's "host": the role waits for this nickname
    bool fakeTurn = false;  // internet = 1 with a TURN relay that refuses the login (FakeTurn): turn below is set for it
    std::string turn;       // server.ini's "turn"
};

class ServerProcess {
public:
    ~ServerProcess() { Stop(); }

    // Starts exe with dir\server.ini and the data in dir\data (both made fresh), and waits until the server has opened
    // its port (its log says so). False if it did not: the log tail tells why.
    bool Start(const std::wstring& exe, const std::wstring& dir, uint16_t port, const ServerOptions& opt);
    void Stop();
    std::string LogTail(int lines) const;

private:
    PROCESS_INFORMATION m_proc{};
    std::wstring m_log;
};

}  // namespace coop::bot
