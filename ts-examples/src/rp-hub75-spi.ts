import { Renderer } from 'renderer';
import { Collection, Circle, Rectangle, LineSegment } from 'shapes';
import { SPI1 } from 'spi';
import { Format } from './constants.js';

const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;
const MAX_PIXELS = PANEL_WIDTH * PANEL_HEIGHT;
const BUFFER_SIZE_BYTES = MAX_PIXELS * 3;

const PIN_SCK = 13;
const PIN_MOSI = 11;
const PIN_CS = 10;
const SPI_BAUD = 2_000_000;
const SPI_MODE = 0;
const MODE_MAGIC = 0xceda2083;

const SYNC_WORDS = [
    0xac92,
    0x3bca,
    0x41bf,
    0x393d,
    0xa74a,
    0xae01,
    0x155d,
    0xfb70,
];

function setupSpi() {
    SPI1.setup({
        sck: PIN_SCK,
        mosi: PIN_MOSI,
        baud: SPI_BAUD,
        mode: SPI_MODE,
        order: 'msb',
    });
}

function buildSyncBuffer() {
    const buffer = new Uint8Array(SYNC_WORDS.length * 2);
    const view = new DataView(buffer.buffer);

    for (let i = 0; i < SYNC_WORDS.length; i++) {
        view.setUint16(i * 2, SYNC_WORDS[i], true);
    }

    return buffer;
}

function buildModesetBuffer() {
    const buffer = new Uint8Array(12);
    const view = new DataView(buffer.buffer);

    view.setUint8(0, Format.RGB_888);
    view.setUint8(1, 0);
    view.setUint16(2, PANEL_WIDTH, true);
    view.setUint16(4, PANEL_HEIGHT, true);
    view.setUint32(8, MODE_MAGIC, true);

    return buffer;
}

function sendRpHub75Frame(syncBuffer: Uint8Array, modesetBuffer: Uint8Array, frameBuffer: ArrayBuffer, frameSize: number) {
    SPI1.write(syncBuffer, PIN_CS);
    SPI1.write(modesetBuffer, PIN_CS);
    SPI1.write(new Uint8Array(frameBuffer, 0, frameSize), PIN_CS);
}

export async function rpHub75SpiExample() {
    setupSpi();

    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const renderBuffer = new ArrayBuffer(BUFFER_SIZE_BYTES);
    const syncBuffer = buildSyncBuffer();
    const modesetBuffer = buildModesetBuffer();

    const scene = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1] });
    const background = new Rectangle({ x: 0, y: 0, width: PANEL_WIDTH, height: PANEL_HEIGHT, color: [10, 10, 30, 1], fill: true });
    const sun = new Circle({ x: 18, y: 18, radius: 8, color: [255, 180, 0, 1], fill: true });
    const planet = new Circle({ x: 48, y: 32, radius: 6, color: [0, 180, 255, 1], fill: true });
    const trail = new LineSegment({ x: 18, y: 18, x2: 48, y2: 32, color: [255, 255, 255, 1] });
    const marker = new Rectangle({ x: 28, y: 46, width: 8, height: 8, color: [255, 0, 120, 1], fill: true });

    scene.add(background);
    scene.add(trail);
    scene.add(sun);
    scene.add(planet);
    scene.add(marker);

    let angle = 0;

    console.log(`Starting RP-HUB75 SPI loop (${syncBuffer.length} sync bytes + ${modesetBuffer.length} modeset bytes + rendered RGB888 frames)...`);

    while (true) {
        const orbitX = Math.cos(angle) * 18;
        const orbitY = Math.sin(angle) * 18;

        planet.setPosition(32 + orbitX, 32 + orbitY);
        trail.setPosition(18, 18);
        marker.setPosition(28 + Math.cos(angle * 0.5) * 10, 46 + Math.sin(angle * 0.5) * 6);

        const bytesWritten = renderer.render(scene, renderBuffer, true, Format.RGB_888);
        sendRpHub75Frame(syncBuffer, modesetBuffer, renderBuffer, bytesWritten);

        angle += 0.15;
        await sleep(33);
    }
}

rpHub75SpiExample();
