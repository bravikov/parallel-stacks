import assert from 'node:assert/strict';
import test from 'node:test';
import * as path from 'path';
import { promises as fs } from 'fs';
import createMerger from '../../media/merger';

type MergerModule = Awaited<ReturnType<typeof createMerger>>;
let Merger!: MergerModule;

type FrameData = {
    function: string;
    filename: string;
    row: number;
    column: number;
};

function createStacks(input: FrameData[][]): InstanceType<MergerModule['VectorVectorFrame']> {
    const stacks = new Merger.VectorVectorFrame();
    for (const inputStack of input) {
        const stack = new Merger.VectorFrame();
        for (const inputFrame of inputStack) {
            const frame = new Merger.Frame();
            try {
                frame.function = inputFrame.function;
                frame.filename = inputFrame.filename;
                frame.row = inputFrame.row;
                frame.column = inputFrame.column;
                stack.push_back(frame);
            } finally {
                frame.delete();
            }
        }
        if (stack.size() > 0) {
            stacks.push_back(stack);
        }
        stack.delete();
    }
    return stacks;
}

test('merge_to_graphviz_dot should match expected .dot output', async () => {
    const expectedDotPath = path.resolve(__dirname, '../../../../threads-merger/tests_data/test-frame.dot');
    const expectedDot = await fs.readFile(expectedDotPath, 'utf8');

    Merger = await createMerger();

    const input: FrameData[][] = [
        [
            { function: 'func2', filename: 'file2.cpp', row: 20, column: 10 },
            { function: 'func1', filename: 'file1.cpp', row: 10, column: 5 }
        ],
        [
            { function: 'func3', filename: 'file3.cpp', row: 30, column: 15 },
            { function: 'func1', filename: 'file1.cpp', row: 10, column: 5 }
        ]
    ];

    const stacks = createStacks(input);

    try {
        const actualDot = Merger.merge_to_graphviz_dot(stacks);
        assert.strictEqual(actualDot, expectedDot);
    } finally {
        stacks.delete();
    }
});
