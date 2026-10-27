#include "features/internet/Internet.h"

#include <random>

#include "common/lib/log/Log.h"
#include "common/lib/util/Util.h"

namespace coop::server {

namespace {
Internet* g_self = nullptr;
}

void Internet::Start(ENetHost* host, uint16_t port, const std::string& turnSpec, const std::wstring& dataDir) {
    m_dataDir = dataDir;
    m_host = host;
    m_port = port;
    m_turnEnabled = !turnSpec.empty();
    if (m_turnEnabled) m_turn.Start(turnSpec, port);
    g_self = this;
    host->intercept = &Internet::Intercept;
    m_quit = false;
    m_thread = std::thread(&Internet::Worker, this);
}

void Internet::Stop() {
    m_quit = true;
    m_signal.Stop();
    m_turn.Stop();
    if (m_thread.joinable()) m_thread.join();
    if (m_upnp.ok) {
        nat::UpnpUnmapPort(m_upnp, m_port);
        m_upnp.ok = false;
    }
    if (m_host) m_host->intercept = nullptr;
    g_self = nullptr;
}

std::string Internet::Code() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_code;
}

bool Internet::TakeNewCode(std::string& code) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_codeNew) return false;
    m_codeNew = false;
    code = m_code;
    return true;
}

bool Internet::TakeRelayFailure(std::string& why, std::string& whyEn) {
    if (!m_turnEnabled || m_turnFailureTaken || m_turn.GetStatus(&why, &whyEn) != TurnRelay::Status::Failed) return false;
    m_turnFailureTaken = true;
    return true;
}

// STUN answers and punch packets arrive on the game port: they never reach ENet's protocol code
int ENET_CALLBACK Internet::Intercept(ENetHost* host, ENetEvent*) {
    Internet* self = g_self;
    const uint8_t* data = host->receivedData;
    size_t len = host->receivedDataLength;
    if (!self) return 0;
    if (len >= sizeof(nat::kPunchMagic) - 1 && !memcmp(data, nat::kPunchMagic, sizeof(nat::kPunchMagic) - 1)) return 1;
    if (len == sizeof(nat::kDiscoverMagic) - 1 && !memcmp(data, nat::kDiscoverMagic, len)) {
        std::string reply = nat::kHereMagic + std::to_string(self->m_port);
        ENetBuffer buf{reply.size(), (void*)reply.data()};
        enet_socket_send(host->socket, &host->receivedAddress, &buf, 1);
        return 1;
    }
    if (!nat::IsStunPacket(data, len)) return 0;
    std::lock_guard<std::mutex> lock(self->m_mutex);
    nat::Endpoint mapped;
    if (self->m_stunWaiting && nat::ParseStunResponse(data, len, self->m_txid, mapped)) {
        self->m_mapped = mapped;
        self->m_stunWaiting = false;
    }
    return 1;
}

void Internet::Worker() {
    // 1. the router forwards the port (works for most home routers; useless behind carrier NAT)
    m_upnp = nat::UpnpMapPort(m_port, "SA:COOP server");
    if (m_upnp.ok) LOGI("UPnP: порт %u проброшен. Внешний адрес роутера: %s", m_port, m_upnp.externalIp.c_str());
    else LOGI("UPnP недоступен (%s). Будем пробивать NAT", m_upnp.error.c_str());

    // 2. the address the internet sees for our socket
    for (const std::string& server : nat::StunServers()) {
        if (m_quit) return;
        size_t colon = server.rfind(':');
        ENetAddress addr{};
        if (enet_address_set_host(&addr, server.substr(0, colon).c_str()) != 0) continue;
        addr.port = (enet_uint16)atoi(server.c_str() + colon + 1);
        uint8_t req[20];
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            nat::BuildStunRequest(req, m_txid);
            m_stunWaiting = true;
        }
        ENetBuffer buf{sizeof(req), req};
        enet_socket_send(m_host->socket, &addr, &buf, 1);
        for (int i = 0; i < 20 && !m_quit; ++i) {
            Sleep(100);
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_stunWaiting) break;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_mapped.Valid()) break;
        m_stunWaiting = false;
    }

    nat::Endpoint ep;
    nat::Endpoint upnpEp;
    if (m_upnp.ok && nat::Endpoint::Parse(m_upnp.externalIp + ":" + std::to_string(m_port), upnpEp) &&
        !nat::IsPrivateIp(upnpEp.ip)) {
        ep = upnpEp;  // directly reachable
    } else {
        std::lock_guard<std::mutex> lock(m_mutex);
        ep = m_mapped;
    }
    // the relay address goes to the signalling channel (or into the code when nothing else works). Only then is it
    // waited for, and not past its failure: the code came 15 s late every time, also when the relay had failed (#64)
    nat::Endpoint relay;
    auto relayPending = [&] {
        return m_turnEnabled && !(relay = m_turn.Relayed()).Valid() && m_turn.GetStatus() == TurnRelay::Status::Starting;
    };
    for (int i = 0; i < 150 && !ep.Valid() && !m_quit && relayPending(); ++i) Sleep(100);
    relay = m_turn.Relayed();
    if (!ep.Valid()) ep = relay;
    if (!ep.Valid()) {
        LOGW("Внешний адрес не определён (нет интернета?). Подключиться можно только по локальной сети или по IP");
        return;
    }
    // NET-07: the key is kept in server-data: a restart with the same external address gives the same code, and a friend's
    // code from yesterday still works. It was random at every start.
    uint8_t key = 0;
    std::vector<uint8_t> saved;
    std::wstring keyPath = m_dataDir.empty() ? L"" : m_dataDir + L"invite-key.txt";
    if (!keyPath.empty() && ReadFileBytes(keyPath, saved) && !saved.empty()) {
        key = (uint8_t)atoi(std::string(saved.begin(), saved.end()).c_str());
    } else {
        std::random_device rd;
        key = (uint8_t)rd();
        std::string text = std::to_string(key);
        if (!keyPath.empty()) WriteFileBytes(keyPath, text.data(), text.size());
    }
    std::string code = nat::MakeInviteCode(ep, key);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_code = code;
        m_codeNew = true;
    }
    LOGI("Код приглашения: %s (внешний адрес %s)", code.c_str(), ep.ToString().c_str());

    // 3. joiners behind NAT announce their endpoint; we answer with packets that open our NAT for them
    m_signal.Start(nat::kDefaultSignalUrl, nat::SignalTopic(code), [this](const std::string& msg) { QueuePunch(msg); });

    // 4. the relay's address once it has one, and again when the relay had to take a new one (a joiner reads the
    // channel's history when it connects, the latest RELAY wins), and a finite UPnP lease renewed while the server runs
    nat::Endpoint announced;
    for (uint64_t renewAt = NowMs() + (uint64_t)m_upnp.leaseSeconds * 500; !m_quit;) {
        relay = m_turn.Relayed();
        if (relay.Valid() && (relay.ip != announced.ip || relay.port != announced.port)) {
            if (announced.Valid() && ep.ip == announced.ip && ep.port == announced.port)
                LOGW("Код приглашения указывает на старый адрес ретранслятора. Перезапустите сервер, чтобы получить новый код");
            announced = relay;
            if (relay.ip != ep.ip) {
                nat::SignalPublish(nat::kDefaultSignalUrl, nat::SignalTopic(code), "RELAY " + relay.ToString());
                LOGI("Ретранслятор для друзей за любым NAT: %s", relay.ToString().c_str());
            }
        }
        bool renew = m_upnp.ok && m_upnp.leaseSeconds > 0;
        if (!m_turnEnabled && !renew) break;
        Sleep(500);
        if (!renew || NowMs() < renewAt) continue;
        nat::UpnpResult again = nat::UpnpMapPort(m_port, "SA:COOP server");
        if (again.ok) m_upnp = again;
        renewAt = NowMs() + (uint64_t)m_upnp.leaseSeconds * 500;
    }
}

void Internet::QueuePunch(const std::string& message) {
    // "PUNCH 1.2.3.4:5678"
    if (message.rfind("PUNCH ", 0) != 0) return;
    nat::Endpoint ep;
    if (!nat::Endpoint::Parse(message.substr(6), ep)) return;
    ENetAddress addr{};
    addr.host = ep.ip;
    addr.port = ep.port;
    LOGI("Пробивка NAT к %s", ep.ToString().c_str());
    if (m_turnEnabled) m_turn.AllowPeer(ep.ip);  // TURN forwards only peers we named
    std::lock_guard<std::mutex> lock(m_mutex);
    m_punches.push_back({addr, 10, 0});
}

void Internet::Service() {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t now = NowMs();
    for (auto it = m_punches.begin(); it != m_punches.end();) {
        if (now >= it->nextMs) {
            ENetBuffer buf{sizeof(nat::kPunchMagic) - 1, (void*)nat::kPunchMagic};
            enet_socket_send(m_host->socket, &it->addr, &buf, 1);
            it->nextMs = now + 200;
            if (--it->left <= 0) {
                it = m_punches.erase(it);
                continue;
            }
        }
        ++it;
    }
}

}  // namespace coop::server
