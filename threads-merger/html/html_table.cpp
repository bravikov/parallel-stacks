#include "html_table.hpp"

namespace Html {

TableCell::TableCell(std::string content, int colspan)
    : content_(std::move(content)), colspan_(colspan) {}

void TableCell::render(std::ostringstream& ss) const {
    ss << "        <td";
    if (colspan_ > 1) {
        ss << " colspan=\"" << colspan_ << "\"";
    }
    ss << ">" << content_ << "</td>\n";
}

void TableRow::add_cell(const TableCell& cell) {
    cells_.push_back(cell);
}

void TableRow::render(std::ostringstream& ss) const {
    ss << "      <tr>\n";
    for (const auto& cell : cells_) {
        cell.render(ss);
    }
    ss << "      </tr>\n";
}

void Table::add_row(const TableRow& row) {
    rows_.push_back(row);
}

void Table::render(std::ostringstream& ss) const {
    ss << "    <table border=\"1\" cellpadding=\"5\" cellspacing=\"0\">\n";
    for (const auto& row : rows_) {
        row.render(ss);
    }
    ss << "    </table>\n";
}

} // namespace Html
