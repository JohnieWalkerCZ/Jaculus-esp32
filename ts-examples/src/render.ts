import { Hub75 } from 'hub75';
import { Font, Renderer, Texture } from 'renderer';
import { Format } from './constants.js';
import {
    Collection,
    Rectangle,
    Circle,
    RegularPolygon,
    Polygon,
    Point,
    LineSegment,
    Color
} from 'shapes';

const PANEL_WIDTH = 64;
const PANEL_HEIGHT = 64;

const MAX_PIXELS = PANEL_WIDTH * PANEL_HEIGHT;
const BUFFER_SIZE_BYTES = MAX_PIXELS * 9;

export async function shapeExample() {
    const display = new Hub75(PANEL_WIDTH, PANEL_HEIGHT);
    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const buffer = new ArrayBuffer(BUFFER_SIZE_BYTES);

    // 2. Wait for Hardware
    while (!display.isInitialized()) {
        await sleep(100);
    }

    // 3. Construct the Scene
    const scene = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1] });

    const line1 = new LineSegment({
        x: 0, y: 20,
        x2: 63, y2: 35,
        color: [255, 255, 255, 1]
    });
    scene.add(line1);

    // --- Red Rectangle (Top Left) ---
    const rect = new Rectangle({
        x: 6, y: 6,
        width: 6, height: 6,
        color: [255, 0, 0, 1],
        fill: true
    });
    scene.add(rect);

    // --- Blue Circle (Top Left) ---
    const circle = new Circle({
        x: 18, y: 6,
        radius: 5,
        color: [0, 0, 255, 1],
        fill: true
    });
    scene.add(circle);

    // --- Green Hexagon (Bottom Right) ---
    const hexagon = new RegularPolygon({
        x: 48, y: 48,
        radius: 10,
        sides: 6,
        color: [0, 255, 0, 1],
        fill: true
    });
    scene.add(hexagon);

    // --- Yellow Pentagon (Top Right) ---
    const pentagon = new RegularPolygon({
        x: 48, y: 16,
        radius: 8,
        sides: 5,
        color: [255, 255, 0, 1],
        fill: true
    });
    scene.add(pentagon);

    // --- Magenta Polygon (Bottom Left) ---
    const polygon = new Polygon({
        x: 16, y: 48,
        color: [255, 0, 255, 1],
        vertices: [
            [0, 0],
            [10, 5],
            [5, 15],
            [0, 10]
        ],
        fill: true
    });
    scene.add(polygon);

    // --- Dark Green Line (Vertical-ish) ---
    const line2 = new LineSegment({
        x: 32, y: 0,
        x2: 20, y2: 40,
        color: [0, 100, 50, 1]
    });
    scene.add(line2);

    // --- Dark Red Rectangle (Center Left) ---
    const lineRect = new Rectangle({
        x: 25, y: 20,
        width: 8, height: 5,
        color: [155, 0, 0, 1],
        fill: true
    });
    scene.add(lineRect);

    // --- Cyan Point (Center) ---
    const point = new Point({
        x: 32, y: 42,
        color: [155, 255, 255, 1]
    });
    scene.add(point);

    // 4. Render Loop
    console.log("Starting Render Loop...");

    while (true) {
        // Render scene to buffer, capturing the exact amount of bytes written
        const bytesWritten = renderer.render(scene, buffer);

        display.setBuffer(buffer, bytesWritten, 10, true);

        await sleep(100);
    }
}

export async function solarSystemExample() {
    const display = new Hub75(PANEL_WIDTH, PANEL_HEIGHT);
    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const buffer = new ArrayBuffer(BUFFER_SIZE_BYTES);
    console.log("Initializing Display...");

    // 3. Construct the Scene
    const sunCollection = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1], z: 0 });
    sunCollection.setPivot(32, 32);

    const sun = new Circle({
        x: 32, y: 32,
        radius: 8,
        color: [255, 255, 0, 1],
        fill: true
    });
    sunCollection.add(sun);

    const earthCollection = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1], z: 5 });
    earthCollection.setPivot(32, 32);

    const earth = new Circle({
        x: 32 + 20, y: 32,
        radius: 4,
        color: [0, 150, 255, 1],
        fill: true,
        z: 10
    });
    earthCollection.add(earth);

    const alien = new Circle({
        x: 32 - 20, y: 32,
        radius: 4,
        color: [0, 255, 100, 1],
        fill: true,
        z: 10
    });
    earthCollection.add(alien);

    const moonCollection = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1], z: 10 });
    moonCollection.setPivot(52, 32);

    const moon = new Circle({
        x: 32 + 20 + 8, y: 32,
        radius: 2,
        color: [200, 200, 200, 1],
        fill: true,
        z: 10
    });
    moonCollection.add(moon);

    earthCollection.add(moonCollection);
    sunCollection.add(earthCollection);

    const earthOrbit = new Circle({
        x: 32, y: 32,
        radius: 20,
        color: [100, 100, 100, 1],
        fill: false,
        z: 1
    });
    sunCollection.add(earthOrbit);

    const moonOrbit = new Circle({
        x: 32 + 20, y: 32,
        radius: 8,
        color: [100, 100, 100, 1],
        fill: false,
        z: 1
    });
    earthCollection.add(moonOrbit);

    while (true) {
        earthCollection.rotate(1.5);
        moonCollection.rotate(3);

        const bytesWritten = renderer.render(sunCollection, buffer);

        display.setBuffer(buffer, bytesWritten, 10, true);
        await sleep(1);
    }
}

export async function collisionExample() {
    const display = new Hub75(PANEL_WIDTH, PANEL_HEIGHT);
    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const buffer = new ArrayBuffer(BUFFER_SIZE_BYTES);

    // Wait for Hardware
    while (!display.isInitialized()) {
        await sleep(100);
    }

    // Main Collection
    const mainCollection = new Collection({ x: 0, y: 0, color: [0, 0, 0, 1], z: 0 });

    const triangle = new RegularPolygon({
        x: 32, y: 60,
        radius: 4,
        sides: 3,
        color: [0, 255, 0, 1],
        fill: true
    });

    // Enemy (Red Rectangle) positioned at (20, 0)
    const enemy = new Rectangle({
        x: 20, y: 0,
        width: 10, height: 10,
        color: [255, 0, 0, 1],
        fill: true
    });
    // Add colliders
    triangle.addCollider();
    enemy.addCollider();

    mainCollection.add(triangle);
    mainCollection.add(enemy);

    console.log("Starting Collision Loop...");

    while (true) {
        // Move enemy down
        enemy.translate(0, 1);

        // Check collision
        if (triangle.intersects(enemy)) {
            console.log("Collision!");
            triangle.setColor([255, 0, 0, 1]); // Red on collision
            console.log(`Collision detected! Enemy Y: ${enemy.getY()}`);
        } else {
            triangle.setColor([0, 255, 0, 1]); // Green otherwise
        }

        const bytesWritten = renderer.render(mainCollection, buffer);
        display.setBuffer(buffer, bytesWritten, 10, true);

        await sleep(100);
    }
}

async function textExample() {
    const display = new Hub75(PANEL_WIDTH, PANEL_HEIGHT);
    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const buffer = new ArrayBuffer(BUFFER_SIZE_BYTES);
    const font = new Font();

    while (true) {
        let bytesWritten = 0;

        // 1. Draw "ABCDEFGHIJKLMNOPQRSTUVWXYZ" in Red
        bytesWritten = renderer.drawText(
            buffer,
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
            0, 0,
            font,
            [255, 0, 0, 1], // Red
            true // Wrap
        );

        // Push to display AND clear the previous frame (true)
        display.setBuffer(buffer, bytesWritten, 10, true);

        // 2. Draw Numbers in Blue
        bytesWritten = renderer.drawText(
            buffer,
            "0123456789",
            0, 48,
            font,
            [0, 0, 255, 1], // Blue
            true
        );
        // Push to display, but DO NOT clear the previous letters (false)
        display.setBuffer(buffer, bytesWritten, 10, false);

        // 3. Draw "abcdefghijklmnopqrstuvwxyz" in Green
        bytesWritten = renderer.drawText(
            buffer,
            "abcdefghijklmnopqrstuvwxyz",
            0, 24,
            font,
            [0, 255, 0, 1], // Green
            true
        );
        // Push to display, DO NOT clear (false)
        display.setBuffer(buffer, bytesWritten, 10, false);

        console.log("Displayed Alphabet and Numbers.");

        // Wait 1 second (1000ms)
        await sleep(1000);

        bytesWritten = renderer.drawText(
            buffer,
            "!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ ",
            0, 0,
            font,
            [255, 255, 0, 1.0], // Yellow
            true
        );
        console.log("Displaying Symbols...");
        display.setBuffer(buffer, bytesWritten, 10, true);
        await sleep(1000);
    }
}

async function textureExample() {
    const display = new Hub75(PANEL_WIDTH, PANEL_HEIGHT);
    const renderer = new Renderer(PANEL_WIDTH, PANEL_HEIGHT);
    const buffer = new ArrayBuffer(BUFFER_SIZE_BYTES);

    const texture = new Texture();
    const success = texture.load("/data/data/brick.bmp");
    if (success) {
        texture.setWrapMode("repeat");
        console.log(`Texture loaded: ${texture.getWidth()}x${texture.getHeight()}`);
    } else {
        console.log("Failed to load texture BMP");
    }

    const scene = new Collection({ x: 0, y: 0, color: "black" });

    const rect = new Rectangle({
        x: 0, y: 0,
        color: [255, 255, 255, 1],
        width: 64, height: 64,
        fill: true
    });
    rect.setTexture(texture);
    rect.setFixTexture(true);
    rect.setTextureScale(4.0, 4.0);
    rect.setPivot(32, 32);

    scene.add(rect);

    while (true) {

        const bytesWritten = renderer.render(scene, buffer, true, Format.RGB_888);
        console.log(bytesWritten)

        display.setBuffer(buffer, bytesWritten, Format.RGB_888, true);

        await sleep(500);
    }
}

async function main() {
    // Uncomment the example you want to run
    // await shapeExample();
    // await solarSystemExample();
    // await collisionExample();
    // await textExample();
    // await textureExample();
}

main().catch(console.error);
