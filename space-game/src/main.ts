import * as THREE from 'three';
import { EventBus } from './core/EventBus';
import { GameLoop } from './core/GameLoop';
import { SceneManager } from './core/SceneManager';
import { MainMenuScene } from './ui/MainMenuScene';
import { EditorScene } from './editor/EditorScene';
// Flug-Szene (inkl. Rapier-WASM) lazy laden, damit das Menü schnell startet.

const canvas = document.querySelector<HTMLCanvasElement>('#view');
const ui = document.querySelector<HTMLElement>('#ui');
if (!canvas || !ui) throw new Error('#view / #ui fehlt in index.html');

const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.shadowMap.enabled = true;

const bus = new EventBus();
const scenes = new SceneManager(bus, () => ({ renderer, ui, bus }))
  .register('menu', () => new MainMenuScene())
  .register('editor', () => new EditorScene())
  .register('flight', async () => new (await import('./flight/FlightScene')).FlightScene());

function resize(): void {
  const w = window.innerWidth;
  const h = window.innerHeight;
  renderer.setSize(w, h, false);
  scenes.resize(w, h);
}
window.addEventListener('resize', resize);

bus.on('scene:change', ({ to }) => {
  document.body.dataset.activeScene = to;
  resize();
});

const loop = new GameLoop({
  fixedUpdate: (dt) => scenes.fixedUpdate(dt),
  render: (alpha, frameDt) => scenes.render(alpha, frameDt),
});

resize();
await scenes.goTo('menu');
loop.start();

// Debug-Zugriff für Konsole und Playwright.
declare global {
  interface Window {
    orbitwerk?: { bus: EventBus; scenes: SceneManager; loop: GameLoop };
  }
}
window.orbitwerk = { bus, scenes, loop };
