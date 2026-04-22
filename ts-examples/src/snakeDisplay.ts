import { Renderer } from 'renderer';
import { Collection, Rectangle } from 'shapes';
import { SPI2 } from 'spi';
import { Format } from './constants.js';
import * as gpio from 'gpio';
import * as adc from "adc";

// --- CONFIGURATION ---
const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;
const GRID_SIZE = 16;
const CELL_SIZE = PANEL_WIDTH / GRID_SIZE; // 4 pixels per grid unit

const PIN_SCK = 3;
const PIN_CS = 1;
const SPI_BAUD = 2_000_000;
const SPI_MODE = 0;
const MODE_MAGIC = 0xfb;

const SYNC_WORDS = [
    0xac92, 0x3bca, 0x41bf, 0x393d, 0xa74a, 0xae01, 0x155d, 0xfb70, 0xf681, 0x2f6d, 0x4931, 0x0fa3, 0x77bf, 0xd756, 0x26f9, 0x4eb6,
];

const ADC_X = 4;
const ADC_Y = 5;
adc.configure(ADC_X);
adc.configure(ADC_Y);

const DEADZONE = 300; // Threshold from center (512) to register movement

function sendRpHub75Frame(syncBuffer: Uint8Array, modesetBuffer: Uint8Array, frameBuffer: ArrayBuffer) {
    SPI2.transfer(syncBuffer, PIN_CS, 0, true);
    SPI2.transfer(modesetBuffer, PIN_CS, 0, true);

    SPI2.transfer(frameBuffer, PIN_CS, 0, true);
}

function getJoystickDirection(currentDir) {
    const xVal = adc.read(ADC_X);
    const yVal = adc.read(ADC_Y);

    // Calculate displacement from center (512)
    const dx = xVal - 512;
    const dy = yVal - 512;

    // Determine which axis has the stronger input
    if (Math.abs(dx) > DEADZONE || Math.abs(dy) > DEADZONE) {
        if (Math.abs(dx) > Math.abs(dy)) {
            if (dx > 0 && currentDir.y === 0) return { x: 0, y: -1 };
            if (dx < 0 && currentDir.y === 0) return { x: 0, y: 1 };
        } else {
            if (dy > 0 && currentDir.x === 0) return { x: 1, y: 0 };
            if (dy < 0 && currentDir.x === 0) return { x: -1, y: 0 };
        }
    }
    return currentDir; // Return original direction if no input
}

function setupSpi() {
    SPI2.setup({
        sck: PIN_SCK,
        data0: 38,
        data1: 39,
        data2: 41,
        data3: 45,
        baud: SPI_BAUD,
        mode: SPI_MODE,
        order: 'lsb',
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
    const buffer = new Uint8Array(8);
    const view = new DataView(buffer.buffer);

    view.setUint8(0, MODE_MAGIC);
    view.setUint8(1, 0);
    view.setUint8(2, Format.RGB_565_LITTLE);
    view.setUint8(3, 255);
    view.setUint16(4, PANEL_WIDTH, true);

    return buffer;
}

// --- GAME LOGIC ---
let snake = [{ x: 8, y: 8 }, { x: 7, y: 8 }, { x: 6, y: 8 }];
let direction = { x: 1, y: 0 };
let food = { x: 4, y: 4 };

function newFood() {
    return {
        x: Math.floor(Math.random() * GRID_SIZE),
        y: Math.floor(Math.random() * GRID_SIZE)
    };
}

async function runSnake() {
    setupSpi();
    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const renderBuffer = new ArrayBuffer(PANEL_WIDTH * PANEL_HEIGHT * 2);
    const syncBuffer = buildSyncBuffer();
    const modesetBuffer = buildModesetBuffer();

    const FRAME_TIME = 75;   // The speed of the snake
    const POLL_INTERVAL = 25; // Poll the joystick every 25ms

    while (true) {
        // --- 1. Update Logic ---
        let next = { x: snake[0].x + direction.x, y: snake[0].y + direction.y };

        // Wrap around
        next.x = (next.x + GRID_SIZE) % GRID_SIZE;
        next.y = (next.y + GRID_SIZE) % GRID_SIZE;

        // Collision Check (Self)
        if (snake.find(p => p.x === next.x && p.y === next.y)) {
            snake = [{ x: 8, y: 8 }, { x: 7, y: 8 }, { x: 6, y: 8 }];
        } else {
            snake.unshift(next);
            if (next.x === food.x && next.y === food.y) {
                food = newFood();
            } else {
                snake.pop();
            }
        }

        // --- 2. Render Frame ---
        const scene = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1] });

        for (let pos of snake) {
            scene.add(new Rectangle({
                x: pos.x * CELL_SIZE,
                y: pos.y * CELL_SIZE,
                width: CELL_SIZE - 1,
                height: CELL_SIZE - 1,
                color: [0, 255, 0, 1],
                fill: true
            }));
        }

        scene.add(new Rectangle({
            x: food.x * CELL_SIZE,
            y: food.y * CELL_SIZE,
            width: CELL_SIZE,
            height: CELL_SIZE,
            color: [255, 0, 0, 1],
            fill: true
        }));

        renderer.render(scene, renderBuffer, true, Format.RGB_565_LITTLE);
        sendRpHub75Frame(syncBuffer, modesetBuffer, renderBuffer);

        // --- 3. Responsive Waiting (Polling Phase) ---
        let elapsed = 0;
        while (elapsed < FRAME_TIME) {
            direction = getJoystickDirection(direction);
            await sleep(POLL_INTERVAL);
            elapsed += POLL_INTERVAL;
        }
    }
}

runSnake();
