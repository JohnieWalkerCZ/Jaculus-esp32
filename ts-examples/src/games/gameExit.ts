import * as gpio from "gpio";
import { EXIT_PIN } from '../pins.js';

// Central exit-button handling. Games call running() each frame; the pin is
// configured lazily on first use so a game works standalone without the menu.
let configured = false;

// The menu calls useMenu() so games it launches don't also auto-run on import.
// A game imported on its own (no menu) sees standalone() === true and self-runs.
let menuActive = false;
export function useMenu(): void { menuActive = true; }
export function standalone(): boolean { return !menuActive; }

/** True while the current game should keep running (exit button not pressed). */
export function running(): boolean {
    // Standalone: no menu to return to, so the exit button has nothing to do.
    // Keep playing instead of ending the program.
    if (standalone()) {
        return true;
    }
    if (!configured) {
        gpio.pinMode(EXIT_PIN, gpio.PinMode.INPUT_PULLUP);
        configured = true;
    }
    return gpio.read(EXIT_PIN) !== 0;
}
