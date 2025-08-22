#include "merger.hpp"

#include <emscripten/bind.h>
#include <vector>

EMSCRIPTEN_BINDINGS(parallel_stacks_module) {
    emscripten::class_<Frame>("Frame")
        .constructor<>()
        .property("function", &Frame::function)
        .property("filename", &Frame::filename)
        .property("row", &Frame::row)
        .property("column", &Frame::column);
    
    // Биндинги для векторов
    emscripten::register_vector<Frame>("VectorFrame");
    emscripten::register_vector<std::vector<Frame>>("VectorVectorFrame");
    
    emscripten::function("merge_to_graphviz_dot", &merge_to_graphviz_dot<Frame>);
}
