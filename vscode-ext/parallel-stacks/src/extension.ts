// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import * as path from 'path';
import { promises as fs } from 'fs';
import * as os from 'os';

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

    // Use the console to output diagnostic information (console.log) and errors (console.error)
    // This line of code will only be executed once when your extension is activated
    console.log('Congratulations, your extension "parallel-stacks" is now active!');

    // Command to display thread stacks in a new tab (Webview)
    const disposable = vscode.commands.registerCommand('parallel-stacks.show', async () => {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            vscode.window.showWarningMessage('No active debug session.');
            return;
        }

        try {
            // Request thread information
            // https://microsoft.github.io/debug-adapter-protocol/specification#Requests_Threads
            const threadsResponse = await session.customRequest('threads');
            const threads = threadsResponse.threads || [];

            const stacks: Array<Array<{ function: string; filename: string; row: number; column: number }>> = [];

            // Request the call stack for each thread
            // Specification of Thread type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_Thread
            // Specification of StackFrame type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_StackFrame

            for (const thread of threads) {

                // https://microsoft.github.io/debug-adapter-protocol/specification#Requests_StackTrace
                const stackTraceResponse = await session.customRequest('stackTrace', {
                    threadId: thread.id,
                    startFrame: 0,
                    levels: 200 // Adjust the stack depth if needed
                });

                // Specification of StackFrame type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_StackFrame
                // Specification of Source type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_Source
                const frames = stackTraceResponse.stackFrames || [];

                // Build the path for the merger (sequence of frames)
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

            // Load and use the merger (WASM) within the extension to obtain DOT
            let dot = await getDotFromMerger(context, stacks);

            // Open a new tab (Webview) purely to render the SVG generated from DOT
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

            // Render to SVG
            const svg = viz.renderString(dot, { format: "svg" });

            const svgPanZoomUri = vscode.Uri.joinPath(context.extensionUri, 'media', 'svg-pan-zoom.min.js');
            const svgPanZoomWebviewUri = panel.webview.asWebviewUri(svgPanZoomUri).toString();
            const svgPanZoomFileUri = vscode.Uri.file(svgPanZoomUri.fsPath).toString();

            const html = await buildWebviewHtml(context, svg, svgPanZoomWebviewUri);
            panel.webview.html = html;
            await persistWebviewHtml(html, [[svgPanZoomWebviewUri, svgPanZoomFileUri]]);
        } catch (err: any) {
            vscode.window.showErrorMessage(`Failed to fetch stacks: ${err.message || err}`);
        }
    });

    context.subscriptions.push(disposable);
}

// This method is called when your extension is deactivated
export function deactivate() {}

async function persistWebviewHtml(html: string, replacements: Array<[string, string]> = []): Promise<void> {
    const filePath = path.join(os.tmpdir(), `parallel-stacks-webview-${Date.now()}.html`);
    try {
        const normalizedHtml = applyReplacements(html, replacements);
        await fs.writeFile(filePath, normalizedHtml, 'utf8');
        console.log(`Parallel Stacks webview HTML saved to ${filePath}`);
    } catch (error) {
        console.error('Failed to save Parallel Stacks webview HTML:', error);
    }
}

async function buildWebviewHtml(
    context: vscode.ExtensionContext,
    svgContent: string,
    svgPanZoomUri: string
): Promise<string> {
    const template = await getWebviewTemplate(context);
    return applyReplacements(template, [
        ['{{SVG_CONTENT}}', svgContent],
        ['{{SVG_PAN_ZOOM_URI}}', svgPanZoomUri]
    ]);
}

// ======== Merger (WASM) loading ========
let cachedMergerModule: any | null = null;
let cachedWebviewTemplate: string | null = null;

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

async function getWebviewTemplate(context: vscode.ExtensionContext): Promise<string> {
    if (cachedWebviewTemplate !== null) {
        return cachedWebviewTemplate;
    }

    const templatePath = path.join(context.extensionPath, 'media', 'webview.html');
    cachedWebviewTemplate = await fs.readFile(templatePath, 'utf8');
    return cachedWebviewTemplate;
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

function applyReplacements(value: string, replacements: Array<[string, string]>): string {
    return replacements.reduce((acc, [from, to]) => acc.split(from).join(to), value);
}
