#ifndef MERGER_HPP
#define MERGER_HPP

#include "html/html_table.hpp"

#include <string>
#include <functional>
#include <unordered_map>
#include <stack>
#include <iostream>
#include <sstream>
#include <vector>
#include <iterator>


struct Frame
{
    std::string function;
    std::string filename;
    int row = 0;
    int column = 0;

    auto operator<=>(const Frame&) const = default;

    // Оператор вывода для Frame
    friend std::ostream& operator<<(std::ostream& os, const Frame& frame) {
        os << "Frame{\"" << frame.function << "\", \"" << frame.filename << "\", "
           << frame.row << ", " << frame.column << "}";
        return os;
    }
};


namespace std
{

inline void hash_combine(std::size_t& seed, std::size_t hash) {
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<>
struct hash<Frame> {
    size_t operator()(const Frame& f) const noexcept {
        size_t seed = 0;
        hash_combine(seed, std::hash<std::string>{}(f.function));
        hash_combine(seed, std::hash<std::string>{}(f.filename));
        hash_combine(seed, std::hash<int>{}(f.row));
        hash_combine(seed, std::hash<int>{}(f.column));
        return seed;
    }
};

} // namespace std


template<typename T>
struct Node
{
    std::size_t count = 0;
    std::size_t level = 0;
    std::unordered_map<T, Node> next_nodes;

    auto operator<=>(const Node&) const = default;

    // Оператор вывода для Node
    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
        return node.print_node(os, 0);
    }

private:
    // Вспомогательная функция для рекурсивного вывода с отступами
    std::ostream& print_node(std::ostream& os, int indent) const;
};


// Реализация функции print_node
template<typename T>
std::ostream& Node<T>::print_node(std::ostream& os, int indent) const {
    const int MAX_DEPTH = 15; // Максимальная глубина рекурсии

    if (this->level > MAX_DEPTH) {
        os << "... (max depth exceeded)";
        return os;
    }

    std::string indent_str(indent * 2, ' '); // 2 пробела на уровень отступа

    os << indent_str << "Node{count=" << this->count << ", level=" << this->level;

    if (!this->next_nodes.empty()) {
        os << std::endl << indent_str << "  next_nodes:";
        for (const auto& [value, next_node] : this->next_nodes) {
            os << std::endl << indent_str << "    " << value << " ->";
            next_node.print_node(os, indent + 2);
        }
    }

    os << "}";
    return os;
}


template<typename T>
Node<T> merge(
    const std::vector<std::vector<T>>& lists,
    const std::size_t depth_limit = 0)
{
    Node<T> root{};

    for (const auto& list: lists) {
        if (list.empty()) continue;

        root.count++; // Увеличиваем счетчик для каждого непустого стека

        // Добавляем элементы в дерево, начиная с последнего (корневого)
        Node<T>* current = &root;

        auto last_item = list.rend();
        if (depth_limit > 0 && list.size() > depth_limit) {
            last_item = std::next(list.rbegin(), depth_limit);
        }

        std::size_t level = 0;
        for (auto valueIt = list.rbegin(); valueIt != last_item; valueIt++) {
            level++;

            // Получаем ссылку на узел (создает новый, если не существует)
            auto& node_ref = current->next_nodes[*valueIt];
            if (node_ref.count == 0) {
                // Новый узел
                node_ref.count = 1;
                node_ref.level = level;
            } else {
                // Существующий узел
                node_ref.count++;
            }
            current = &node_ref;
        }
    }

    return root;
}


template<typename T>
struct HtmlTableRow {
    static Html::TableRow to_row(const T& item, size_t level = 0);
    static std::size_t column_count();
};

template<typename T>
std::string get_dot_graph(const Node<T>& root) {
    std::ostringstream dot;
    dot << "digraph G {\n";
    dot << "  rankdir=BT;\n";
    dot << "  node [shape=plaintext];\n";

    int table_id_count = 0;

    using Pair = decltype(root.next_nodes)::value_type;

    std::stack<std::reference_wrapper<const Pair>> nodes_stack;
    std::stack<int> table_id_stack;

    for (const auto& next_node: root.next_nodes) {
        nodes_stack.push(next_node);
        table_id_stack.push(table_id_count++);
    }

    const Pair * next_node_ptr = nullptr;
    int current_table_id = 0;
    std::vector<std::reference_wrapper<const Pair>> current_table;

    std::unordered_multimap<int, int> table_links;

    auto save_table = [&]() {
        Html::Table table;

        if (!current_table.empty()) {
            Html::TableRow row;
            const auto thread_count = current_table[0].get().second.count;
            auto threads_str = std::to_string(thread_count) + " Thread";
            if (thread_count > 1) {
                threads_str += "s";
            }
            const size_t colspan = HtmlTableRow<T>::column_count();
            Html::TableCell cell{threads_str, colspan};
            row.add_cell(cell);
            table.add_row(row);
        }
        for (auto it = current_table.rbegin(); it != current_table.rend(); ++it) {
            const size_t level = it->get().second.level - 1;
            table.add_row(HtmlTableRow<T>::to_row(it->get().first, level));
        }

        current_table.clear();

        dot << "  table_" << current_table_id << " [label=<" << std::endl;
        table.render(dot);
        dot << "  >]" << std::endl << std::endl;
    };

    while (next_node_ptr != nullptr || !nodes_stack.empty()) {
        const Pair * current_node_ptr = nullptr;

        if (next_node_ptr) {
            current_node_ptr = next_node_ptr;
            next_node_ptr = nullptr;
        } else {
            current_node_ptr = &nodes_stack.top().get();
            nodes_stack.pop();

            current_table_id = table_id_stack.top();
            table_id_stack.pop();
        }

        current_table.push_back(*current_node_ptr);

        if (current_node_ptr->second.next_nodes.size() == 1) {
            next_node_ptr = &(*current_node_ptr->second.next_nodes.begin());
        }
        else if (current_node_ptr->second.next_nodes.size() > 1) {
            for (const auto& next_node: current_node_ptr->second.next_nodes) {
                nodes_stack.push(next_node);
                table_id_stack.push(table_id_count++);

                // Link the current table to a next table.
                table_links.emplace(current_table_id, table_id_stack.top());

            }

            save_table();
        }
        else {
            save_table();
        }
    }

    for (const auto& link : table_links) {
        dot << "  table_" << link.first << " -> table_" << link.second << " [arrowsize=2 minlen=2]" << std::endl;
    }

    dot << "}\n";
    return dot.str();
}

template<typename T>
std::string merge_to_graphviz_dot(const std::vector<std::vector<T>>& lists)
{
    const auto root = merge(lists);
    return get_dot_graph(root);
}

#endif // MERGER_HPP
