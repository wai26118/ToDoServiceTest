// Utility.hpp
// Common utility functions for the ToDoService project

#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <limits>

using namespace std;

static string generate_id() {
    static random_device rd;
    static mt19937_64 gen(rd());
    static uniform_int_distribution<uint64_t> dis(0, numeric_limits<uint64_t>::max());

    stringstream ss;
    ss << hex << setfill('0');

    uint64_t a = dis(gen);
    uint64_t b = dis(gen);

    // Set version to 4 (random) and variant bits
    b = (b & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL; // version 4
    b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL; // variant RFC 4122

    ss << setw(8)  << (a >> 32)
       << setw(4)  << ((a >> 16) & 0xFFFF)
       << setw(4)  << (a & 0xFFFF)
       << setw(4)  << ((b >> 48) & 0xFFFF)
       << setw(12) << (b & 0xFFFFFFFFFFFFULL);

    string id = ss.str();
    id.insert(8, "-");
    id.insert(13, "-");
    id.insert(18, "-");
    id.insert(23, "-");

    return id;
}

#endif // UTILITY_HPP