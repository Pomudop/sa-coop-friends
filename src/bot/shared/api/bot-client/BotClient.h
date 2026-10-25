#pragma once
// One fake player for the server tests (INF-05): an ENet client that greets the server as the game does (proto::Hello)
// and records everything that comes back. It lives on the caller's thread and does nothing between Poll() calls: that
// is how a test makes a player go silent, as a game that hangs or loses its line.
#include <enet/enet.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "common/api/protocol/Protocol.h"
#include "common/lib/buffer/Buffer.h"

namespace coop::bot {

// A game message relayed by the server: [id][sender][payload]
struct Relayed {
    uint8_t id = 0;
    uint8_t from = 0;
    uint8_t channel = 0;
    std::vector<uint8_t> payload;
    uint64_t at = 0;  // NowMs() when it arrived
};

struct Chat {
    uint8_t from = 0;
    std::string text, en;
};

class BotClient {
public:
    explicit BotClient(std::string name);
    ~BotClient();
    BotClient(const BotClient&) = delete;
    BotClient& operator=(const BotClient&) = delete;

    // What Connect greets with: the fork's protocol and version, the bot's name. A test changes it before Connect.
    proto::Hello hello;

    // Starts connecting; the hello goes out from Poll once ENet has connected (as Session does in the game).
    bool Connect(const std::string& host, uint16_t port);
    // Services ENet for up to ms milliseconds (0: only what is pending).
    void Poll(uint32_t ms = 0);
    void Send(uint8_t channel, const Writer& w);
    void SetStatus(uint8_t status);
    // Leaves as the game does: data = 1 tells the server it was a quit, not a timeout.
    void Quit();
    // The game's process is gone: the server hears nothing more and times the connection out.
    void Crash();

    const std::string& Name() const { return m_name; }
    bool Connected() const { return m_connected; }
    bool Welcomed() const { return m_welcomed; }
    bool Rejected() const { return m_rejected; }
    bool Kicked() const { return m_kicked; }
    bool Disconnected() const { return m_disconnected; }
    const proto::Welcome& Welcome() const { return m_welcome; }
    uint8_t Id() const { return m_welcome.yourId; }
    const std::string& RejectReason() const { return m_rejectReason; }
    const std::vector<proto::PlayerEntry>& Joins() const { return m_joins; }
    const std::vector<std::pair<uint8_t, uint8_t>>& Leaves() const { return m_leaves; }  // {id, LeaveReason}
    const std::vector<uint8_t>& HostChanges() const { return m_hostChanges; }
    const std::vector<Chat>& Chats() const { return m_chats; }
    const std::vector<Relayed>& Received() const { return m_relayed; }
    bool LeftId(uint8_t id) const;

private:
    void OnPacket(uint8_t channel, const uint8_t* data, size_t size);

    std::string m_name;
    ENetHost* m_host = nullptr;
    ENetPeer* m_peer = nullptr;
    bool m_connected = false, m_welcomed = false, m_rejected = false, m_kicked = false, m_disconnected = false;
    proto::Welcome m_welcome;
    std::string m_rejectReason;
    std::vector<proto::PlayerEntry> m_joins;
    std::vector<std::pair<uint8_t, uint8_t>> m_leaves;
    std::vector<uint8_t> m_hostChanges;
    std::vector<Chat> m_chats;
    std::vector<Relayed> m_relayed;
};

}  // namespace coop::bot
