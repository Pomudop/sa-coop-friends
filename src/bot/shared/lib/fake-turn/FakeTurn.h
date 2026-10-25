#pragma once
// A TURN server that refuses every login (STUN error 401 with a realm and a nonce, RFC 8656), for the test of what the
// host is told when its relay does not work (NET-04). It answers on 127.0.0.1 from a thread of its own.
#include <winsock2.h>

#include <atomic>
#include <cstdint>
#include <thread>

namespace coop::bot {

class FakeTurn {
public:
    ~FakeTurn() { Stop(); }
    bool Start(uint16_t port);
    void Stop();
    int Requests() const { return m_requests; }  // the requests answered so far

private:
    void Thread();

    SOCKET m_sock = INVALID_SOCKET;
    std::thread m_thread;
    std::atomic<bool> m_quit{false};
    std::atomic<int> m_requests{0};
};

}  // namespace coop::bot
