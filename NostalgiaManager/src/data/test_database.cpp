#include "gtest/gtest.h"
#include "../data/Database.h"
#include <sstream>
#include <string>

// Helper function to simulate reading a CSV file content into a string
std::string readFileContent(const std::string& csv_data) {
    return csv_data;
}

TEST(DatabaseTest, SuccessfulDataLoadingAndParsing) {
    // Setup: Valid CSV data structure. Header row + two valid records.
    std::string validCsv = 
        "MatchID,PlayerA_ID,PlayerB_ID,MatchDate\r\n" // Header
        "1001,1,2,2024-01-15\r\n"                     // Record 1
        "1002,3,4,2024-01-16\r\n";                      // Record 2

    Database db; // Assuming Database constructor handles initial setup or is stateless for parsing

    // Action: Load and parse the data
    auto records = db.LoadMatchesFromCsv(readFileContent(validCsv));

    // Assertions
    ASSERT_EQ(records.size(), 2);
    EXPECT_TRUE(records[0].hasMatchID());
    EXPECT_EQ(records[0].getMatchID(), 1001);
    EXPECT_EQ(records[1].getMatchID(), 1002);
}

TEST(DatabaseTest, HandlesMissingRequiredFields) {
    // Setup: A record missing PlayerB_ID (Critical failure).
    std::string partialCsv = 
        "MatchID,PlayerA_ID,PlayerB_ID,MatchDate\r\n" // Header
        "2001,5,6,2024-02-01\r\n"                   // Valid record
        "2002,7,,2024-02-02\r\n";                    // Missing field!

    Database db;

    // Action: Load and parse the data, expecting failure or skipping bad records.
    auto records = db.LoadMatchesFromCsv(readFileContent(partialCsv));

    // Assertions: Expect only the valid record to be successfully parsed/loaded
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].getMatchID(), 2001);
}

TEST(DatabaseTest, HandlesEmptyOrMalformedInput) {
    // Case 1: Completely empty input
    std::string emptyCsv = "";
    Database db;
    auto recordsEmpty = db.LoadMatchesFromCsv(readFileContent(emptyCsv));
    ASSERT_TRUE(recordsEmpty.empty());

    // Case 2: Only header row (no data)
    std::string headerOnlyCsv = "MatchID,PlayerA_ID,PlayerB_ID,MatchDate\r\n";
    auto recordsHeaderOnly = db.LoadMatchesFromCsv(readFileContent(headerOnlyCsv));
    ASSERT_TRUE(recordsHeaderOnly.empty());

    // Case 3: Malformed CSV (e.g., too many fields) - depends on library robustness, testing graceful failure expected
}