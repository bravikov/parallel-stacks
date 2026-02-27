#include "merger.hpp"


void add_level_cell(Html::TableRow& row, const LevelRange& level_range)
{
    if (level_range.first != level_range.last) {
        const auto range_string = std::to_string(level_range.first) + "–" + std::to_string(level_range.last);
        row.add_cell(Html::TableCell{range_string});
    } else {
        row.add_cell(Html::TableCell{std::to_string(level_range.first)});
    }

}

template<>
Html::TableRow HtmlTableRow<int>::to_row(const int& item, const LevelRange& level_range)
{
    Html::TableRow row;
    add_level_cell(row, level_range);
    row.add_cell(Html::TableCell{std::to_string(item)});
    return row;
}

template <>
std::size_t HtmlTableRow<int>::column_count() { return 2; }

template<>
Html::TableRow HtmlTableRow<std::string>::to_row(const std::string& item, const LevelRange& level_range)
{
    Html::TableRow row;
    add_level_cell(row, level_range);
    row.add_cell(Html::TableCell{item});
    return row;
}

template<>
std::size_t HtmlTableRow<std::string>::column_count() { return 2; }

template<>
Html::TableRow HtmlTableRow<Frame>::to_row(const Frame& frame, const LevelRange& level_range) {
    Html::TableRow row;

    add_level_cell(row, level_range);
    row.add_cell(Html::TableCell{frame.function});
    row.add_cell(Html::TableCell{frame.filename + ":" + std::to_string(frame.row) + ":" + std::to_string(frame.column)});

    return row;
}

template<>
std::size_t HtmlTableRow<Frame>::column_count() { return 3; }
