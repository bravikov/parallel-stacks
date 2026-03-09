import assert from 'node:assert/strict';
import test from 'node:test';
import { calculateStackBounds } from '../stackBounds';

test('calculateStackBounds returns unbounded request params when depthLimit is 0', () => {
    const bounds = calculateStackBounds({ totalFrames: 10, depthLimit: 0 });
    assert.deepStrictEqual(bounds, { startFrame: 0, levels: 0 });
});

test('calculateStackBounds returns last N frames when depthLimit is less than totalFrames', () => {
    const bounds = calculateStackBounds({ totalFrames: 10, depthLimit: 3 });
    assert.deepStrictEqual(bounds, { startFrame: 7, levels: 3 });
});

test('calculateStackBounds returns full stack when depthLimit exceeds totalFrames', () => {
    const bounds = calculateStackBounds({ totalFrames: 10, depthLimit: 100 });
    assert.deepStrictEqual(bounds, { startFrame: 0, levels: 10 });
});

test('calculateStackBounds normalizes non-integer values', () => {
    const bounds = calculateStackBounds({ totalFrames: 12.9, depthLimit: 2.8 });
    assert.deepStrictEqual(bounds, { startFrame: 10, levels: 2 });
});

test('calculateStackBounds clamps negative totalFrames to zero', () => {
    const bounds = calculateStackBounds({ totalFrames: -5, depthLimit: 3 });
    assert.deepStrictEqual(bounds, { startFrame: 0, levels: 0 });
});

test('calculateStackBounds treats negative depthLimit as unbounded request', () => {
    const bounds = calculateStackBounds({ totalFrames: 10, depthLimit: -2 });
    assert.deepStrictEqual(bounds, { startFrame: 0, levels: 0 });
});

test('calculateStackBounds returns full-stack request params when totalFrames is not finite', () => {
    const bounds = calculateStackBounds({ totalFrames: undefined, depthLimit: 3 });
    assert.deepStrictEqual(bounds, { startFrame: 0, levels: 0 });
});
