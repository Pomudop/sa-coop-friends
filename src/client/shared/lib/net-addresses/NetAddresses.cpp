#include "shared/lib/net-addresses/NetAddresses.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "common/lib/util/Util.h"
#include "shared/config/Config.h"

namespace coop::netaddr {

namespace {
bool Contains(const std::wstring& s, const wchar_t* what) {
    std::wstring l = s;
    for (wchar_t& c : l) c = (wchar_t)towlower(c);
    return l.find(what) != std::wstring::npos;
}

bool Private(uint8_t a, uint8_t b) {
    return a == 10 || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168);
}
}  // namespace

std::vector<LocalAddress> List() {
    std::vector<LocalAddress> out;
    ULONG size = 16 * 1024;
    std::vector<uint8_t> buf;
    ULONG rc = ERROR_BUFFER_OVERFLOW;
    for (int i = 0; i < 3 && rc == ERROR_BUFFER_OVERFLOW; ++i) {
        buf.resize(size);
        rc = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr,
                                  (IP_ADAPTER_ADDRESSES*)buf.data(), &size);
    }
    if (rc != NO_ERROR) return out;
    for (auto* a = (IP_ADAPTER_ADDRESSES*)buf.data(); a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        std::wstring desc = a->Description ? a->Description : L"", name = a->FriendlyName ? a->FriendlyName : L"";
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            if (!u->Address.lpSockaddr || u->Address.lpSockaddr->sa_family != AF_INET) continue;
            auto* in = (sockaddr_in*)u->Address.lpSockaddr;
            auto* b = (uint8_t*)&in->sin_addr;
            if (b[0] == 127 || (b[0] == 169 && b[1] == 254)) continue;
            Kind kind;
            // Radmin VPN hands out 26.0.0.0/8, Hamachi 25.0.0.0/8 (public ranges they borrow), Tailscale 100.64.0.0/10
            if (Contains(desc, L"radmin") || Contains(name, L"radmin")) kind = Kind::Radmin;
            else if (Contains(desc, L"hamachi") || Contains(name, L"hamachi")) kind = Kind::Hamachi;
            else if (Contains(desc, L"zerotier") || Contains(name, L"zerotier")) kind = Kind::ZeroTier;
            else if (Contains(desc, L"tailscale") || Contains(name, L"tailscale") || (b[0] == 100 && (b[1] & 0xC0) == 64))
                kind = Kind::Tailscale;
            else if (Private(b[0], b[1])) kind = Kind::Lan;
            else continue;  // a public address of this PC: the invite code covers it
            char ip[INET_ADDRSTRLEN] = {};
            inet_ntop(AF_INET, &in->sin_addr, ip, sizeof(ip));
            out.push_back({kind, ip, WideToUtf8(name)});
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const LocalAddress& x, const LocalAddress& y) { return x.kind < y.kind; });
    return out;
}

std::string KindName(Kind k) {
    switch (k) {
        case Kind::Radmin: return "Radmin VPN";
        case Kind::Hamachi: return "Hamachi";
        case Kind::ZeroTier: return "ZeroTier";
        case Kind::Tailscale: return "Tailscale";
        case Kind::Lan: return TR("Домашняя сеть", "Home network");
    }
    return "";
}

bool SplitHostPort(const std::string& text, std::string& host, uint16_t& port) {
    size_t colon = text.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= text.size() || text.find(':') != colon) return false;
    std::string p = text.substr(colon + 1);
    if (p.find_first_not_of("0123456789") != std::string::npos || p.size() > 5) return false;
    int v = atoi(p.c_str());
    if (v <= 0 || v > 65535) return false;
    host = text.substr(0, colon);
    port = (uint16_t)v;
    return true;
}

}  // namespace coop::netaddr
