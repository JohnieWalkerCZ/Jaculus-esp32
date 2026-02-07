declare module "shapes" {
	import { Texture } from "renderer";
	export type Color = [number, number, number, number] | "red" | "green" | "blue" | "yellow" | "magenta" | "cyan" | "white" | "black";

	export interface ShapeParams {
		x: number;
		y: number;
		color: Color;
		z?: number;
	}

	export class Shape {
		setPosition(x: number, y: number): void;
		translate(dx: number, dy: number): void;
		rotate(angle: number): void;
		setPivot(x: number, y: number): void;
		setScale(scaleX: number, scaleY: number, originX?: number, originY?: number): void;
		setColor(color: Color): void;
		setZ(z: number): void;
		setX(x: number): void;
		setY(y: number): void;
		setTexture(texture: Texture): void;
		setFixTexture(fixed: boolean): void;
		setTextureRotation(rotation: number): void;
		setTextureOffset(offsetX: number, offsetY: number): void;
		setTextureScale(scaleX: number, scaleY: number): void;
		setRotationAngle(angle: number): void;
		setScaleX(scaleX: number): void;
		setScaleY(scaleY: number): void;
		setUVScaleX(scaleX: number): void;
		setUVScaleY(scaleY: number): void;
		setUVOffsetX(offsetX: number): void;
		setUVOffsetY(offsetY: number): void;
		setUVRotation(rotation: number): void;
		getX(): number;
		getY(): number;
		getZ(): number;
		getColor(): Color;
		getRotationAngle(): number;
		getScaleX(): number;
		getScaleY(): number;
		intersects(other: Shape): boolean;
		addCollider(collider?: any): void;
		removeCollider(): void;
	}

	export class Collection extends Shape {
		constructor(params: ShapeParams);
		add(shape: Shape): void;
	}

	export interface CircleParams extends ShapeParams {
		radius: number;
		fill?: boolean;
	}

	export class Circle extends Shape {
		constructor(params: CircleParams);
	}

	export interface RectangleParams extends ShapeParams {
		width: number;
		height: number;
		fill?: boolean;
	}

	export class Rectangle extends Shape {
		constructor(params: RectangleParams);
	}

	export interface PolygonParams extends ShapeParams {
		vertices: [number, number][];
		fill?: boolean;
	}

	export class Polygon extends Shape {
		constructor(params: PolygonParams);
	}

	export interface LineSegmentParams extends ShapeParams {
		x2: number;
		y2: number;
	}

	export class LineSegment extends Shape {
		constructor(params: LineSegmentParams);
	}

	export class Point extends Shape {
		constructor(params: ShapeParams);
	}

	export interface RegularPolygonRadiusParams extends ShapeParams {
		sides: number;
		radius: number;
		fill?: boolean;
	}

	export interface RegularPolygonSideParams extends ShapeParams {
		sides: number;
		sideLength: number;
		fill?: boolean;
	}

	export class RegularPolygon extends Shape {
		constructor(params: RegularPolygonRadiusParams | RegularPolygonSideParams);
	}
}

declare module "renderer" {
	import { Collection, Color } from "shapes";

	export class FrameBuffer {
		constructor(width: number, height: number);
		clear(): void;
	}
	export class Texture {
		constructor();
		load(path: string): boolean;
		setWrapMode(mode: "repeat" | "clamp" | "mirror"): void;
		getWidth(): number;
		getHeight(): number;
		isValid(): number;
	}
	export class Font {
		constructor();
		getHeight(): number;
		getCharWidth(char: string): number;
		getCharSpacing(char: string): number;
	}

	export class Renderer {
		constructor(width: number, height: number);
		render(scene: Collection, buffer: FrameBuffer): void;
		drawText(buffer: FrameBuffer, text: string, x: number, y: number, font: Font, color: Color, wrap: boolean): void;
	}
}
/*
*/
