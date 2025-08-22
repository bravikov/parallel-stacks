#include "merger.hpp"

#include <cctype>
#include <iostream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string_view ltrim(std::string_view sv) {
    size_t i = 0;
    while (i < sv.size() && std::isspace(static_cast<unsigned char>(sv[i]))) ++i;
    return sv.substr(i);
}

std::string_view rtrim(std::string_view sv) {
    size_t i = sv.size();
    while (i > 0 && std::isspace(static_cast<unsigned char>(sv[i - 1]))) --i;
    return sv.substr(0, i);
}

std::string_view trim(std::string_view sv) {
    return rtrim(ltrim(sv));
}

std::vector<std::vector<std::string>> parse_input(std::string_view input) {
    std::vector<std::vector<std::string>> result;

    for (auto stack_range : input | std::views::split(';')) {
        // Build string_view over the subrange
        std::string_view stack_sv(stack_range.begin(), stack_range.end());
        stack_sv = trim(stack_sv);
        if (stack_sv.empty()) continue;

        std::vector<std::string> stack_vec;
        for (auto token_range : stack_sv | std::views::split(',')) {
            std::string_view token_sv(token_range.begin(), token_range.end());
            token_sv = trim(token_sv);
            if (token_sv.empty()) continue;
            stack_vec.emplace_back(token_sv);
        }
        if (!stack_vec.empty()) {
            result.push_back(std::move(stack_vec));
        }
    }

    return result;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println(std::cerr, "Usage: {} [-d] \"f,e,d,c,b,a; f,e,g,c,b,a\" > example.svg", argv[0]);
        std::println(std::cerr, "  -d   output DOT instead of SVG");
        return 1;
    }

    try {
        bool output_dot = false;
        int argi = 1;
        if (argi < argc && std::string_view(argv[argi]) == "-d") {
            output_dot = true;
            ++argi;
        }

        if (argi >= argc) {
            std::println(std::cerr, "Error: missing input string. See --help.");
            return 1;
        }

        std::string_view input(argv[argi]);
        auto lists = parse_input(input);

        const auto tree = merge<std::string>(lists);
        const auto dot = get_dot_graph(tree);
        if (output_dot) {
            std::println("{}", dot);
        } else {
            const auto svg = dot_to_svg(dot);
            std::println("{}", svg);
        }
        return 0;
    } catch (const std::exception& ex) {
        std::println(std::cerr, "Error: {}", ex.what());
        return 2;
    }
}
