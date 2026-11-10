import * as THREE from 'three';
import type { GameScene, SceneContext } from '../core/Scene';
import { createStarfield } from '../render/starfield';
import { el } from './dom';

/** Hauptmenü: Titel ORBITWERK, Buttons "Hangar" und "Flug" vor einem langsam drehenden Planeten. */
export class MainMenuScene implements GameScene {
  readonly id = 'menu' as const;
  private scene = new THREE.Scene();
  private camera = new THREE.PerspectiveCamera(45, 1, 0.1, 2000);
  private planet!: THREE.Mesh;
  private ctx!: SceneContext;
  private spin = 0;
  private prevSpin = 0;

  enter(ctx: SceneContext): void {
    this.ctx = ctx;
    this.scene.background = new THREE.Color(0x03050a);
    this.scene.add(createStarfield());

    // Platzhalter-Heimatplanet (eigener Look; echte Welt kommt mit T11).
    const geo = new THREE.SphereGeometry(4, 96, 64);
    const mat = new THREE.MeshStandardMaterial({ color: 0x2f7f8f, roughness: 0.85, metalness: 0 });
    this.planet = new THREE.Mesh(geo, mat);
    this.planet.position.set(0, -5.2, 0);
    this.scene.add(this.planet);

    // Einfacher Atmosphärensaum (Fresnel-artig, additiv).
    const rim = new THREE.Mesh(
      new THREE.SphereGeometry(4.25, 96, 64),
      new THREE.ShaderMaterial({
        transparent: true,
        blending: THREE.AdditiveBlending,
        side: THREE.BackSide,
        depthWrite: false,
        vertexShader: /* glsl */ `
          varying vec3 vN; varying vec3 vV;
          void main(){ vec4 mv = modelViewMatrix*vec4(position,1.0); vN = normalize(normalMatrix*normal); vV = normalize(-mv.xyz); gl_Position = projectionMatrix*mv; }`,
        fragmentShader: /* glsl */ `
          varying vec3 vN; varying vec3 vV;
          void main(){ float f = pow(1.0 - abs(dot(vN, vV)), 2.5); gl_FragColor = vec4(vec3(0.45,0.75,1.0)*f*1.4, f); }`,
      }),
    );
    rim.position.copy(this.planet.position);
    this.scene.add(rim);

    const sun = new THREE.DirectionalLight(0xfff1e0, 2.2);
    sun.position.set(-6, 4, 5);
    this.scene.add(sun, new THREE.AmbientLight(0x223344, 0.4));

    this.camera.position.set(0, 0, 10);
    this.camera.lookAt(0, -1.2, 0);

    const hangar = el('button', { className: 'btn', text: 'Hangar', attrs: { 'data-action': 'hangar' } });
    const flight = el('button', { className: 'btn', text: 'Flug', attrs: { 'data-action': 'flight' } });
    hangar.addEventListener('click', () => void ctx.goTo('editor'));
    flight.addEventListener('click', () => void ctx.goTo('flight'));
    ctx.ui.append(
      el('div', { className: 'menu', attrs: { 'data-scene': 'menu' } }, [
        el('h1', { text: 'ORBITWERK' }),
        el('p', { className: 'subtitle', text: 'Helion-System · Raketenwerft' }),
        el('div', { className: 'buttons' }, [hangar, flight]),
      ]),
    );
    this.resize(ctx.renderer.domElement.clientWidth, ctx.renderer.domElement.clientHeight);
  }

  fixedUpdate(dt: number): void {
    this.prevSpin = this.spin;
    this.spin += dt * 0.05;
  }

  render(alpha: number): void {
    this.planet.rotation.y = this.prevSpin + (this.spin - this.prevSpin) * alpha;
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

/** Geometrien/Materialien einer Szene freigeben. */
export function disposeScene(scene: THREE.Scene): void {
  scene.traverse((o) => {
    const m = o as THREE.Mesh;
    m.geometry?.dispose();
    const mat = m.material;
    if (Array.isArray(mat)) mat.forEach((x) => x.dispose());
    else mat?.dispose();
  });
  scene.clear();
}
