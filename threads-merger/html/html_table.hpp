#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <string_view>

/* Lightweight builders that emit HTML table markup for inline use inside DOT graphs.
 *
 * Graphviz HTML-Like Labels: https://www.graphviz.org/doc/info/shapes.html#html
 *
 */

namespace Html {

class TableCell {
public:
    explicit TableCell(std::string content, std::size_t colspan = 1);

    friend class TableRow;

private:
    std::string content_;
    std::size_t colspan_;

    void render(std::ostringstream& ss, std::string_view sides) const;
    std::size_t colspan() const;
};

class TableRow {
public:
    void add_cell(const TableCell& cell);

    friend class Table;

private:
    std::vector<TableCell> cells_;

    void render(std::ostringstream& ss, std::size_t row_index) const;
};

class Table {
public:
    void add_row(const TableRow& row);
    void render(std::ostringstream& ss) const;

private:
    std::vector<TableRow> rows_;
};

} // namespace Html
