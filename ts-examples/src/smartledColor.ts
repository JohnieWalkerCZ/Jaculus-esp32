export function rgb(r: number, g: number, b: number): number {
    const red = r & 0xFF;
    const green = g & 0xFF;
    const blue = b & 0xFF;
    return (red << 16) | (green << 8) | blue;
}
