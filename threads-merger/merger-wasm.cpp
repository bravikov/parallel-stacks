#include "merger.hpp"

#include <emscripten/bind.h>
#include <vector>
#include <map>

float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}


EMSCRIPTEN_BINDINGS(parallel_stacks_module) {
    emscripten::function("lerp", &lerp);

    emscripten::class_<Frame>("Frame")
        .constructor<>()
        .property("function", &Frame::function)
        .property("filename", &Frame::filename)
        .property("row", &Frame::row)
        .property("column", &Frame::column);
    
    emscripten::class_<Node<Frame>>("Node")
        .constructor<>()
        .property("count", &Node<Frame>::count)
        .property("level", &Node<Frame>::level)
        .property("next_nodes", &Node<Frame>::next_nodes);
    
    // Биндинги для векторов
    emscripten::register_vector<Frame>("VectorFrame");
    emscripten::register_vector<std::vector<Frame>>("VectorVectorFrame");
    
    // Биндинги для map
    emscripten::register_map<Frame, Node<Frame>>("MapFrameNode");
    
    emscripten::function("merge", &merge<Frame>);
}
