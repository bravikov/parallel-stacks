#include "merger.hpp"

#include <graphviz/gvc.h>
#include <cgraph.h>

std::string dot_to_svg(const std::string& dot_content) {
    // Инициализируем Graphviz
    GVC_t* gvc = gvContext();
    if (!gvc) {
        return "Error: Failed to create Graphviz context";
    }

    // Создаем граф из DOT
    Agraph_t* graph = agmemread(const_cast<char*>(dot_content.c_str()));
    if (!graph) {
        gvFreeContext(gvc);
        return "Error: Failed to parse DOT content";
    }

    // Устанавливаем layout engine
    gvLayout(gvc, graph, "dot");

    // Конвертируем в SVG
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

    // Очищаем ресурсы
    gvFreeLayout(gvc, graph);
    agclose(graph);
    gvFreeContext(gvc);

    return svg_result;
}

template<>
Html::TableRow to_row<int>(const int& item, size_t level)
{
    Html::TableRow row;
    row.add_cell(Html::TableCell{std::to_string(level)});
    row.add_cell(Html::TableCell{std::to_string(item)});
    return row;
}

template<>
Html::TableRow to_row<std::string>(const std::string& item, size_t level)
{
    Html::TableRow row;
    row.add_cell(Html::TableCell{std::to_string(level)});
    row.add_cell(Html::TableCell{item});
    return row;
}

template<>
Html::TableRow to_row<Frame>(const Frame& frame, size_t level) {
    Html::TableRow row;

    row.add_cell(Html::TableCell{std::to_string(level)});
    row.add_cell(Html::TableCell{frame.function});
    row.add_cell(Html::TableCell{frame.filename + ":" + std::to_string(frame.row) + ":" + std::to_string(frame.column)});

    return row;
}
