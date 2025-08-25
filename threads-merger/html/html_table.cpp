#include "html_table.hpp"

namespace {
// Simple HTML escaping for text nodes
std::string escape_html(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}
} // namespace

namespace Html {

TableCell::TableCell(std::string content, std::size_t colspan)
    : content_(std::move(content)), colspan_(colspan) {}

void TableCell::render(std::ostringstream& ss) const {
    ss << "        <td";
    if (colspan_ > 1) {
        ss << " colspan=\"" << colspan_ << "\"";
    }
    ss << ">" << escape_html(content_) << "</td>\n";
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
