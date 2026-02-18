#include <time.h>
#include <fstream>
#include <gtest/gtest.h>

#include "merger.hpp"

std::filesystem::path baseFolder;

enum {
    A=0,
    B=1,
    C=2,
    D=3,
    E=4,
    F=5,
    G=6,
};

TEST(merge, two_intersected_stacks)
{
    auto input = std::vector<std::vector<int>>{
        {F, E, D, C, B, A},
        {F, E, G, C, B, A},
    };

    Node<int> nodeF1{.count=1, .level=6, .next_nodes={} };
    Node<int> nodeE1{.count=1, .level=5, .next_nodes={{F, nodeF1},} };
    Node<int> nodeD {.count=1, .level=4, .next_nodes={{E, nodeE1},} };

    Node<int> nodeF2{.count=1, .level=6, .next_nodes={} };
    Node<int> nodeE2{.count=1, .level=5, .next_nodes={{F, nodeF2},} };
    Node<int> nodeG {.count=1, .level=4, .next_nodes={{E, nodeE2},} };

    Node<int> nodeC {.count=2, .level=3, .next_nodes={{D, nodeD}, {G, nodeG}, }};
    Node<int> nodeB {.count=2, .level=2, .next_nodes={{C, nodeC}, }};
    Node<int> nodeA {.count=2, .level=1, .next_nodes={{B, nodeB}, }};

    Node<int> root  {.count=2, .level=0, .next_nodes={{A, nodeA}, }};

    const auto& expected = root;

    auto actual = merge<int>(input);

    EXPECT_EQ(actual, expected);
}


TEST(merge, independent_stacks) {
    auto input = std::vector<std::vector<int>>{
        {C, B, A},
        {F, E, D},
    };

    // Ожидаемый результат: два независимых стека
    Node<int> nodeC{.count=1, .level=3, .next_nodes={} };
    Node<int> nodeB{.count=1, .level=2, .next_nodes={{C, nodeC}} };
    Node<int> nodeA{.count=1, .level=1, .next_nodes={{B, nodeB}} };

    Node<int> nodeF{.count=1, .level=3, .next_nodes={}};
    Node<int> nodeE{.count=1, .level=2, .next_nodes={{F, nodeF}} };
    Node<int> nodeD{.count=1, .level=1, .next_nodes={{E, nodeE}} };

    Node<int> root {.count=2, .level=0, .next_nodes={{A, nodeA}, {D, nodeD}}};

    const auto& expected = root;
    auto actual = merge<int>(input);

    EXPECT_EQ(actual, expected);
}

TEST(merge, stack_with_recursion) {
    auto input = std::vector<std::vector<int>>{
        {B, A, A, A},
    };

    // Ожидаемый результат: стек с рекурсией
    // Одинаковые элементы на одном уровне должны объединяться
    Node<int> nodeB {.count=1, .level=4, .next_nodes={} };
    Node<int> nodeA3{.count=1, .level=3, .next_nodes={{B, nodeB}} };
    Node<int> nodeA2{.count=1, .level=2, .next_nodes={{A, nodeA3}} };
    Node<int> nodeA1{.count=1, .level=1, .next_nodes={{A, nodeA2}} };

    Node<int> root{.count=1, .level=0, .next_nodes={{A, nodeA1}} };

    const auto& expected = root;
    auto actual = merge<int>(input);

    EXPECT_EQ(actual, expected);
}

TEST(merge, empty_lists) {
    // Тест на пустой список стеков
    auto input = std::vector<std::vector<int>>{};

    Node<int> expected{};

    auto actual = merge<int>(input);
    EXPECT_EQ(actual, expected);
}

TEST(merge, empty_stacks) {
    // Тест на список, содержащий только пустые стеки
    auto input = std::vector<std::vector<int>>{
        {},
        {},
        {}
    };

    Node<int> expected{};

    auto actual = merge<int>(input);
    EXPECT_EQ(actual, expected);
}

TEST(merge, mixed_empty_stacks) {
    // Тест на список, содержащий смесь пустых и непустых стеков
    auto input = std::vector<std::vector<int>>{
        {},
        {B, A},
        {},
        {D, C},
        {}
    };

    // Ожидаемый результат: только непустые стеки
    Node<int> nodeB{.count=1, .level=2, .next_nodes={}};
    Node<int> nodeA{.count=1, .level=1, .next_nodes={{B, nodeB}}};

    Node<int> nodeD{.count=1, .level=2, .next_nodes={}};
    Node<int> nodeC{.count=1, .level=1, .next_nodes={{D, nodeD}}};

    Node<int> expected{.count=2, .level=0, .next_nodes={{A, nodeA}, {C, nodeC}}};

    auto actual = merge<int>(input);
    EXPECT_EQ(actual, expected);
}

TEST(merge, frame_stacks) {
    // Тест на использование структуры Frame вместо int
    auto input = std::vector<std::vector<Frame>>{
        {Frame{"func2", "file2.cpp", 20, 10}, Frame{"func1", "file1.cpp", 10, 5}},
        {Frame{"func3", "file3.cpp", 30, 15}, Frame{"func1", "file1.cpp", 10, 5}}
    };

    Node<Frame> nodeFunc2{.count=1, .level=2, .next_nodes={} };
    Node<Frame> nodeFunc3{.count=1, .level=2, .next_nodes={} };
    Node<Frame> nodeFunc1{.count=2, .level=1,
        .next_nodes={{Frame{"func2", "file2.cpp", 20, 10}, nodeFunc2}, {Frame{"func3", "file3.cpp", 30, 15}, nodeFunc3}} };

    Node<Frame> expected{.count=2, .level=0,
        .next_nodes={{Frame{"func1", "file1.cpp", 10, 5}, nodeFunc1}}};

    auto actual = merge<Frame>(input);
    EXPECT_EQ(actual, expected);
}

TEST(stack_depth_limit, default_limit)
{
    auto input = std::vector<std::vector<int>>{
        {F, E, D, C, B, A},
    };

    Node<int> nodeF {.count=1, .level=6, .next_nodes={} };
    Node<int> nodeE {.count=1, .level=5, .next_nodes={{F, nodeF}, }};
    Node<int> nodeD {.count=1, .level=4, .next_nodes={{E, nodeE}, }};
    Node<int> nodeC {.count=1, .level=3, .next_nodes={{D, nodeD}, }};
    Node<int> nodeB {.count=1, .level=2, .next_nodes={{C, nodeC}, }};
    Node<int> nodeA {.count=1, .level=1, .next_nodes={{B, nodeB}, }};

    Node<int> root  {.count=1, .level=0, .next_nodes={{A, nodeA}, }};

    const auto& expected = root;

    auto actual = merge<int>(input);

    EXPECT_EQ(actual, expected);
}

TEST(stack_depth_limit, limit_3)
{
    auto input = std::vector<std::vector<int>>{
        {F, E, D, C, B, A},
    };

    Node<int> nodeC {.count=1, .level=3, .next_nodes={} };
    Node<int> nodeB {.count=1, .level=2, .next_nodes={{C, nodeC}, }};
    Node<int> nodeA {.count=1, .level=1, .next_nodes={{B, nodeB}, }};

    Node<int> root  {.count=1, .level=0, .next_nodes={{A, nodeA}, }};

    const auto& expected = root;

    auto actual = merge<int>(input, 3);

    EXPECT_EQ(actual, expected);
}

TEST(stack_depth_limit, two_intersected_stacks_limit_4)
{
    auto input = std::vector<std::vector<int>>{
        {F, E, D, C, B, A},
        {F, E, G, C, B, A},
    };

    Node<int> nodeD {.count=1, .level=4, .next_nodes={} };

    Node<int> nodeG {.count=1, .level=4, .next_nodes={} };

    Node<int> nodeC {.count=2, .level=3, .next_nodes={{D, nodeD}, {G, nodeG}, }};
    Node<int> nodeB {.count=2, .level=2, .next_nodes={{C, nodeC}, }};
    Node<int> nodeA {.count=2, .level=1, .next_nodes={{B, nodeB}, }};

    Node<int> root  {.count=2, .level=0, .next_nodes={{A, nodeA}, }};

    const auto& expected = root;

    auto actual = merge<int>(input, 4);

    EXPECT_EQ(actual, expected);
}

TEST(dot, basic) {
    auto input = std::vector<std::vector<int>>{
       {7, 6, 5, 4, 3, 2, 1},
       {7, 6, 5, 3, 2, 1},
    };

    std::ifstream expectedDotFile{baseFolder / "tests_data/test-int.dot"};
    ASSERT_FALSE(expectedDotFile.fail());

    std::string expectedDot{
        std::istreambuf_iterator<char>(expectedDotFile),
        std::istreambuf_iterator<char>()
    };

    const auto tree = merge<int>(input);
    const auto actualDot = get_dot_graph(tree);

    ASSERT_EQ(actualDot, expectedDot);
}

TEST(dot, frame_stacks) {
    // Тест на использование структуры Frame вместо int
    auto input = std::vector<std::vector<Frame>>{
        {Frame{"func2", "file2.cpp", 20, 10}, Frame{"func1", "file1.cpp", 10, 5}},
        {Frame{"func3", "file3.cpp", 30, 15}, Frame{"func1", "file1.cpp", 10, 5}}
    };

    std::ifstream expectedDotFile{baseFolder / "tests_data/test-frame.dot"};
    ASSERT_FALSE(expectedDotFile.fail());

    std::string expectedDot{
        std::istreambuf_iterator<char>(expectedDotFile),
        std::istreambuf_iterator<char>()
    };

    const auto tree = merge<Frame>(input);
    const auto actualDot = get_dot_graph(tree);

    ASSERT_EQ(actualDot, expectedDot);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    baseFolder = std::filesystem::path{argv[0]}.parent_path();
    return RUN_ALL_TESTS();
}
