#pragma once
#include <winsock2.h>

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "common/api/nat-traversal/NatTraversal.h"

namespace coop::server {

// TURN relay (RFC 8656 subset: Allocate, Refresh, CreatePermission, Send/Data indications) for friends
// behind any NAT: they send to the relayed address on the TURN server, which is always reachable.
// Each friend gets a local UDP socket that talks to the game port, so ENet sees an ordinary peer.
//
// spec: "metered:<app>:<api key>"                    credentials from a free metered.ca account
//       "turn:<host>:<port>:<user>:<password>"        any TURN server with static credentials
class TurnRelay {
public:
    enum class Status { Off, Starting, Ready, Failed };

    ~TurnRelay() { Stop(); }
    void Start(const std::string& spec, uint16_t gamePort);  // returns at once
    void Stop();
    void AllowPeer(uint32_t ipNet);  // the friend's public address (signalling): TURN only forwards known peers
    nat::Endpoint Relayed();          // valid once allocated
    // Failed comes with the reason in both languages, for the host: it was only in server.log (#64)
    Status GetStatus(std::string* why = nullptr, std::string* whyEn = nullptr);

private:
    struct Peer {
        SOCKET local = INVALID_SOCKET;  // 127.0.0.1:any <-> game port
        sockaddr_in addr{};             // the friend as seen by the TURN server
        uint64_t lastMs = 0;
        // datagrams each way since the last report, for the log: where a friend's connection lost the server
        uint32_t toCount = 0, fromCount = 0, toMax = 0;
        uint64_t toBytes = 0, fromBytes = 0, reportMs = 0;
    };

    void Thread();
    bool Credentials();
    void Fail(const std::string& why, const std::string& whyEn);
    bool Request(uint16_t method, const std::vector<std::vector<uint8_t>>& attrs, std::vector<uint8_t>* response);
    void SendIndication(const sockaddr_in& peer, const uint8_t* data, size_t len);
    void OnTurnPacket(const uint8_t* data, size_t len);
    void RefreshPermissions();

    std::string m_spec;
    uint16_t m_gamePort = 0;
    std::thread m_thread;
    std::atomic<bool> m_quit{false};
    SOCKET m_sock = INVALID_SOCKET;
    sockaddr_in m_server{};
    std::string m_host, m_user, m_password, m_realm, m_nonce;
    std::vector<uint8_t> m_key;  // MD5(user:realm:password)
    uint32_t m_lifetime = 600;
    int m_lastError = 0;  // of the last Request: the TURN error code, -1 when the server did not answer

    std::mutex m_mutex;
    Status m_status = Status::Off;
    std::string m_why, m_whyEn;
    nat::Endpoint m_relayed;
    std::set<uint32_t> m_allowed, m_permitted;
    std::map<uint64_t, Peer> m_peers;  // key: ip << 16 | port
};

}  // namespace coop::server
