# Parallel Stacks

The Parallel Stacks extension is useful for debugging multithreaded applications.

The extension renders thread stack traces as a graph.

The principle of converting stacks into a graph is illustrated here:

![Stacks collapsing into a parallel stacks graph](images/stacks-to-parallel-stacks.png)

What the graph looks like for a real Go program:

![Debugging of Go App](images/example-gorilla-websocket-chat-1.png)
![Debugging of Go App](images/example-gorilla-websocket-chat-2.png)
![Debugging of Go App](images/example-gorilla-websocket-chat-3.png)

This is the [Chat Example](https://github.com/gorilla/websocket/tree/main/examples/chat) application from the [Gorilla WebSocket](https://github.com/gorilla/websocket) project.

How to use the extension:

1. Install the extension.
2. Start debugging your application.
3. Break execution at a breakpoint.
4. Launch Parallel Stacks through the command palette by pressing (`Ctrl+Shift+P` or `Cmd+Shift+P` on Mac) and typing `Parallel Stacks`.

Stack depth limit: 200 frames. Deeper frames are not displayed.

The graph style, i.e. the color of the background, blocks, and text, varies depending on the VS Code theme. The style can be changed on the fly without restarting VS Code and without regenerating the graph.
