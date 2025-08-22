#include "merger.hpp"

#include <cctype>
#include <iostream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <stdexcept>
#include <vector>

// Graphviz is only needed for CLI SVG rendering
#include <graphviz/gvc.h>
#include <graphviz/cgraph.h>

static std::string dot_to_svg(const std::string& dot_content) {
    GVC_t* gvc = gvContext();
    if (!gvc) {
        return "Error: Failed to create Graphviz context";
    }

    Agraph_t* graph = agmemread(const_cast<char*>(dot_content.c_str()));
    if (!graph) {
        gvFreeContext(gvc);
        return "Error: Failed to parse DOT content";
    }

    gvLayout(gvc, graph, "dot");

    char* svg_data = nullptr;
    size_t svg_length = 0;
    gvRenderData(gvc, graph, "svg", (char**)&svg_data, &svg_length);

    std::string svg_result;
    if (svg_data && svg_length > 0) {
        svg_result = std::string(svg_data, svg_length);
        gvFreeRenderData(svg_data);
    } else {
        svg_result = "Error: Failed to generate SVG";
    }

    gvFreeLayout(gvc, graph);
    agclose(graph);
    gvFreeContext(gvc);

    return svg_result;
}

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

std::vector<std::vector<Frame>> parse_input_frames(std::string_view input) {
    std::vector<std::vector<Frame>> result;

    for (auto stack_range : input | std::views::split(';')) {
        std::string_view stack_sv(stack_range.begin(), stack_range.end());
        stack_sv = trim(stack_sv);
        if (stack_sv.empty()) continue;

        std::vector<Frame> stack_vec;
        for (auto token_range : stack_sv | std::views::split(',')) {
            std::string_view token_sv(token_range.begin(), token_range.end());
            token_sv = trim(token_sv);
            if (token_sv.empty()) continue;

            // token format: function[:filename[:row[:column]]]
            // find up to three ':' separators, but all fields except function are optional
            auto p1 = token_sv.find(':');
            auto p2 = (p1 != std::string_view::npos) ? token_sv.find(':', p1 + 1) : std::string_view::npos;
            auto p3 = (p2 != std::string_view::npos) ? token_sv.find(':', p2 + 1) : std::string_view::npos;

            std::string_view func_sv = (p1 == std::string_view::npos) ? token_sv : token_sv.substr(0, p1);
            std::string_view file_sv;
            std::string_view row_sv;
            std::string_view col_sv;

            if (p1 != std::string_view::npos && p2 == std::string_view::npos) {
                // function:filename
                file_sv = token_sv.substr(p1 + 1);
            } else if (p1 != std::string_view::npos && p2 != std::string_view::npos && p3 == std::string_view::npos) {
                // function:filename:row
                file_sv = token_sv.substr(p1 + 1, p2 - p1 - 1);
                row_sv  = token_sv.substr(p2 + 1);
            } else if (p1 != std::string_view::npos && p2 != std::string_view::npos && p3 != std::string_view::npos) {
                // function:filename:row:column
                file_sv = token_sv.substr(p1 + 1, p2 - p1 - 1);
                row_sv  = token_sv.substr(p2 + 1, p3 - p2 - 1);
                col_sv  = token_sv.substr(p3 + 1);
            }

            func_sv = trim(func_sv);
            file_sv = trim(file_sv);
            row_sv  = trim(row_sv);
            col_sv  = trim(col_sv);

            if (func_sv.empty()) {
                throw std::runtime_error("Invalid frame token (empty func): '" + std::string(token_sv) + "'");
            }

            int row = 0;
            int col = 0;
            if (!row_sv.empty()) {
                try { row = std::stoi(std::string(row_sv)); }
                catch (const std::exception&) { throw std::runtime_error("Invalid frame token (row not integer): '" + std::string(token_sv) + "'"); }
            }
            if (!col_sv.empty()) {
                try { col = std::stoi(std::string(col_sv)); }
                catch (const std::exception&) { throw std::runtime_error("Invalid frame token (column not integer): '" + std::string(token_sv) + "'"); }
            }

            Frame f;
            f.function = std::string(func_sv);
            f.filename = std::string(file_sv);
            f.row = row;
            f.column = col;

            stack_vec.emplace_back(std::move(f));
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
        std::println(std::cerr, "Usage: {} [-d] -f \"<func,file,line,col,...;...>\" > example.svg", argv[0]);
        std::println(std::cerr, "  -d   output DOT instead of SVG");
        std::println(std::cerr, "  -f   interpret input as formatted frames 'func:file:line:col, ...; ...'");
        return 1;
    }

    try {
        bool output_dot = false;
        bool formatted_frames = false;
        int argi = 1;
        while (argi < argc && argv[argi][0] == '-') {
            std::string_view opt(argv[argi]);
            if (opt == "-d") {
                output_dot = true;
            } else if (opt == "-f") {
                formatted_frames = true;
            } else {
                std::println(std::cerr, "Unknown option: {}", opt);
                return 1;
            }
            ++argi;
        }

        if (argi >= argc) {
            std::println(std::cerr, "Error: missing input string. See --help.");
            return 1;
        }

        std::string_view input(argv[argi]);
        if (formatted_frames) {
            auto lists = parse_input_frames(input);
            const auto tree = merge<Frame>(lists);
            const auto dot = get_dot_graph(tree);
            if (output_dot) {
                std::println("{}", dot);
            } else {
                const auto svg = dot_to_svg(dot);
                std::println("{}", svg);
            }
            return 0;
        }

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
