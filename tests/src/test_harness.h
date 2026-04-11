#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>

// Simple hand-rolled test harness for mmemu
// Documentation: Add TEST_CASE("name") { ... } and use ASSERT(expr)

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& getTestCases() {
    static std::vector<TestCase> cases;
    return cases;
}

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        getTestCases().push_back({name, func});
    }
};

#define TEST_CASE(name) \
    void test_##name(); \
    static TestRegistrar registrar_##name(#name, test_##name); \
    void test_##name()

#define ASSERT(expr) \
    if (!(expr)) { \
        std::cerr << "Assertion failed: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define ASSERT_EQ(a, b) \
    if (!((a) == (b))) { \
        std::cerr << "Assertion failed: " << #a << " == " << #b \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define ASSERT_NE(a, b) \
    if (!((a) != (b))) { \
        std::cerr << "Assertion failed: " << #a << " != " << #b \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

// EXPECT_* mirrors ASSERT_* (both abort on failure in this harness)
#define EXPECT_TRUE(expr)   ASSERT(expr)
#define EXPECT_EQ(a, b)     ASSERT_EQ(a, b)

inline int runAllTests() {
    int passed = 0;
    auto& cases = getTestCases();
    std::cout << "Running " << cases.size() << " tests..." << std::endl;
    for (auto& tc : cases) {
        std::cout << "  [ RUN      ] " << tc.name << std::endl;
        tc.func();
        std::cout << "  [       OK ] " << tc.name << std::endl;
        passed++;
    }
    std::cout << "Passed " << passed << " / " << cases.size() << " tests." << std::endl;
    return 0;
}

inline int runTest(const std::string& name) {
    auto& cases = getTestCases();
    for (auto& tc : cases) {
        if (tc.name == name) {
            std::cout << "Running test: " << name << std::endl;
            tc.func();
            std::cout << "Test passed." << std::endl;
            return 0;
        }
    }
    std::cerr << "Test not found: " << name << std::endl;
    return 1;
}
