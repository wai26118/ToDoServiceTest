// tests/todo_service_test.cpp
// Unit tests for generate_id() from Utility.hpp using GoogleTest

#include <gtest/gtest.h>

#include <string>
#include <regex>           // for UUID format validation
#include <unordered_set>   // for uniqueness check
#include <vector>
#include <thread>
#include <mutex>

#include "../src/Utility.hpp"  // your generate_id() function

// Test fixture (optional but recommended for future shared setup)
class GenerateIdTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test 1: generate_id() produces a string in valid UUID v4 format
// TEST_F(GenerateIdTest, Format_IsValidUuidV4) {
//     std::string id = generate_id();

//     // UUID v4: 36 chars (32 hex + 4 hyphens)
//     ASSERT_EQ(id.length(), 36u) << "UUID string length should be 36";

//     // Correct hyphen positions
//     EXPECT_EQ(id[8],  '-') << "Hyphen missing/misplaced at position 9 (0-based)";
//     EXPECT_EQ(id[13], '-') << "Hyphen missing/misplaced at position 14";
//     EXPECT_EQ(id[18], '-') << "Hyphen missing/misplaced at position 19";
//     EXPECT_EQ(id[23], '-') << "Hyphen missing/misplaced at position 24";

//     // Version 4: position 14 must be '4'
//     EXPECT_EQ(id[14], '4') << "UUID version must be 4 (position 14 should be '4')";

//     // Variant: position 19 must be 8, 9, a, or b
//     char variant = id[19];
//     EXPECT_TRUE(variant == '8' || variant == '9' || variant == 'a' || variant == 'b')
//         << "Invalid variant nibble (position 19): " << variant;

//     // Full regex check (case-insensitive)
//     std::regex uuid_v4_regex(
//         "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$",
//         std::regex::icase
//     );

//     EXPECT_TRUE(std::regex_match(id, uuid_v4_regex))
//         << "Generated ID does not match UUID v4 pattern: " << id;
// }

// Test 2: Consecutive calls generate different IDs
TEST_F(GenerateIdTest, ConsecutiveCalls_Unique) {
    std::string id1 = generate_id();
    std::string id2 = generate_id();

    EXPECT_NE(id1, id2) << "Two consecutive UUIDs should be different";
}

// Test 3: Multi-threaded generation produces unique IDs
// TEST_F(GenerateIdTest, MultiThreadedGeneration_UniqueIds) {
//     constexpr int num_threads = 8;
//     constexpr int ids_per_thread = 250;

//     std::vector<std::string> all_ids;
//     std::mutex mutex;

//     std::vector<std::thread> threads;
//     for (int t = 0; t < num_threads; ++t) {
//         threads.emplace_back([&]() {
//             std::vector<std::string> local;
//             local.reserve(ids_per_thread);
//             for (int i = 0; i < ids_per_thread; ++i) {
//                 local.push_back(generate_id());
//             }

//             std::lock_guard<std::mutex> lock(mutex);
//             all_ids.insert(all_ids.end(), local.begin(), local.end());
//         });
//     }

//     for (auto& th : threads) {
//         th.join();
//     }

//     size_t total_ids = all_ids.size();
//     EXPECT_EQ(total_ids, num_threads * ids_per_thread);

//     std::unordered_set<std::string> unique(all_ids.begin(), all_ids.end());
//     EXPECT_EQ(unique.size(), total_ids)
//         << "Duplicate UUIDs found in multi-threaded generation ("
//         << (total_ids - unique.size()) << " duplicates)";
// }

// // Test 4: All non-hyphen characters are valid hex digits
// TEST_F(GenerateIdTest, OnlyValidHexCharacters) {
//     std::string id = generate_id();

//     std::string hex_only = id;
//     hex_only.erase(std::remove(hex_only.begin(), hex_only.end(), '-'), hex_only.end());

//     EXPECT_EQ(hex_only.length(), 32u);

//     for (char c : hex_only) {
//         bool is_hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
//         EXPECT_TRUE(is_hex) << "Invalid hex character '" << c << "' in UUID: " << id;
//     }
// }

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}