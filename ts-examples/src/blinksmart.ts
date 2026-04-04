import { SmartLed } from "smartled";
import { rgb } from "./smartledColor.js";

/**
 * This example blinks using a smart LED on pin 48.
 */

const LED_PIN = 48;

let strip = new SmartLed(LED_PIN, 1);

let state = false;

setInterval(() => {
    if (state) {
        strip.set(0, rgb(0, 0, 0));
    }
    else {
        strip.set(0, rgb(20, 0, 0));
    }
    strip.show();

    state = !state;
}, 1000);
