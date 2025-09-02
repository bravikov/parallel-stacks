// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import * as path from 'path';

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

    // Use the console to output diagnostic information (console.log) and errors (console.error)
    // This line of code will only be executed once when your extension is activated
    console.log('Congratulations, your extension "parallel-stacks" is now active!');

    // Команда для отображения стеков потоков в новой вкладке (Webview)
    const disposable = vscode.commands.registerCommand('parallel-stacks.show', async () => {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            vscode.window.showWarningMessage('Нет активной сессии отладки.');
            return;
        }

        try {
            // Запрашиваем информацию о потоках
			// https://microsoft.github.io/debug-adapter-protocol/specification#Requests_Threads
            const threadsResponse = await session.customRequest('threads');
            const threads = threadsResponse.threads || [];

            const stacks: Array<Array<{ function: string; filename: string; row: number; column: number }>> = [];

            // Для каждого потока запрашиваем стек вызовов

			// Specification of Thread type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_Thread
			// Specification of StachFrame type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_StackFrame

            for (const thread of threads) {

				// https://microsoft.github.io/debug-adapter-protocol/specification#Requests_StackTrace
                const stackTraceResponse = await session.customRequest('stackTrace', {
                    threadId: thread.id,
                    startFrame: 0,
                    levels: 20 // Можно изменить глубину стека
                });

                // Specification of StackFrame type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_StackFrame
                // Specification of Source type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_Source
                const frames = stackTraceResponse.stackFrames || [];

                // Собираем путь для merger (последовательность кадров)
                const stack = frames.map((frame: any) => ({
                    function: String(frame.name || ''),
                    filename: frame.source?.path ? path.basename(frame.source.path) : String(frame.source?.name || ''),
                    row: Number(frame.line || 0),
                    column: Number(frame.column || 0)
                }));
                if (stack.length > 0) {
                    stacks.push(stack);
                }
            }

            // Загружаем и используем merger (WASM) на стороне расширения, получаем DOT
            let dot = await getDotFromMerger(context, stacks);

            // Открываем новую вкладку (Webview) только для рендера SVG из DOT
            const panel = vscode.window.createWebviewPanel(
                'parallelStacks',
                'Parallel Stacks',
                vscode.ViewColumn.One,
                {
                    enableScripts: true,
                }
            );

            const { instance } = await import("@viz-js/viz");
            const viz = await instance();

            // рендер в SVG
            const svg = viz.renderString(dot, { format: "svg" });

            panel.webview.html = `
                <!DOCTYPE html>
                <html lang="ru" style="height: 100%; margin: 0; padding: 0;">
                <head>
                    <meta charset="UTF-8">
                    <meta name="viewport" content="width=device-width, initial-scale=1.0">
                    <style>
                        html, body {
                            height: 100%;
                            margin: 0;
                            padding: 0;
                            overflow: hidden;
                            font-family: -apple-system, BlinkMacSystemFont, Segoe UI, Helvetica, Arial, sans-serif;
                        }
                        #graph {
                            width: 100%;
                            height: 100%;
                            margin: 0;
                            padding: 0;
                            display: flex;
                        }
                        svg {
                            flex: 1;
                            width: 100%;
                            height: 100%;
                        }
                    </style>
                </head>
                <body>
                    <div id="graph">${svg}</div>

                    <!-- подключение библиотеки -->
                    <script src="${panel.webview.asWebviewUri(vscode.Uri.joinPath(context.extensionUri, 'media', 'svg-pan-zoom.min.js'))}"></script>
                    <script>
                        const svgElement = document.querySelector('svg');
                        const panZoom = svgPanZoom(svgElement, {
                            zoomEnabled: true,
                            controlIconsEnabled: true,
                            fit: true,
                            center: true,
                            minZoom: 0.1,
                            maxZoom: 10
                        });
                    </script>
                </body>
                </html>
            `;
        } catch (err: any) {
            vscode.window.showErrorMessage(`Ошибка получения стеков: ${err.message || err}`);
        }
    });

    context.subscriptions.push(disposable);
}

// This method is called when your extension is deactivated
export function deactivate() {}

// ======== Merger (WASM) загрузка на стороне Node ========
let cachedMergerModule: any | null = null;

async function getMergerModule(context: vscode.ExtensionContext): Promise<any> {
    if (cachedMergerModule) {
        return cachedMergerModule;
    }

    const mergerJsPath = path.join(context.extensionPath, 'media', 'merger.js');
    const mergerWasmPath = path.join(context.extensionPath, 'media', 'merger.wasm');

    const g: any = global as any;
    const prevModule = g.Module;
    g.Module = {
        locateFile: (p: string) => (p.endsWith('merger.wasm') ? mergerWasmPath : p)
    };
    const mod = require(mergerJsPath);
    await new Promise<void>((resolve) => {
        if (mod && (mod.calledRun || mod.runtimeInitialized)) {
            resolve();
            return;
        }
        mod.onRuntimeInitialized = () => resolve();
    });
    cachedMergerModule = mod;
    if (prevModule === undefined) {
        delete g.Module;
    } else {
        g.Module = prevModule;
    }
    return mod;
}

async function getDotFromMerger(
    context: vscode.ExtensionContext,
    paths: Array<Array<{ function: string; filename: string; row: number; column: number }>>
): Promise<string> {
    try {
        const mod = await getMergerModule(context);
        const VV = mod.VectorVectorFrame;
        const V = mod.VectorFrame;
        const F = mod.Frame;
        const vv = new VV();
        for (const pathArr of paths) {
            const v = new V();
            for (const fr of pathArr) {
                const f = new F();
                f.function = fr.function;
                f.filename = fr.filename;
                f.row = fr.row;
                f.column = fr.column;
                v.push_back(f);
            }
            vv.push_back(v);
        }
        const dot: string = mod.merge_to_graphviz_dot(vv);
        return dot || 'digraph { }';
    } catch (e: any) {
        console.error('getDotFromMerger failed:', e);
        return 'digraph { label="merge_to_graphviz_dot failed" }';
    }
}
