declare module "hub75" {
	import { FrameBuffer } from "renderer";

	type Pixel = [number, number, number, number, number, number];
	type Pixels = Pixel[];

	class Hub75 {
		constructor(panelWidth?: number, panelHeight?: number, chainLength?: number);

		setBuffer(pixels: Pixels, clearPrevious?: boolean): void;
		setBufferDirect(buffer: FrameBuffer, clearPrevious?: boolean): void;
		clear(): void;
		setBrightness(brightness: number): void;
		isInitialized(): boolean;
	}
}
