import * as THREE from 'three';
import type { GameScene, SceneContext } from '../core/Scene';
import { PhysicsWorld, type BodyHandle } from '../physics/PhysicsWorld';
import { el } from '../ui/dom';
import { disposeScene } from '../ui/MainMenuScene';

const BALL_RADIUS = 1;
const START_HEIGHT = 12;

/** Test-Flugszene: eine Kugel (Rapier-Körper) fällt auf einen Boden. Platzhalter für T21. */
export class FlightScene implements GameScene {
  readonly id = 'flight' as const;
  private scene = new THREE.Scene();
  private camera = new THREE.PerspectiveCamera(55, 1, 0.1, 1000);
  private ctx!: SceneContext;
  private physics!: PhysicsWorld;
  private ball!: BodyHandle;
  private ballMesh!: THREE.Mesh;
  private readout!: HTMLElement;

  async enter(ctx: SceneContext): Promise<void> {
    this.ctx = ctx;
    this.physics = await PhysicsWorld.create();
    this.physics.addStaticBox({ x: 0, y: -0.5, z: 0 }, { x: 25, y: 0.5, z: 25 });
    this.ball = this.physics.addDynamicSphere({ x: 0, y: START_HEIGHT, z: 0 }, BALL_RADIUS, 0.45);

    this.scene.background = new THREE.Color(0x7fb2d9);
    this.scene.fog = new THREE.Fog(0x7fb2d9, 30, 90);
    const ground = new THREE.Mesh(
      new THREE.BoxGeometry(50, 1, 50),
      new THREE.MeshStandardMaterial({ color: 0x5d7a4a, roughness: 0.95 }),
    );
    ground.position.y = -0.5;
    ground.receiveShadow = true;
    this.ballMesh = new THREE.Mesh(
      new THREE.SphereGeometry(BALL_RADIUS, 48, 32),
      new THREE.MeshStandardMaterial({ color: 0xff8a3d, roughness: 0.4, metalness: 0.2 }),
    );
    this.ballMesh.castShadow = true;
    const sun = new THREE.DirectionalLight(0xfff4e6, 2.4);
    sun.position.set(8, 20, 10);
    sun.castShadow = true;
    sun.shadow.mapSize.set(1024, 1024);
    this.scene.add(ground, this.ballMesh, sun, new THREE.HemisphereLight(0xbcd8ff, 0x3a4a2a, 0.7));

    this.camera.position.set(0, 6, 18);
    this.camera.lookAt(0, 4, 0);

    const back = el('button', { className: 'btn', text: 'Menü', attrs: { 'data-action': 'menu' } });
    back.addEventListener('click', () => void ctx.goTo('menu'));
    this.readout = el('div', { className: 'hud-readout', attrs: { 'data-testid': 'altitude' } });
    ctx.ui.append(el('div', { className: 'hud-bar', attrs: { 'data-scene': 'flight' } }, [back, el('span', { className: 'title', text: 'Flug' })]), this.readout);
    this.resize(ctx.renderer.domElement.clientWidth, ctx.renderer.domElement.clientHeight);
  }

  fixedUpdate(): void {
    this.physics.step();
  }

  render(alpha: number): void {
    const p = this.physics.interpolated(this.ball, alpha);
    this.ballMesh.position.set(p.x, p.y, p.z);
    this.readout.textContent = `Höhe: ${Math.max(0, p.y - BALL_RADIUS).toFixed(2)} m`;
    this.ctx.renderer.render(this.scene, this.camera);
  }

  /** Aktuelle physikalische Höhe der Kugel (für Tests/Debug). */
  get ballHeight(): number {
    return this.ball.translation().y;
  }

  resize(w: number, h: number): void {
    this.camera.aspect = w / Math.max(h, 1);
    this.camera.updateProjectionMatrix();
  }

  exit(): void {
    this.physics.dispose();
    disposeScene(this.scene);
  }
}
