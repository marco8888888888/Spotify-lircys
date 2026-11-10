import * as THREE from 'three';

/** Deterministischer PRNG (mulberry32) für prozedurale Inhalte. */
export function mulberry32(seed: number): () => number {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

/** Prozeduraler Sternenhimmel als Punktwolke auf einer großen Kugel. */
export function createStarfield(count = 4000, radius = 900, seed = 7): THREE.Points {
  const rnd = mulberry32(seed);
  const pos = new Float32Array(count * 3);
  const col = new Float32Array(count * 3);
  const c = new THREE.Color();
  for (let i = 0; i < count; i++) {
    const u = rnd() * 2 - 1;
    const phi = rnd() * Math.PI * 2;
    const s = Math.sqrt(1 - u * u);
    pos[i * 3] = radius * s * Math.cos(phi);
    pos[i * 3 + 1] = radius * u;
    pos[i * 3 + 2] = radius * s * Math.sin(phi);
    const temp = rnd();
    c.setHSL(temp < 0.7 ? 0.6 : 0.08, 0.5 * rnd(), 0.6 + 0.4 * rnd());
    col.set([c.r, c.g, c.b], i * 3);
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  geo.setAttribute('color', new THREE.BufferAttribute(col, 3));
  const mat = new THREE.PointsMaterial({ size: 1.6, sizeAttenuation: false, vertexColors: true, depthWrite: false });
  return new THREE.Points(geo, mat);
}
