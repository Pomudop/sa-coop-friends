#pragma once

// SA:COOP Friends: a fork of SA:COOP by Karimov-Akbar (github.com/Karimov-Akbar/sa-coop), see NOTICE.md and docs/fork.
#define COOP_NAME "SA:COOP Friends"
#define COOP_EDITION "Friends"  // next to the version in F9: the panel title stays "SA:COOP" in the game's gothic font
#define COOP_VERSION_STR "0.3.0.1"  // tools/build.py gives the version resource its numbers from this

#ifndef RC_INVOKED  // the resource compiler reads only the macros above (VersionInfo.rc)
#include <cstdint>

namespace coop {
// Upstream's protocol number, updated on every merge of an upstream protocol change (docs/fork/BACKLOG.md, INF-03).
constexpr uint32_t kUpstreamProtocol = 19;
// The fork's numbers start at 1000 so that the fork and upstream refuse each other with "another version of the mod"
// instead of connecting and silently disagreeing on messages.
constexpr uint32_t kForkProtocolBase = 1000;
// Bumped on any incompatible change of the network protocol.
constexpr uint32_t kProtocolVersion = kForkProtocolBase + kUpstreamProtocol;
constexpr uint16_t kDefaultPort = 7788;
// Size of gta_sa.exe 1.0 US (HOODLUM) — the only supported executable.
constexpr uint32_t kSupportedExeSize = 14383616;
}  // namespace coop
#endif
