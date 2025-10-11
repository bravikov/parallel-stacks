#include "html_table.hpp"

#include <cctype>

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

void TableCell::render(std::ostringstream& ss, std::string_view sides) const {
    ss << "        <td";
    if (colspan_ > 1) {
        ss << " COLSPAN=\"" << colspan_ << "\"";
    }
    if (sides.empty()) {
        ss << " BORDER=\"0\"";
    } else if (sides != "LTRB") {
        ss << " SIDES=\"";
        for (char side : sides) {
            ss << static_cast<char>(std::toupper(static_cast<unsigned char>(side)));
        }
        ss << "\"";
    }
    ss << ">";
    ss << "<FONT POINT-SIZE=\"40\">";
    ss << escape_html(content_);
    ss << "</FONT>";
    ss << "</td>\n";
}

std::size_t TableCell::colspan() const {
    return colspan_;
}

void TableRow::add_cell(const TableCell& cell) {
    cells_.push_back(cell);
}

void TableRow::render(std::ostringstream& ss, std::size_t row_index) const {
    ss << "      <tr>\n";
    std::size_t column_index = 0;
    for (const auto& cell : cells_) {
        std::string sides;
        if (column_index > 0) {
            sides.push_back('L');
        }
        if (row_index > 0) {
            sides.push_back('T');
        }
        cell.render(ss, sides);
        column_index += cell.colspan();
    }
    ss << "      </tr>\n";
}

void Table::add_row(const TableRow& row) {
    rows_.push_back(row);
}

void Table::render(std::ostringstream& ss) const {
    ss << "    <table BORDER=\"1\" CELLBORDER=\"1\" CELLPADDING=\"10\" CELLSPACING=\"0\" STYLE=\"ROUNDED\">\n";
    for (std::size_t row_index = 0; row_index < rows_.size(); ++row_index) {
        rows_[row_index].render(ss, row_index);
    }
    ss << "    </table>\n";
}

} // namespace Html
