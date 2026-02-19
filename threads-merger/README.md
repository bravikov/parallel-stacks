# C++ Threads Merger Library

## CLI

    ./threads-merger-cli -f "bbb:file2.cpp:15:4,aaa:file1.cpp:10:4; ccc:file3.cpp:15:4,aaa:file1.cpp:10:4;" > example.svg

    ./threads-merger-cli -f "bbb:file2.cpp:15:4,aaa:::; ccc:file3.cpp:15:4,aaa:::;" > example.svg

    ./threads-merger-cli "f,e,d,c,b,a; f,e,g,c,b,a" > example.svg

    ./threads-merger-cli -d "f,e,d,c,b,a; f,e,g,c,b,a" > example.dot

## Developing

## Quick Start

```sh
# Prepare a conan profile.
conan profile detect --name apple-clang-debug

# Change the build type to Debug.
nano ~/.conan2/profiles/apple-clang-debug

# Clone the source code.
git clone git@github.com:bravikov/parallel-stacks.git

# Go to the library folder.
cd threads-merger

# Install dependencies with conan.
conan install . --build=missing --profile=apple-clang-debug

# Prepare building.
cmake --preset conan-debug

# Build.
cmake --build --preset=conan-debug

# Run unit tests.
./build/Debug/threads-merger-tests

# Run CLI.
./build/Debug/threads-merger-cli
```

Dependency for macOS:

    brew install graphviz

## HTML Page Example

Run in the threads-merger folder:

    python3 -m http.server

Open [http://localhost:8000](http://localhost:8000) in a browser.
