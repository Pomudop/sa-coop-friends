#pragma once
// The server test scenarios (INF-05). Each one starts a fresh coop-server.exe with the settings it needs, plays it with
// fake players (BotClient) and checks what the server does. A scenario marked with a backlog id describes a known bug:
// it is expected to fail until that task is done, and passing tells that the mark should go.
#include <cstdint>
#include <string>
#include <vector>

namespace coop::bot {

struct Settings {
    std::wstring serverExe;  // coop-server.exe to start for each scenario
    std::wstring workDir;    // its server.ini and data go to workDir\<scenario>\ (with trailing backslash)
    uint16_t port = 7790;    // the first scenario's port; each next one takes the next port
    std::string connect;     // host of a server already running: only the scenarios that need no server of their own
    uint16_t connectPort = 0;
};

struct ScenarioInfo {
    const char* name;
    const char* what;      // one line for the report
    const char* knownBug;  // backlog id of the bug it shows, or nullptr
    bool ownServer;        // needs a server with its own settings or a fresh state (not run with --connect)
};

enum class Outcome {
    Pass,
    Fail,
    KnownBug,  // failed as expected: the bug is still there
    Fixed,     // a known bug's scenario passed: the bug seems fixed, the mark should go (counts as a failure)
    Skipped,   // needs a server of its own, and the run is against --connect
    NoServer,  // coop-server.exe did not start (counts as a failure)
};

struct Result {
    Outcome outcome = Outcome::Pass;
    std::vector<std::string> notes;     // measurements worth printing
    std::vector<std::string> failures;  // what went wrong
    std::string serverLog;              // the server log's tail when something went wrong
};

const std::vector<ScenarioInfo>& ScenarioList();
Result RunScenario(size_t index, const Settings& settings);

}  // namespace coop::bot
