# C++ Threads Merger Library

## CLI

./threads-merger-cli -f "bbb:file2.cpp:15:4,aaa:file1.cpp:10:4; ccc:file3.cpp:15:4,aaa:file1.cpp:10:4;" > example.svg

./threads-merger-cli -f "bbb:file2.cpp:15:4,aaa:::; ccc:file3.cpp:15:4,aaa:::;" > example.svg

./threads-merger-cli "f,e,d,c,b,a; f,e,g,c,b,a" > example.svg

./threads-merger-cli -d "f,e,d,c,b,a; f,e,g,c,b,a" > example.dot

## Developing

Dependency for macOS:

    brew install graphviz

## Build WASM module

    emcc --std=c++23 -lembind html/html_table.cpp merger.cpp merger-wasm.cpp -o build/merger.js

## HTML Page Example

Run in the threads-merger folder:

    python3 -m http.server

Open [http://localhost:8000](http://localhost:8000) in a browser.
