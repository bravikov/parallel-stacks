// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import * as path from 'path';
import { promises as fs } from 'fs';
import * as os from 'os';
import createMerger from '../media/merger';
import { calculateStackBounds } from './stackBounds';

const LAST_SAVE_DIR_KEY = 'parallelStacks.lastSaveDir';
const MAX_STACK_DEPTH_LIMIT = 1_000_000_000;

type MergerModule = Awaited<ReturnType<typeof createMerger>>;

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {
    let webviewCounter = 0;
    let lastSaveDir: string | null = context.globalState.get<string>(LAST_SAVE_DIR_KEY) ?? null;

    // Use the console to output diagnostic information (console.log) and errors (console.error)
    // This line of code will only be executed once when your extension is activated
    console.log('Congratulations, your extension "parallel-stacks" is now active!');

    let Merger: MergerModule | null = null;

    // Command to display thread stacks in a new tab (Webview)
    const disposable = vscode.commands.registerCommand('parallel-stacks.show', async () => {
        if (!Merger) {
            Merger = await createMerger();
        }

        const session = vscode.debug.activeDebugSession;
        if (!session) {
            vscode.window.showWarningMessage('No active debug session.');
            return;
        }

        const configuration = vscode.workspace.getConfiguration();
        const configuredDepthLimit = configuration.get<number>('parallelStacks.stackDepthLimit') ?? 200;
        const depthLimit = Number.isFinite(configuredDepthLimit) && configuredDepthLimit >= 0
            ? Math.min(Math.trunc(configuredDepthLimit), MAX_STACK_DEPTH_LIMIT)
            : 200;

        try {
            // Request thread information
            // https://microsoft.github.io/debug-adapter-protocol/specification#Requests_Threads
            const threadsResponse = await session.customRequest('threads');
            const threads = threadsResponse.threads || [];

            const stacks = new Merger.VectorVectorFrame();

            let dot = 'digraph { }';
            try {
                // Request the call stack for each thread
                // Specification of Thread type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_Thread
                // Specification of StackFrame type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_StackFrame
                for (const thread of threads) {
                    // Probe with one frame to discover totalFrames (if the debug adapter provides it).
                    // https://microsoft.github.io/debug-adapter-protocol/specification#Requests_StackTrace
                    const probeStackTraceResponse = await session.customRequest('stackTrace', {
                        threadId: thread.id,
                        startFrame: 0,
                        levels: 1
                    });

                    const { startFrame, levels } = calculateStackBounds({
                        totalFrames: probeStackTraceResponse?.totalFrames,
                        depthLimit: depthLimit
                    });

                    // TODO: limit by Merger.merge_to_graphviz_dot() when totalFrames is unavailable.
                    const stackTraceResponse = await session.customRequest('stackTrace', {
                        threadId: thread.id,
                        startFrame: startFrame,
                        levels: levels
                    });
                    const frames = stackTraceResponse.stackFrames || [];

                    // Specification of StackFrame type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_StackFrame
                    // Specification of Source type: https://microsoft.github.io/debug-adapter-protocol/specification#Types_Source
                    const stack = new Merger.VectorFrame();

                    for (const frame of frames) {
                        const mergerFrame = new Merger.Frame();
                        try {
                            mergerFrame.function = String(frame.name || '');
                            mergerFrame.filename = frame.source?.path
                                ? path.basename(frame.source.path)
                                : String(frame.source?.name || '');
                            mergerFrame.row = Number(frame.line || 0);
                            mergerFrame.column = Number(frame.column || 0);
                            stack.push_back(mergerFrame);
                        } finally {
                            mergerFrame.delete();
                        }
                    }

                    if (stack.size() > 0) {
                        stacks.push_back(stack);
                    }
                    stack.delete();
                }

                dot = Merger.merge_to_graphviz_dot(stacks) || dot;
            } catch (mergeError: any) {
                console.error('merge_to_graphviz_dot failed:', mergeError);
                dot = 'digraph { label="merge_to_graphviz_dot failed" }';
            } finally {
                stacks.delete();
            }

            const tabIndex = ++webviewCounter;
            const panelTitle = `Parallel Stacks ${tabIndex}`;
            // Open a new tab (Webview) purely to render the SVG generated from DOT
            const panel = vscode.window.createWebviewPanel(
                'parallelStacks',
                panelTitle,
                vscode.ViewColumn.One,
                {
                    enableScripts: true,
                }
            );
            const panelIcon = vscode.Uri.joinPath(context.extensionUri, 'images', 'icon.png');
            panel.iconPath = { light: panelIcon, dark: panelIcon };

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

            panel.webview.onDidReceiveMessage(async (message) => {
                if (message?.type === 'saveSvg') {
                    const updatedDir = await handleSaveSvg(context, svg, lastSaveDir, tabIndex);
                    if (updatedDir) {
                        lastSaveDir = updatedDir;
                    }
                }
            });
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

let cachedWebviewTemplate: string | null = null;

async function getWebviewTemplate(context: vscode.ExtensionContext): Promise<string> {
    if (cachedWebviewTemplate !== null) {
        return cachedWebviewTemplate;
    }

    const templatePath = path.join(context.extensionPath, 'media', 'webview.html');
    cachedWebviewTemplate = await fs.readFile(templatePath, 'utf8');
    return cachedWebviewTemplate;
}

function applyReplacements(value: string, replacements: Array<[string, string]>): string {
    return replacements.reduce((acc, [from, to]) => acc.split(from).join(to), value);
}

async function handleSaveSvg(
    context: vscode.ExtensionContext,
    svgContent: string,
    previousDir: string | null,
    tabIndex: number
): Promise<string | null> {
    const defaultDir = previousDir || vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || os.homedir();
    const defaultFileName = `parallel-stacks-${tabIndex}.svg`;
    const defaultUri = vscode.Uri.file(path.join(defaultDir, defaultFileName));

    const targetUri = await vscode.window.showSaveDialog({
        defaultUri,
        filters: { 'SVG': ['svg'] },
        saveLabel: 'Save SVG'
    });

    if (!targetUri) {
        return previousDir;
    }

    try {
        const normalizedSvg = ensureXmlDeclaration(svgContent);
        await fs.writeFile(targetUri.fsPath, normalizedSvg, 'utf8');
        const targetDir = path.dirname(targetUri.fsPath);
        await context.globalState.update(LAST_SAVE_DIR_KEY, targetDir);
        vscode.window.showInformationMessage(`Parallel Stacks SVG saved to ${targetUri.fsPath}`);
        return targetDir;
    } catch (error: any) {
        vscode.window.showErrorMessage(`Failed to save SVG: ${error?.message || error}`);
        return null;
    }
}

function ensureXmlDeclaration(svgContent: string): string {
    const trimmed = svgContent.trimStart();
    const hasXmlDeclaration = trimmed.startsWith('<?xml');
    return hasXmlDeclaration ? svgContent : `<?xml version="1.0" encoding="UTF-8"?>\n${svgContent}`;
}
