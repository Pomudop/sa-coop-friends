#include "features/scenarios/Scenarios.h"

#include <windows.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <set>

#include "common/api/protocol/Protocol.h"
#include "common/config/version/Version.h"
#include "common/lib/util/Util.h"
#include "shared/api/bot-client/BotClient.h"
#include "shared/lib/fake-turn/FakeTurn.h"
#include "shared/lib/server-process/ServerProcess.h"

namespace coop::bot {

using namespace proto;

namespace {

// One scenario's run: its server, its players and what went wrong. Every player is polled together, as each game polls
// on its own; a paused one is not, and falls silent for the server.
class Run {
public:
    Run(std::string host, uint16_t port) : m_host(std::move(host)), m_port(port) {}

    BotClient& Bot(const std::string& name) {
        m_bots.push_back(std::make_unique<BotClient>(name));
        return *m_bots.back();
    }
    // Connects and waits for the server's answer; true when it welcomed the player.
    bool Join(BotClient& b) {
        if (!b.Connect(m_host, m_port)) return false;
        WaitFor([&] { return b.Welcomed() || b.Rejected() || b.Disconnected(); }, 5000);
        return b.Welcomed();
    }
    void Pump(uint32_t ms) {
        uint64_t end = NowMs() + ms;
        do {
            for (auto& b : m_bots)
                if (!m_paused.count(b.get())) b->Poll(0);
            Sleep(1);
        } while (NowMs() < end);
    }
    bool WaitFor(const std::function<bool()>& done, uint32_t ms) {
        for (uint64_t end = NowMs() + ms; !done();) {
            if (NowMs() >= end) return false;
            Pump(5);
        }
        return true;
    }
    void Pause(BotClient& b) { m_paused.insert(&b); }
    void Resume(BotClient& b) { m_paused.erase(&b); }

    bool Check(bool ok, const std::string& what) {
        if (!ok) failures.push_back(what);
        return ok;
    }
    bool JoinOrFail(BotClient& b) {
        return Check(Join(b), Format("%s не вошёл%s%s", b.Name().c_str(), b.Rejected() ? ": " : " (нет ответа)",
                                     b.RejectReason().c_str()));
    }

    std::vector<std::string> notes, failures;

private:
    std::string m_host;
    uint16_t m_port;
    std::vector<std::unique_ptr<BotClient>> m_bots;
    std::set<BotClient*> m_paused;
};

bool Contains(const std::string& s, const char* part) {
    return s.find(part) != std::string::npos;
}

// a state of the size of the game's on-foot one (~77 bytes): the number and the send time, then a known pattern
Writer State(uint32_t seq) {
    Writer w(G_PLAYER_ONFOOT);
    w.U32(seq);
    w.U64(NowMs());
    for (int i = 0; i < 64; ++i) w.U8((uint8_t)(seq + i));
    return w;
}

size_t CountFrom(const BotClient& b, uint8_t id, uint8_t from) {
    return std::count_if(b.Received().begin(), b.Received().end(), [&](auto& m) { return m.id == id && m.from == from; });
}

// ---- scenarios ----

void JoinLeave(Run& r) {
    auto& a = r.Bot("Alpha");
    if (!r.JoinOrFail(a)) return;
    r.Check(a.Welcome().hostId == a.Id(), "первый вошедший не стал хостом");
    auto& b = r.Bot("Bravo");
    if (!r.JoinOrFail(b)) return;
    auto& others = b.Welcome().players;
    r.Check(others.size() == 1 && others[0].name == "Alpha" && others[0].isHost,
            "в приветствии второго игрока нет хоста Alpha");
    r.Check(r.WaitFor([&] { return !a.Joins().empty(); }, 2000) && a.Joins()[0].name == "Bravo",
            "Alpha не узнал о входе Bravo");
    uint8_t bid = b.Id();
    b.Quit();
    if (r.Check(r.WaitFor([&] { return a.LeftId(bid); }, 3000), "Alpha не узнал о выходе Bravo"))
        r.Check(a.Leaves().back().second == LEAVE_QUIT, "выход Bravo засчитан не как выход, а как обрыв");
}

void ChatRelay(Run& r) {
    auto& a = r.Bot("Alpha");
    auto& b = r.Bot("Bravo");
    if (!r.JoinOrFail(a) || !r.JoinOrFail(b)) return;
    const std::string text = "привет из бота, SA:COOP Friends";
    Writer w(C_CHAT);
    w.Str(text, kMaxChatLen * 4);
    a.Send(CH_RELIABLE, w);
    auto got = [&] {
        return std::any_of(b.Chats().begin(), b.Chats().end(), [&](auto& c) { return c.from == a.Id() && c.text == text; });
    };
    r.Check(r.WaitFor(got, 2000), "сообщение чата Alpha не дошло до Bravo");
    Writer cmd(C_CHAT);
    cmd.Str("/players");
    a.Send(CH_RELIABLE, cmd);
    auto listed = [&] {
        return std::any_of(a.Chats().begin(), a.Chats().end(),
                           [&](auto& c) { return c.from == kServerId && Contains(c.text, "Bravo"); });
    };
    r.Check(r.WaitFor(listed, 2000), "команда /players не показала Bravo");
}

void StateRelay(Run& r) {
    auto& a = r.Bot("Alpha");
    auto& b = r.Bot("Bravo");
    if (!r.JoinOrFail(a) || !r.JoinOrFail(b)) return;
    a.SetStatus(STATUS_INGAME);
    r.Pump(200);
    // a player in the menu or loading gets no states: they would pile up and come stale all at once (Server::Relay)
    for (uint32_t i = 0; i < 10; ++i) {
        a.Send(CH_STATE, State(i));
        r.Pump(33);
    }
    r.Pump(300);
    r.Check(CountFrom(b, G_PLAYER_ONFOOT, a.Id()) == 0, "состояния дошли до игрока в меню");

    b.SetStatus(STATUS_INGAME);
    r.Pump(300);
    const uint32_t kFirst = 1000, kCount = 90;  // 3 s at the game's 30 states a second
    for (uint32_t i = 0; i < kCount; ++i) {
        a.Send(CH_STATE, State(kFirst + i));
        r.Pump(33);
    }
    r.Pump(500);
    std::set<uint32_t> seen;
    uint64_t sum = 0, worst = 0;
    bool intact = true;
    for (auto& m : b.Received()) {
        if (m.id != G_PLAYER_ONFOOT || m.from != a.Id()) continue;
        Reader rd(m.payload.data(), m.payload.size());
        uint32_t seq = rd.U32();
        uint64_t sent = rd.U64();
        for (int i = 0; i < 64; ++i) intact &= rd.U8() == (uint8_t)(seq + i);
        intact &= rd.Ok() && rd.Remaining() == 0 && m.channel == CH_STATE;
        if (seq < kFirst || !seen.insert(seq).second) continue;
        uint64_t lag = m.at - sent;
        sum += lag;
        worst = std::max(worst, lag);
    }
    r.Check(intact, "состояние пришло искажённым или не по тому каналу");
    double loss = 100.0 * (kCount - seen.size()) / kCount;
    r.notes.push_back(Format("состояний 30/с: дошло %u из %u (потери %.1f%%), задержка средняя %.1f мс, худшая %u мс",
                             (unsigned)seen.size(), kCount, loss, seen.empty() ? 0.0 : (double)sum / seen.size(),
                             (unsigned)worst));
    r.Check(loss <= 5.0, Format("потеряно %.1f%% состояний на локальной машине", loss));
    r.Check(CountFrom(a, G_PLAYER_ONFOOT, a.Id()) == 0, "сервер вернул отправителю его же состояния");
}

void ReliableRelay(Run& r) {
    auto& a = r.Bot("Alpha");
    auto& b = r.Bot("Bravo");
    if (!r.JoinOrFail(a) || !r.JoinOrFail(b)) return;
    const uint32_t kCount = 200;
    for (uint32_t i = 0; i < kCount; ++i) {
        Writer w(G_PLAYER_ANIM);
        w.U32(i);
        a.Send(CH_RELIABLE, w);
        if (i % 20 == 0) r.Pump(1);
    }
    r.WaitFor([&] { return CountFrom(b, G_PLAYER_ANIM, a.Id()) >= kCount; }, 5000);
    std::vector<uint32_t> order;
    for (auto& m : b.Received())
        if (m.id == G_PLAYER_ANIM && m.from == a.Id()) {
            Reader rd(m.payload.data(), m.payload.size());
            order.push_back(rd.U32());
        }
    bool inOrder = order.size() == kCount;
    for (uint32_t i = 0; inOrder && i < kCount; ++i) inOrder = order[i] == i;
    r.Check(inOrder, Format("надёжных событий дошло %u из %u или не по порядку", (unsigned)order.size(), kCount));
}

void FullServer(Run& r) {
    auto& a = r.Bot("Alpha");
    auto& b = r.Bot("Bravo");
    if (!r.JoinOrFail(a) || !r.JoinOrFail(b)) return;
    auto& c = r.Bot("Charlie");
    r.Check(!r.Join(c) && c.Rejected() && Contains(c.RejectReason(), "нет свободных мест"),
            "третий игрок на сервере на двоих не получил «нет свободных мест»");
}

void Password(Run& r) {
    auto& wrong = r.Bot("Wrong");
    wrong.hello.password = "nope";
    r.Check(!r.Join(wrong) && Contains(wrong.RejectReason(), "неверный пароль"), "неверный пароль не отклонён");
    auto& none = r.Bot("NoPass");
    r.Check(!r.Join(none) && Contains(none.RejectReason(), "неверный пароль"), "вход без пароля не отклонён");
    auto& right = r.Bot("Right");
    right.hello.password = "secret";
    r.JoinOrFail(right);
}

void Versions(Run& r) {
    auto& original = r.Bot("Original");
    original.hello.protocol = kUpstreamProtocol;
    r.Check(!r.Join(original) && Contains(original.RejectReason(), "оригинальный SA:COOP"),
            "клиент оригинального SA:COOP не получил отказ с подсказкой поставить SA:COOP Friends");
    auto& newer = r.Bot("Newer");
    newer.hello.protocol = kProtocolVersion + 1;
    r.Check(!r.Join(newer) && Contains(newer.RejectReason(), "другая версия мода"),
            "клиент другой версии форка не получил «другая версия мода»");
}

void Nicknames(Run& r) {
    auto& shortName = r.Bot("x");
    r.Check(!r.Join(shortName) && shortName.Rejected(), "ник из одного символа принят");
    auto& device = r.Bot("con");
    r.Check(!r.Join(device) && Contains(device.RejectReason(), "Windows"), "ник CON (устройство Windows) принят");
    auto& a = r.Bot("Alpha");
    if (!r.JoinOrFail(a)) return;
    auto& twin = r.Bot("alpha");
    r.Check(!r.Join(twin) && Contains(twin.RejectReason(), "уже на сервере"),
            "второй игрок с тем же ником (другим регистром) принят");
}

void CrashReturn(Run& r) {
    auto& a = r.Bot("Alpha");
    auto& b = r.Bot("Bravo");
    if (!r.JoinOrFail(a) || !r.JoinOrFail(b)) return;
    uint8_t oldId = a.Id();
    a.Crash();
    // at once: the old connection still answered a moment ago, so it is a second player with this nickname
    auto& early = r.Bot("Alpha");
    r.Check(!r.Join(early) && Contains(early.RejectReason(), "уже на сервере"),
            "вернувшийся сразу после вылета вытеснил ещё живое соединение");
    // after 3 s of silence the old connection gives way (Server::HandleHello)
    r.Pump(3500);
    auto& back = r.Bot("Alpha");
    if (!r.Check(r.Join(back), "вернувшийся через 3,5 с после вылета не вошёл: " + back.RejectReason())) return;
    r.Check(r.WaitFor([&] { return b.LeftId(oldId); }, 2000), "Bravo не узнал, что старое соединение Alpha закрыто");
    auto left = std::find_if(b.Leaves().begin(), b.Leaves().end(), [&](auto& l) { return l.first == oldId; });
    r.Check(left == b.Leaves().end() || left->second == LEAVE_TIMEOUT, "вылет засчитан как обычный выход");
    auto rejoined = [&] {
        return std::any_of(b.Joins().begin(), b.Joins().end(), [&](auto& j) { return j.id == back.Id() && j.name == "Alpha"; });
    };
    r.Check(r.WaitFor(rejoined, 2000), "Bravo не узнал о возвращении Alpha");
}

void HostChange(Run& r) {
    auto& a = r.Bot("Alpha");
    auto& b = r.Bot("Bravo");
    if (!r.JoinOrFail(a) || !r.JoinOrFail(b)) return;
    a.Quit();
    auto moved = [&] { return std::find(b.HostChanges().begin(), b.HostChanges().end(), b.Id()) != b.HostChanges().end(); };
    r.Check(r.WaitFor(moved, 3000), "после выхода хоста роль хоста не перешла к оставшемуся");
}

void ReservedHost(Run& r) {
    auto& a = r.Bot("Alpha");
    if (!r.JoinOrFail(a)) return;
    r.Check(a.Welcome().hostId == 255, "роль хоста досталась не тому, кто указан в host =");
    auto& boss = r.Bot("Boss");
    if (!r.JoinOrFail(boss)) return;
    r.Check(boss.Welcome().hostId == boss.Id(), "игрок из host = не стал хостом");
    auto announced = [&] {
        return std::any_of(a.Joins().begin(), a.Joins().end(), [&](auto& j) { return j.name == "Boss" && j.isHost; });
    };
    r.Check(r.WaitFor(announced, 2000), "Alpha не узнал, что хост вошёл");
}

// NET-01: every check of a hello comes before the old connection of its nickname is let go
void PasswordBeforeReplace(Run& r) {
    auto& a = r.Bot("Alpha");
    a.hello.password = "secret";
    auto& b = r.Bot("Bravo");
    b.hello.password = "secret";
    if (!r.JoinOrFail(a) || !r.JoinOrFail(b)) return;
    // a live player's nickname with a wrong password: told the password is wrong, not that the nickname is here
    auto& guess = r.Bot("Alpha");
    guess.hello.password = "wrong";
    r.Check(!r.Join(guess) && Contains(guess.RejectReason(), "неверный пароль"),
            "подключение с ником живого игрока и неверным паролем получило не «неверный пароль»");
    uint8_t oldId = a.Id();
    r.Pause(a);  // Alpha's line lags: the server hears nothing from it for a while
    r.Pump(3500);
    auto& intruder = r.Bot("Alpha");
    intruder.hello.password = "wrong";
    r.Check(!r.Join(intruder) && Contains(intruder.RejectReason(), "неверный пароль"), "чужой с неверным паролем вошёл");
    r.Pump(1000);
    r.Check(!b.LeftId(oldId), "подключение с ником Alpha и неверным паролем выкинуло настоящего Alpha");
    // the real Alpha, its game restarted, with the right password: the hung connection gives way
    auto& back = r.Bot("Alpha");
    back.hello.password = "secret";
    if (r.Check(r.Join(back), "Alpha с верным паролем не заменил своё зависшее соединение: " + back.RejectReason()))
        r.Check(r.WaitFor([&] { return b.LeftId(oldId); }, 2000), "Bravo не узнал, что старое соединение Alpha закрыто");
}

// NET-04: a relay that does not work is told to the host with the reason, not only written to server.log
void RelayFailure(Run& r) {
    auto& host = r.Bot("Alpha");
    if (!r.JoinOrFail(host)) return;
    auto told = [&] {
        return std::any_of(host.Chats().begin(), host.Chats().end(), [&](auto& c) {
            return c.from == kServerId && Contains(c.text, "Ретранслятор не работает") && Contains(c.text, "логин");
        });
    };
    uint64_t t0 = NowMs();
    if (r.Check(r.WaitFor(told, 5000), "хост не узнал, что ретранслятор отклонил логин"))
        r.notes.push_back(Format("причина сбоя ретранслятора пришла хосту через %u мс после входа", (unsigned)(NowMs() - t0)));
}

// NET-02: a hello without a version is refused
void ProtocolZero(Run& r) {
    auto& z = r.Bot("Zero");
    z.hello.protocol = 0;
    r.Check(!r.Join(z) && z.Rejected(), "приветствие с протоколом 0 принято");
}

struct Entry {
    ScenarioInfo info;
    ServerOptions options;
    void (*run)(Run&);
};

const std::vector<Entry>& Entries() {
    static const std::vector<Entry> list = {
        {{"join-leave", "вход, хост, список игроков, выход", nullptr, true}, {}, JoinLeave},
        {{"chat", "чат и команда /players", nullptr, false}, {}, ChatRelay},
        {{"state-relay", "состояния 30/с: потери, задержка, не игроку в меню", nullptr, false}, {}, StateRelay},
        {{"reliable-relay", "200 надёжных событий: все и по порядку", nullptr, false}, {}, ReliableRelay},
        {{"full-server", "третий на сервере на двоих: «нет мест»", nullptr, true}, {}, FullServer},
        {{"password", "неверный пароль, без пароля, верный", nullptr, true}, {"secret"}, Password},
        {{"versions", "оригинальный SA:COOP и другая версия форка", nullptr, false}, {}, Versions},
        {{"nicknames", "короткий ник, CON, тот же ник", nullptr, true}, {}, Nicknames},
        {{"crash-return", "вылет и возвращение под тем же ником", nullptr, true}, {}, CrashReturn},
        {{"host-change", "хост вышел: роль переходит оставшемуся", nullptr, true}, {}, HostChange},
        {{"reserved-host", "host = в server.ini: роль ждёт этот ник", nullptr, true}, {"", 2, "Boss"}, ReservedHost},
        {{"password-before-replace", "NET-01: неверный пароль с ником зависшего игрока, верный — заменяет", nullptr, true},
         {"secret"}, PasswordBeforeReplace},
        {{"protocol-zero", "NET-02: приветствие без версии", nullptr, true}, {}, ProtocolZero},
        {{"relay-failure", "NET-04: ретранслятор отклонил логин — хост видит причину", nullptr, true},
         {"", 2, "", true}, RelayFailure},
    };
    return list;
}

}  // namespace

const std::vector<ScenarioInfo>& ScenarioList() {
    static const std::vector<ScenarioInfo> infos = [] {
        std::vector<ScenarioInfo> v;
        for (auto& e : Entries()) v.push_back(e.info);
        return v;
    }();
    return infos;
}

Result RunScenario(size_t index, const Settings& s) {
    const Entry& e = Entries().at(index);
    Result res;
    bool external = !s.connect.empty();
    if (external && e.info.ownServer) {
        res.outcome = Outcome::Skipped;
        return res;
    }
    ServerProcess server;
    FakeTurn turn;
    uint16_t port = external ? s.connectPort : (uint16_t)(s.port + index);
    ServerOptions options = e.options;
    if (options.fakeTurn) {
        // listening before the server starts: its relay asks for an address at once
        uint16_t turnPort = (uint16_t)(port + 500);
        if (!turn.Start(turnPort)) {
            res.outcome = Outcome::Fail;
            res.failures.push_back(Format("не удалось открыть порт %u для поддельного TURN", turnPort));
            return res;
        }
        options.turn = Format("turn:127.0.0.1:%u:coop-bot:wrong", turnPort);
    }
    if (!external && !server.Start(s.serverExe, s.workDir + Utf8ToWide(e.info.name) + L"\\", port, options)) {
        res.outcome = Outcome::NoServer;
        res.serverLog = server.LogTail(15);
        return res;
    }
    Run r(external ? s.connect : "127.0.0.1", port);
    e.run(r);
    res.notes = r.notes;
    res.failures = r.failures;
    bool failed = !res.failures.empty();
    if (e.info.knownBug) res.outcome = failed ? Outcome::KnownBug : Outcome::Fixed;
    else res.outcome = failed ? Outcome::Fail : Outcome::Pass;
    if (res.outcome == Outcome::Fail || res.outcome == Outcome::Fixed) res.serverLog = external ? "" : server.LogTail(15);
    return res;
}

}  // namespace coop::bot
