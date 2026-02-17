// tests/todo_service_test.cpp
// Unit tests for generate_id() from Utility.hpp using GoogleTest

#include <gtest/gtest.h>

#include <string>
#include <regex>
#include <unordered_set>
#include <vector>
#include <thread>
#include <mutex>
#include <optional>
#include <map>

#include "../src/Utility.hpp"  // your generate_id() function
#include "../src/DbAccess.hpp" // PgPool, ToDoItem


// Test fixture (optional but recommended for future shared setup)
class GenerateIdTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test 1: Correct length & hyphen positions
TEST_F(GenerateIdTest, Format_CorrectLengthAndHyphens) {
    std::string id = generate_id();

    ASSERT_EQ(id.length(), 36u) << "ID length should be 36 (32 hex + 4 hyphens)";

    EXPECT_EQ(id[8],  '-') << "Hyphen missing/misplaced at position 9 (0-based)";
    EXPECT_EQ(id[13], '-') << "Hyphen missing/misplaced at position 14";
    EXPECT_EQ(id[18], '-') << "Hyphen missing/misplaced at position 19";
    EXPECT_EQ(id[23], '-') << "Hyphen missing/misplaced at position 24";
}


// Test 2: Consecutive calls generate different IDs
TEST_F(GenerateIdTest, ConsecutiveCalls_Unique) {
    std::string id1 = generate_id();
    std::string id2 = generate_id();

    EXPECT_NE(id1, id2) << "Two consecutive UUIDs should be different";
}

// Test 3: All non-hyphen characters are valid hex digits
TEST_F(GenerateIdTest, OnlyValidHexCharacters) {
    std::string id = generate_id();

    std::string hex_only = id;
    hex_only.erase(std::remove(hex_only.begin(), hex_only.end(), '-'), hex_only.end());

    EXPECT_EQ(hex_only.length(), 32u);

    for (char c : hex_only) {
        bool is_hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        EXPECT_TRUE(is_hex) << "Invalid hex character '" << c << "' in UUID: " << id;
    }
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}