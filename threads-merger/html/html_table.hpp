#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace Html {

class TableCell {
public:
    explicit TableCell(std::string content, int colspan = 1);
    void render(std::ostringstream& ss) const;

private:
    std::string content_;
    int colspan_;
};

class TableRow {
public:
    void add_cell(const TableCell& cell);
    void render(std::ostringstream& ss) const;

private:
    std::vector<TableCell> cells_;
};

class Table {
public:
    void add_row(const TableRow& row);
    void render(std::ostringstream& ss) const;

private:
    std::vector<TableRow> rows_;
};

} // namespace Html
