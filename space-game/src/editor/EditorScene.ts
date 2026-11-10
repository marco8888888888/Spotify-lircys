import * as THREE from 'three';
import type { GameScene, SceneContext } from '../core/Scene';
import { el } from '../ui/dom';
import { disposeScene } from '../ui/MainMenuScene';

/** Leere Hangar-Szene (Platzhalter für T20). */
export class EditorScene implements GameScene {
  readonly id = 'editor' as const;
  private scene = new THREE.Scene();
  private camera = new THREE.PerspectiveCamera(50, 1, 0.1, 500);
  private ctx!: SceneContext;

  enter(ctx: SceneContext): void {
    this.ctx = ctx;
    this.scene.background = new THREE.Color(0x10141c);
    const grid = new THREE.GridHelper(40, 40, 0xff8a3d, 0x2a3242);
    this.scene.add(grid);
    const pad = new THREE.Mesh(
      new THREE.CylinderGeometry(3, 3, 0.2, 48),
      new THREE.MeshStandardMaterial({ color: 0x3a4150, roughness: 0.7, metalness: 0.3 }),
    );
    pad.position.y = 0.1;
    this.scene.add(pad);
    const key = new THREE.DirectionalLight(0xffffff, 1.6);
    key.position.set(5, 10, 6);
    this.scene.add(key, new THREE.HemisphereLight(0x8899bb, 0x222222, 0.6));
    this.camera.position.set(8, 6, 12);
    this.camera.lookAt(0, 2, 0);

    const back = el('button', { className: 'btn', text: 'Menü', attrs: { 'data-action': 'menu' } });
    back.addEventListener('click', () => void ctx.goTo('menu'));
    ctx.ui.append(el('div', { className: 'hud-bar', attrs: { 'data-scene': 'editor' } }, [back, el('span', { className: 'title', text: 'Hangar' })]));
    this.resize(ctx.renderer.domElement.clientWidth, ctx.renderer.domElement.clientHeight);
  }

  fixedUpdate(): void {}

  render(): void {
    this.ctx.renderer.render(this.scene, this.camera);
  }

  resize(w: number, h: number): void {
    this.camera.aspect = w / Math.max(h, 1);
    this.camera.updateProjectionMatrix();
  }

  exit(): void {
    disposeScene(this.scene);
  }
}
