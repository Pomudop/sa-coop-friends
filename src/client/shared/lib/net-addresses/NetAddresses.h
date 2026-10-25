#pragma once
// The IPv4 addresses of this PC that a friend can reach without an invite code: a virtual LAN (Radmin VPN, Hamachi,
// ZeroTier, Tailscale) or the home network. Hosts show them in the F9 window (CONN-01).
#include <string>
#include <vector>

namespace coop::netaddr {

enum class Kind { Radmin, Hamachi, ZeroTier, Tailscale, Lan };

struct LocalAddress {
    Kind kind;
    std::string ip;       // dotted IPv4
    std::string adapter;  // the adapter's friendly name, for the log
};

// Up adapters only; virtual LANs first, then the home network. No loopback, no link-local (169.254), no IPv6.
std::vector<LocalAddress> List();

// "Radmin VPN", "Hamachi", ... (Russian or English by the language setting)
std::string KindName(Kind k);

// Splits "26.1.2.3:7788" into host and port; leaves anything else (an invite code, a host name) as it is.
bool SplitHostPort(const std::string& text, std::string& host, uint16_t& port);

}  // namespace coop::netaddr
