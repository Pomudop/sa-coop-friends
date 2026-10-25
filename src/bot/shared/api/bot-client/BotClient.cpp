#include "shared/api/bot-client/BotClient.h"

#include <algorithm>

#include "common/api/net/Net.h"
#include "common/config/version/Version.h"
#include "common/lib/util/Util.h"

namespace coop::bot {

using namespace proto;

BotClient::BotClient(std::string name) : m_name(std::move(name)) {
    net::Init();
    hello.protocol = kProtocolVersion;
    hello.version = COOP_VERSION_STR;
    hello.name = m_name;
    hello.exeSize = kSupportedExeSize;
}

BotClient::~BotClient() {
    if (m_peer) enet_peer_reset(m_peer);
    if (m_host) enet_host_destroy(m_host);
    net::Shutdown();
}

bool BotClient::Connect(const std::string& host, uint16_t port) {
    // the same host setup as the game's NetClient: the server compresses with the range coder, so must we
    m_host = enet_host_create(nullptr, 1, CH_COUNT, 0, 0);
    if (!m_host) return false;
    enet_host_compress_with_range_coder(m_host);
    m_host->mtu = net::kMtu;
    ENetAddress addr{};
    if (enet_address_set_host(&addr, host.c_str()) != 0) return false;
    addr.port = port;
    m_peer = enet_host_connect(m_host, &addr, CH_COUNT, 0);
    if (!m_peer) return false;
    enet_peer_timeout(m_peer, 0, 10000, 20000);
    return true;
}

void BotClient::Poll(uint32_t ms) {
    if (!m_host) return;
    ENetEvent ev;
    int rc = enet_host_service(m_host, &ev, ms);
    while (rc > 0) {
        switch (ev.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                m_connected = true;
                Writer w(C_HELLO);
                hello.Write(w);
                net::Send(m_peer, CH_RELIABLE, w);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                m_disconnected = true;
                m_peer = nullptr;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                OnPacket(ev.channelID, ev.packet->data, ev.packet->dataLength);
                enet_packet_destroy(ev.packet);
                break;
            default:
                break;
        }
        rc = enet_host_check_events(m_host, &ev);
    }
}

void BotClient::OnPacket(uint8_t channel, const uint8_t* data, size_t size) {
    if (size == 0) return;
    uint8_t id = data[0];
    if (IsRelayed(id)) {
        if (size < 2) return;
        m_relayed.push_back({id, data[1], channel, std::vector<uint8_t>(data + 2, data + size), NowMs()});
        return;
    }
    Reader r(data + 1, size - 1);
    switch (id) {
        case S_WELCOME:
            m_welcomed = m_welcome.Read(r);
            break;
        case S_REJECT:
            m_rejected = true;
            m_rejectReason = r.Str();
            break;
        case S_KICKED:
            m_kicked = true;
            m_rejectReason = r.Str();
            break;
        case S_PLAYER_JOIN: {
            PlayerEntry p;
            p.id = r.U8();
            p.name = r.Str(64);
            p.isHost = r.Bool();
            if (r.Ok()) m_joins.push_back(p);
            break;
        }
        case S_PLAYER_LEAVE: {
            uint8_t who = r.U8(), why = r.U8();
            if (r.Ok()) m_leaves.push_back({who, why});
            break;
        }
        case S_HOST_CHANGED: {
            uint8_t who = r.U8();
            if (r.Ok()) m_hostChanges.push_back(who);
            break;
        }
        case S_CHAT: {
            Chat c;
            c.from = r.U8();
            r.U32();  // colour
            c.text = r.Str();
            c.en = r.Str();
            if (r.Ok()) m_chats.push_back(c);
            break;
        }
        default:  // pings, profile, invite, world: not what the tests look at
            break;
    }
}

void BotClient::Send(uint8_t channel, const Writer& w) {
    net::Send(m_peer, channel, w);
}

void BotClient::SetStatus(uint8_t status) {
    Writer w(C_STATUS);
    w.U8(status);
    Send(CH_RELIABLE, w);
}

void BotClient::Quit() {
    if (!m_peer) return;
    enet_peer_disconnect_later(m_peer, 1);
    for (uint64_t end = NowMs() + 1000; m_peer && NowMs() < end;) Poll(10);
}

void BotClient::Crash() {
    // enet_peer_reset tells the server nothing: to it the player falls silent, as after a crash of the game
    if (m_peer) enet_peer_reset(m_peer);
    m_peer = nullptr;
    if (m_host) enet_host_destroy(m_host);
    m_host = nullptr;
}

bool BotClient::LeftId(uint8_t id) const {
    return std::any_of(m_leaves.begin(), m_leaves.end(), [&](auto& l) { return l.first == id; });
}

}  // namespace coop::bot
