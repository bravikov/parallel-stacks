#include "merger.hpp"


template<>
Html::TableRow HtmlTableRow<int>::to_row(const int& item, size_t level)
{
    Html::TableRow row;
    row.add_cell(Html::TableCell{std::to_string(level)});
    row.add_cell(Html::TableCell{std::to_string(item)});
    return row;
}

template<>
std::size_t HtmlTableRow<int>::column_count() { return 2; }

template<>
Html::TableRow HtmlTableRow<std::string>::to_row(const std::string& item, size_t level)
{
    Html::TableRow row;
    row.add_cell(Html::TableCell{std::to_string(level)});
    row.add_cell(Html::TableCell{item});
    return row;
}

template<>
std::size_t HtmlTableRow<std::string>::column_count() { return 2; }

template<>
Html::TableRow HtmlTableRow<Frame>::to_row(const Frame& frame, size_t level) {
    Html::TableRow row;

    row.add_cell(Html::TableCell{std::to_string(level)});
    row.add_cell(Html::TableCell{frame.function});
    row.add_cell(Html::TableCell{frame.filename + ":" + std::to_string(frame.row) + ":" + std::to_string(frame.column)});

    return row;
}

template<>
std::size_t HtmlTableRow<Frame>::column_count() { return 3; }
