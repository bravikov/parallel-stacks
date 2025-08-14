#ifndef MERGER_HPP
#define MERGER_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <map>
#include <iostream>


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
    std::map<T, Node> next_nodes;

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
Node<T> merge(const std::vector<std::vector<T>>& lists)
{
    Node<T> root{};

    for (const auto& list: lists) {
        if (list.empty()) continue;

        root.count++; // Увеличиваем счетчик для каждого непустого стека

        // Добавляем элементы в дерево, начиная с последнего (корневого)
        Node<T>* current = &root;
        for (std::size_t i = list.size(); i > 0; i--) {
            const auto& value = list[i - 1];
            std::size_t level = list.size() - i + 1; // Уровень: последний элемент = 1, предпоследний = 2, и т.д.

            // Получаем ссылку на узел (создает новый, если не существует)
            auto& node_ref = current->next_nodes[value];
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


#endif // MERGER_HPP
