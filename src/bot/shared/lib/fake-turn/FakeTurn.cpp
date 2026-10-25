#include "shared/lib/fake-turn/FakeTurn.h"

#include <cstring>
#include <vector>

#include "common/api/net/Net.h"
#include "common/lib/util/Util.h"

namespace coop::bot {

namespace {
constexpr uint8_t kMagic[4] = {0x21, 0x12, 0xA4, 0x42};

void Attr(std::vector<uint8_t>& m, uint16_t type, const void* v, size_t n) {
    uint8_t h[4] = {(uint8_t)(type >> 8), (uint8_t)type, (uint8_t)(n >> 8), (uint8_t)n};
    m.insert(m.end(), h, h + 4);
    m.insert(m.end(), (const uint8_t*)v, (const uint8_t*)v + n);
    m.resize(m.size() + (4 - n % 4) % 4, 0);
}
}  // namespace

bool FakeTurn::Start(uint16_t port, Mode mode) {
    m_mode = mode;
    net::Init();  // WSAStartup comes with enet_initialize
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (m_sock == INVALID_SOCKET || bind(m_sock, (sockaddr*)&a, sizeof(a)) != 0) return false;
    m_quit = false;
    m_thread = std::thread(&FakeTurn::Thread, this);
    return true;
}

void FakeTurn::Stop() {
    m_quit = true;
    if (m_thread.joinable()) m_thread.join();
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        net::Shutdown();
    }
}

std::vector<std::pair<uint16_t, uint64_t>> FakeTurn::Requests() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requests;
}

void FakeTurn::Thread() {
    int allocations = 0, refreshes = 0;
    while (!m_quit) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_sock, &fds);
        timeval tv{0, 50000};
        if (select(0, &fds, nullptr, nullptr, &tv) <= 0) continue;
        uint8_t buf[2048];
        sockaddr_in from{};
        int fromLen = sizeof(from);
        int n = recvfrom(m_sock, (char*)buf, sizeof(buf), 0, (sockaddr*)&from, &fromLen);
        // a request (class bits 00) with the magic cookie: answered (or not) as the mode says
        if (n < 20 || (buf[0] & 0xC0) || (buf[1] & 0x10) || (buf[0] & 0x01) || memcmp(buf + 4, kMagic, 4)) continue;
        uint16_t method = (uint16_t)(buf[0] << 8 | buf[1]);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_requests.push_back({method, NowMs()});
        }
        bool success = false;
        if (m_mode == Mode::LoseRefresh) {
            if (method == kRefresh && ++refreshes <= 3) continue;  // lost: the relay's request tries 3 times
            success = method != kRefresh;
        }
        uint16_t type = method | (success ? 0x0100 : 0x0110);
        std::vector<uint8_t> m(20, 0);
        m[0] = (uint8_t)(type >> 8), m[1] = (uint8_t)type;
        memcpy(m.data() + 4, buf + 4, 16);  // magic cookie and transaction id
        if (success && method == kAllocate) {
            // XOR-RELAYED-ADDRESS 127.0.0.1:(40000 + n): each allocation its own port; LIFETIME 120 s
            uint16_t port = (uint16_t)((40000 + ++allocations) ^ 0x2112);
            uint32_t ip = 0x7F000001u ^ 0x2112A442u;
            const uint8_t addr[8] = {0, 1, (uint8_t)(port >> 8), (uint8_t)port, (uint8_t)(ip >> 24), (uint8_t)(ip >> 16),
                                     (uint8_t)(ip >> 8), (uint8_t)ip};
            Attr(m, 0x0016, addr, 8);
            const uint8_t lifetime[4] = {0, 0, 0, 120};
            Attr(m, 0x000D, lifetime, 4);
        } else if (!success && m_mode == Mode::LoseRefresh) {
            const uint8_t error[] = {0, 0, 4, 37, 'A', 'l', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', ' ', 'M', 'i', 's', 'm',
                                     'a', 't', 'c', 'h'};  // 437
            Attr(m, 0x0009, error, sizeof(error));
        } else if (!success) {
            const uint8_t error[] = {0, 0, 4, 1, 'U', 'n', 'a', 'u', 't', 'h', 'o', 'r', 'i', 'z', 'e', 'd'};  // 401
            Attr(m, 0x0009, error, sizeof(error));
            Attr(m, 0x0014, "coop-bot", 8);  // REALM
            Attr(m, 0x0015, "nonce-1", 7);   // NONCE
        }
        m[2] = (uint8_t)((m.size() - 20) >> 8), m[3] = (uint8_t)(m.size() - 20);
        sendto(m_sock, (const char*)m.data(), (int)m.size(), 0, (sockaddr*)&from, fromLen);
    }
}

}  // namespace coop::bot
