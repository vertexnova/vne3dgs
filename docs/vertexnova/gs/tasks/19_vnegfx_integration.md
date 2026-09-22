# Task 19 — Splats in vnegfx, Composited with Meshes

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 6 — Integration (optional) | [12](12_splat_quads.md) (and ideally [14](14_polish.md) item D) | — | [ ] |

> **Goal:** make Gaussian splatting a render pass in vnegfx, so splat scenes and ordinary meshes appear in
> one frame and occlude each other correctly.

## Why this task

A standalone viewer is a demo; an engine feature is a tool. Real uses need both: annotations, instruments
or UI geometry over a reconstructed scene (the surgical-overlay idea from the original vne3dgs roadmap),
or a captured environment around modeled objects. That means depth-correct compositing of splats with
opaque meshes.

## Learn

### 1. The compositing problem

Meshes are opaque and write depth. Splats are semi-transparent and sorted, and have no single depth per
pixel. Correct mixing:

1. **Meshes first:** render opaque meshes → color + depth buffer.
2. **Splats test against mesh depth:**
   - *Quads (Task 12):* depth **test** on against the mesh depth buffer, depth **write** off. Splats behind
     a mesh are rejected per fragment; splats in front blend over it.
   - *Tile compute (Task 13):* read the mesh depth per pixel and stop walking a pixel's list once a splat's
     depth exceeds it (treat the mesh as the "background" at that pixel).
3. **Meshes drawn after splats** (e.g. transparent overlays) need a splat depth: use the **expected depth**
   `Σ Tᵢ αᵢ zᵢ / (1 − T)` or the depth where `T` crosses 0.5 (Task 06 *Going further*, Task 14 item D).
   Neither is exact; choose per use case.

### 2. Depth conventions, again

Splat depth is camera-space z (Task 05); the depth buffer holds a non-linear (or reversed-Z) NDC depth.
Convert with the projection's near/far values. Check vnegfx's conventions (reversed-Z?) before writing
the comparison.

### 3. Fitting into vnegfx

vnegfx has a frame engine (`PassPipeline`, `GraphicsSystem`, `RenderSession`) and transparency techniques
(A-buffer and weighted-blended OIT samples). Study how an existing transparent technique is registered,
which resources it reads (depth), and where it sits in the pass order. Then add a **splat pass** that wraps
`vne3dgs_gpu`'s renderer.

Decide where the code lives: a `SplatPass` inside vnegfx depending on vne3dgs, or an adapter in vne3dgs
depending on vnegfx. Prefer the direction that keeps vne3dgs's core free of engine dependencies.

## Read

- `../vnegfx/docs/vertexnova/gfx/transparency-domain-onboarding.md`: how a new transparency technique is
  added to vnegfx; the closest template for a splat pass.
- `../vnegfx/docs/vertexnova/gfx/architecture/`: the frame engine and pass pipeline.
- `../vnegfx/samples/11_abuffer_oit`, `12_weighted_blend_oit`, `09_opaque_transparent`: how transparency
  passes consume depth and compose with opaque geometry.
- `../vnegfx/ARCHITECTURE_REVIEW.md`: where a new technique should plug in.

## Build

- [ ] A splat render pass registered with vnegfx's pass pipeline (quads path first).
- [ ] Depth-tested splats against the opaque pass's depth.
- [ ] A vnegfx sample: the garden scene with a mesh (e.g. the teapot from vnerhi's samples) placed on the
  table, orbitable.
- [ ] (Optional) The expected-depth output for passes that run after splats.

## Test

| Case | Expected |
|------|----------|
| Opaque quad placed in front of splats | fully hides them |
| Opaque quad placed behind splats | splats blend over it; the quad shows through where splat alpha is low |
| Mesh partly inside a splat cloud | correct partial occlusion (inspect visually; add a golden-image test once stable) |

## Done when

- [ ] A teapot sits on the garden table, hidden where the table's splats are in front of it and visible
  in front of the background, in the vnegfx sample on Metal and Vulkan.

## Check yourself

1. Why don't splats write depth?
2. What's the difference between "expected depth" and "median depth" for splats, and when would you pick each?
3. Why render opaque meshes first?

<details><summary>Answers</summary>

1. They're semi-transparent; writing depth would make the first splat drawn hide everything behind it,
   even when it's nearly transparent.
2. Expected depth averages depth weighted by contribution (smooth, but can fall between surfaces); median
   depth (where `T` crosses 0.5) picks the dominant surface (sharper, but jumps). Median is better for
   picking and occlusion, expected for smooth effects.
3. So the splat pass can test against their depth and blend over their color, the standard
   opaque-then-transparent order.

</details>

## My notes

_Fill in after finishing._
