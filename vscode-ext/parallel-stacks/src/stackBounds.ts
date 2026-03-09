export type StackBounds = {
    startFrame: number;
    levels: number;
};

export type CalculateStackBoundsArgs = {
    totalFrames: unknown;
    depthLimit: number;
};

export function calculateStackBounds({
    totalFrames,
    depthLimit
}: CalculateStackBoundsArgs): StackBounds {
    const safeDepthLimit = Math.max(0, Math.trunc(depthLimit));
    if (safeDepthLimit === 0) {
        return {
            startFrame: 0,
            levels: 0
        };
    }

    const numericTotalFrames = Number(totalFrames);
    if (!Number.isFinite(numericTotalFrames)) {
        return {
            startFrame: 0,
            levels: 0
        };
    }

    const safeTotalFrames = Math.max(0, Math.trunc(numericTotalFrames));

    const levels = Math.min(safeDepthLimit, safeTotalFrames);
    return {
        startFrame: Math.max(0, safeTotalFrames - levels),
        levels
    };
}
