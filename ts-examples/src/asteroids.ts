import { Renderer } from 'renderer';
import { Collection, Polygon, Circle, RegularPolygon } from 'shapes';
import { SPI2 } from 'spi';
import { Format } from './constants.js';
import * as adc from "adc";

// --- CONFIGURATION ---
const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;

// --- SPI SETUP (Boilerplate) ---
const PIN_SCK = 3;
const PIN_CS = 1;
const SPI_BAUD = 2_000_000;
const SPI_MODE = 0;
const MODE_MAGIC = 0xfb;
const SYNC_WORDS = [0xac92, 0x3bca, 0x41bf, 0x393d, 0xa74a, 0xae01, 0x155d, 0xfb70, 0xf681, 0x2f6d, 0x4931, 0x0fa3, 0x77bf, 0xd756, 0x26f9, 0x4eb6];

// --- HARDWARE CONFIG ---
const MOVE_X = 4;
const MOVE_Y = 5;
const AIM_X = 7;
const AIM_Y = 6;

adc.configure(MOVE_X);
adc.configure(MOVE_Y);
adc.configure(AIM_X);
adc.configure(AIM_Y);

const CENTER = 512;
const DEADZONE = 200;

// --- INPUT UTILITIES ---
function getStickVector(pinX, pinY, swapped) {
    const rawX = adc.read(pinX) - CENTER;
    const rawY = adc.read(pinY) - CENTER;

    if (Math.abs(rawX) < DEADZONE && Math.abs(rawY) < DEADZONE) {
        return { x: 0, y: 0, active: false };
    }

    // Normalize the vector so diagonal movement isn't faster
    const mag = Math.sqrt(rawX * rawX + rawY * rawY);
    return {
        x: (swapped ? -1 : 1) * rawY / mag,
        y: (swapped ? -1 : 1) * -rawX / mag,
        active: true
    };
}

function setupSpi() {
    SPI2.setup({ sck: PIN_SCK, data0: 38, data1: 39, data2: 41, data3: 45, baud: SPI_BAUD, mode: SPI_MODE, order: 'lsb' });
}

function buildSyncBuffer() {
    const buffer = new Uint8Array(SYNC_WORDS.length * 2);
    const view = new DataView(buffer.buffer);
    for (let i = 0; i < SYNC_WORDS.length; i++) view.setUint16(i * 2, SYNC_WORDS[i], true);
    return buffer;
}

function buildModesetBuffer() {
    const buffer = new Uint8Array(8);
    const view = new DataView(buffer.buffer);
    view.setUint8(0, MODE_MAGIC); view.setUint8(2, Format.RGB_888); view.setUint16(4, PANEL_WIDTH, true);
    return buffer;
}

// --- GAME LOGIC ---
let player = { x: 32, y: 32, angle: 0, speed: 2 };
let bullets = [];
let asteroids = [];
let fireCooldown = 0;

function spawnAsteroid() {
    // Spawn randomly on the edges
    const edge = Math.floor(Math.random() * 4);
    let ax, ay;
    if (edge === 0) { ax = Math.random() * 64; ay = 0; }
    else if (edge === 1) { ax = 64; ay = Math.random() * 64; }
    else if (edge === 2) { ax = Math.random() * 64; ay = 64; }
    else { ax = 0; ay = Math.random() * 64; }

    asteroids.push({
        x: ax,
        y: ay,
        vx: (Math.random() - 0.5) * 1.5,
        vy: (Math.random() - 0.5) * 1.5,
        r: 4 + Math.random() * 4
    });
}

function resetGame() {
    player = { x: 32, y: 32, angle: 0, speed: 2 };
    bullets = [];
    asteroids = [];
    for (let i = 0; i < 4; i++) spawnAsteroid();
}

async function runAsteroids() {
    setupSpi();
    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const renderBuffer = new ArrayBuffer(PANEL_WIDTH * PANEL_HEIGHT * 3);
    const syncBuffer = buildSyncBuffer();
    const modesetBuffer = buildModesetBuffer();

    const FRAME_TIME = 11;   // ~30 FPS
    const POLL_INTERVAL = 11; // Poll inputs 3x per frame

    resetGame();

    while (true) {
        let moveInput = { x: 0, y: 0, active: false };
        let aimInput = { x: 0, y: 0, active: false };

        // --- 1. Responsive Input Polling ---
        let elapsed = 0;
        while (elapsed < FRAME_TIME) {
            let m = getStickVector(MOVE_X, MOVE_Y, false);
            let a = getStickVector(AIM_X, AIM_Y, true);
            // Latch the input if it was active at any point during polling
            if (m.active) moveInput = m;
            if (a.active) aimInput = a;

            await sleep(POLL_INTERVAL);
            elapsed += POLL_INTERVAL;
        }

        // --- 2. Update Logic ---

        // Player Movement
        if (moveInput.active) {
            player.x += moveInput.x * player.speed;
            player.y += moveInput.y * player.speed;

            player.angle = Math.atan2(-moveInput.y, moveInput.x) * 180 / Math.PI;
        }

        // Player Aim & Shooting
        if (aimInput.active) {
            player.angle = Math.atan2(-aimInput.y, aimInput.x) * 180 / Math.PI;

            if (fireCooldown <= 0) {
                bullets.push({
                    x: player.x,
                    y: player.y,
                    vx: aimInput.x * 5,
                    vy: aimInput.y * 5,
                    life: 20 // Frames until bullet despawns
                });
                fireCooldown = 6; // Fire rate delay
            }
        }
        if (fireCooldown > 0) fireCooldown--;

        // Screen Wrapping (Player)
        player.x = (player.x + PANEL_WIDTH) % PANEL_WIDTH;
        player.y = (player.y + PANEL_HEIGHT) % PANEL_HEIGHT;

        // Update Bullets
        for (let i = bullets.length - 1; i >= 0; i--) {
            let b = bullets[i];
            b.x += b.vx;
            b.y += b.vy;
            b.life--;

            // Screen Wrapping (Bullets)
            // b.x = (b.x + PANEL_WIDTH) % PANEL_WIDTH;
            // b.y = (b.y + PANEL_HEIGHT) % PANEL_HEIGHT;
            if (b.x < 0 || b.x >= PANEL_WIDTH || b.y < 0 || b.y >= PANEL_HEIGHT) {
                b.life = 0;
            }

            if (b.life <= 0) bullets.splice(i, 1);
        }

        // Update Asteroids & Collisions
        for (let i = asteroids.length - 1; i >= 0; i--) {
            let a = asteroids[i];
            a.x += a.vx;
            a.y += a.vy;

            // Screen Wrapping (Asteroids)
            a.x = (a.x + PANEL_WIDTH) % PANEL_WIDTH;
            a.y = (a.y + PANEL_HEIGHT) % PANEL_HEIGHT;

            // Player Collision (Death)
            const dxP = player.x - a.x;
            const dyP = player.y - a.y;
            if (Math.sqrt(dxP * dxP + dyP * dyP) < a.r + 2) {
                resetGame();
                break;
            }

            // Bullet Collision
            for (let j = bullets.length - 1; j >= 0; j--) {
                let b = bullets[j];
                const dx = b.x - a.x;
                const dy = b.y - a.y;
                if (Math.sqrt(dx * dx + dy * dy) < a.r + 2) {
                    asteroids.splice(i, 1);
                    bullets.splice(j, 1);
                    spawnAsteroid(); // Spawn a new one to keep the game going
                    break;
                }
            }
        }

        // --- 3. Render Frame ---
        const scene = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1] });

        // Draw Asteroids (Hexagons)
        for (let a of asteroids) {
            scene.add(new RegularPolygon({
                x: a.x, y: a.y,
                sides: 6, radius: a.r,
                color: [100, 100, 255, 1], // Light Blue
                fill: false,
            }));
        }

        // Draw Bullets (Tiny circles)
        for (let b of bullets) {
            scene.add(new Circle({
                x: b.x, y: b.y,
                radius: 1,
                color: [255, 255, 0, 1], // Yellow
                fill: true
            }));
        }

        // Draw Player (Triangle/Polygon)
        // A simple triangle pointing "Right" by default
        const ship = new Polygon({
            x: player.x,
            y: player.y,
            vertices: [[3, 0], [-2, -2], [-2, 2]],
            color: [0, 255, 0, 1], // Green
            fill: true
        });

        ship.setRotationAngle(player.angle);
        scene.add(ship);

        // Push to display
        renderer.render(scene, renderBuffer, true, Format.RGB_888);
        SPI2.transfer(syncBuffer, PIN_CS, 0, true);
        SPI2.transfer(modesetBuffer, PIN_CS, 0, true);
        SPI2.transfer(renderBuffer, PIN_CS, 0, true);
    }
}

runAsteroids();
