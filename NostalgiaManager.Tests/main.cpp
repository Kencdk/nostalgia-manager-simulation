// Test runner entry point. Builds as NostalgiaManager.Tests.exe alongside
// NostalgiaManager.exe so CI can run it as a normal post-build step.
//
// Usage: NostalgiaManager.Tests.exe [dataDir]
//   dataDir defaults to "NostalgiaManager/data" (relative to the working
//   directory the exe is run from).
#include <iostream>
#include <string>

int RunCmFormatTests();
int RunDatabaseTests(const std::string& dataDir);
int RunMatchEngineTests(const std::string& dataDir);

int main(int argc, char** argv) {
    std::string dataDir = argc > 1 ? argv[1] : "NostalgiaManager/data";

    std::cout << "=== CM Position Format Tests ===\n";
    int cmFailures = RunCmFormatTests();

    std::cout << "\n=== Database Tests ===\n";
    int dbFailures = RunDatabaseTests(dataDir);

    std::cout << "\n=== MatchEngine Tests ===\n";
    int engineFailures = RunMatchEngineTests(dataDir);

    int total = cmFailures + dbFailures + engineFailures;
    std::cout << "\n===================================\n";
    std::cout << (total == 0 ? "All tests passed."
                              : std::to_string(total) + " test check(s) failed across all suites.")
              << "\n";
    return total == 0 ? 0 : 1;
}
