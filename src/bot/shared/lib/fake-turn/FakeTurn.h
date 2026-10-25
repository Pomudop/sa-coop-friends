#pragma once
// A TURN server (RFC 8656) that misbehaves on purpose, for the tests of the server's relay. It answers on 127.0.0.1
// from a thread of its own.
//  * RefuseLogin: every request gets error 401 with a realm and a nonce (NET-04: what the host is told);
//  * LoseRefresh: allocations and permissions are granted, the first Refresh goes unanswered (3 attempts of one
//    request), the next ones get 437, allocation mismatch (NET-05: the relay retries and takes a new address).
#include <winsock2.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace coop::bot {

class FakeTurn {
public:
    enum class Mode { RefuseLogin, LoseRefresh };
    static constexpr uint16_t kAllocate = 0x003, kRefresh = 0x004, kPermission = 0x008;

    ~FakeTurn() { Stop(); }
    bool Start(uint16_t port, Mode mode);
    void Stop();
    // every request so far: {method, NowMs() when it came}
    std::vector<std::pair<uint16_t, uint64_t>> Requests();

private:
    void Thread();

    SOCKET m_sock = INVALID_SOCKET;
    Mode m_mode = Mode::RefuseLogin;
    std::thread m_thread;
    std::atomic<bool> m_quit{false};
    std::mutex m_mutex;
    std::vector<std::pair<uint16_t, uint64_t>> m_requests;
};

}  // namespace coop::bot
