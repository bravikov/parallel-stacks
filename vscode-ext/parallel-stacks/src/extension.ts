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

            let stackInfo: string[] = [];
            const paths: Array<Array<{ function: string; filename: string; row: number; column: number }>> = [];

            // Для каждого потока запрашиваем стек вызовов

			// Specification of Thread type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_Thread
			// Specification of type StachFrame type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_StackFrame

            for (const thread of threads) {

				// https://microsoft.github.io/debug-adapter-protocol/specification#Requests_StackTrace
                const stackTraceResponse = await session.customRequest('stackTrace', {
                    threadId: thread.id,
                    startFrame: 0,
                    levels: 20 // Можно изменить глубину стека
                });
                const frames = stackTraceResponse.stackFrames || [];
                const frameInfo = frames.map((f: any) => `${f.name} (${f.source?.name}:${f.line})`).join('<br>');
                stackInfo.push(`<b>Поток: ${thread.name}</b><br>${frameInfo}`);

                // Собираем путь для merger (последовательность кадров)
                const path = frames.map((f: any) => ({
                    function: String(f.name || ''),
                    filename: String(f.source?.path || f.source?.name || ''),
                    row: Number(f.line || 0),
                    column: Number(f.column || 0)
                }));
                if (path.length > 0) {
                    paths.push(path);
                }
            }

            // Загружаем и используем merger (WASM) на стороне расширения, получаем DOT
            let dot = await getDotFromMerger(context, paths);

            // Открываем новую вкладку (Webview) только для рендера SVG из DOT
            const panel = vscode.window.createWebviewPanel(
                'parallelStacks',
                'Parallel Stacks',
                vscode.ViewColumn.One,
                {
                    enableScripts: true,
                    // Разрешаем доступ к локальным ресурсам из папки media
                    localResourceRoots: [vscode.Uri.file(path.join(context.extensionPath, 'media'))]
                }
            );

            const nonce = getNonce();
            const cspSource: string = (panel.webview as any).cspSource || '';
            // Встраиваем DOT-граф и подключаем Viz.js для рендера SVG
            // Примечание: для офлайн-использования лучше положить viz.js и full.render.js в папку `media/`
            panel.webview.html = `
                <!DOCTYPE html>
                <html lang="ru">
                <head>
                    <meta charset="UTF-8">
                    <meta http-equiv="Content-Security-Policy" content="default-src 'none'; img-src ${cspSource} https: data:; script-src 'nonce-${nonce}' https://cdn.jsdelivr.net; style-src ${cspSource} 'unsafe-inline'; font-src ${cspSource}; connect-src https:;">
                    <style>
                        body { font-family: -apple-system, BlinkMacSystemFont, Segoe UI, Helvetica, Arial, sans-serif; padding: 1em; }
                        b { color: #007acc; }
                        .thread { margin-bottom: 1em; }
                        #graph { margin-top: 1em; }
                        .error { color: #f00; white-space: pre-wrap; }
                    </style>
                </head>
                <body>
                    ${stackInfo.map(s => `<div class="thread">${s}</div>`).join('')}
                    <div id="graph"></div>

                    <script nonce="${nonce}" src="https://cdn.jsdelivr.net/npm/viz.js@2.1.2/viz.js"></script>
                    <script nonce="${nonce}" src="https://cdn.jsdelivr.net/npm/viz.js@2.1.2/full.render.js"></script>
                    <script nonce="${nonce}">
                        (function(){
                            const dot = ${JSON.stringify(dot)};
                            const container = document.getElementById('graph');
                            try {
                                const viz = new Viz();
                                viz.renderSVGElement(dot)
                                    .then(svg => {
                                        container.innerHTML = '';
                                        container.appendChild(svg);
                                    })
                                    .catch(err => {
                                        container.innerHTML = '<div class="error">Viz render error: ' + (err && (err.message || String(err))) + '</div>';
                                    });
                            } catch (e) {
                                container.innerHTML = '<div class="error">Viz init error: ' + (e && (e.message || String(e))) + '</div>';
                            }
                        })();
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

// ======== Helpers ========
function getNonce(): string {
    const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    let text = '';
    for (let i = 0; i < 32; i++) {
        text += possible.charAt(Math.floor(Math.random() * possible.length));
    }
    return text;
}
