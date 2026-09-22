# Task 14 — Polish (Pick-List)

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 4 — Real-time GPU viewer | [12](12_splat_quads.md) | — | [ ] |

> **Goal:** pick the improvements that interest you. Each item is small, self-contained and teaches one
> idea used in production 3DGS renderers.

## How to use this task

Do the items in any order, and skip any. Each has the same mini-structure: learn → build → done when. Mark
the ones you finish below, and record measurements in *My notes*.

## Items

### A. Culling

- **Learn:** frustum culling (Task 05 culls per Gaussian after projection; culling earlier is cheaper), and
  **small-splat culling** (splats whose radius is below ~1 px add little).
- **Build:** a cheap pre-pass test of the center + max scale against the frustum planes
  (`vne::math::Frustum` exists in vnemath); an ImGui slider for the minimum radius.
- **Done when:** you've measured frame time vs image difference for several thresholds.
- [ ] done

### B. Memory: compress the SH

- **Learn:** SH is ~80% of the data. fp16 halves it with negligible visual change. Web formats (`.splat`,
  `.ksplat`, Niantic's open **SPZ**) quantize positions, scales and rotations too.
- **Build:** a GPU buffer with fp16 SH (`packHalf2x16` / `unpackHalf2x16` in GLSL); optionally drop SH
  degree 3 for distant Gaussians.
- **Done when:** the memory is halved and the PSNR vs fp32 on the golden view is > 45 dB.
- [ ] done

### C. Anti-aliasing: beyond the `+0.3` dilation

- **Learn:** the fixed screen-space dilation (Task 05) makes zoomed-out views too bright and fat, and
  zoomed-in thin structures erode. Read **Mip-Splatting** (Yu et al. 2024) §3–4. A practical fix, used by
  gsplat's `antialiased` rasterize mode, keeps the dilation but **compensates opacity**:
  `opacity' = opacity · sqrt(det(Σ₂D) / det(Σ₂D + 0.3·I))`, so a sub-pixel splat is fainter instead of
  fatter.
- **Build:** a toggle between the classic and compensated modes; zoom far out on the garden and compare.
- **Caveat:** scenes trained without compensation look slightly different with it. That's expected;
  models trained with it (e.g. in Task 17 with gsplat's antialiased mode) look right.
- [ ] done

### D. Debug views

- **Build:** view modes for depth (`Σ Tᵢαᵢzᵢ`), alpha (`1 − T`), splats per pixel, SH degree 0 vs full,
  "only Gaussians with opacity > x", and "scale × k".
- **Done when:** you can explain one artifact in the garden using these views.
- [ ] done

### E. Multiple scenes and transforms

- **Learn:** placing a scene with a model matrix. Remember that SH lives in world space (Task 08 §5):
  transforming positions and covariances is easy, SH needs rotating too.
- **Build:** load two `.ply` files with separate transforms; either rotate SH (degree-1 is a 3×3; higher
  degrees need Wigner-D matrices) or evaluate SH with the direction transformed back into each scene's
  local frame (much simpler).
- [ ] done

### F. Performance profiling

- **Build:** a benchmark mode: a fixed camera path of 300 frames, per-stage GPU timings to CSV. Run it on
  the Mac and the Spark, with both renderers.
- **Done when:** a table in *My notes* shows where time goes and what the bottleneck is on each machine.
- [ ] done

### G. (Stretch) Stereo / XR

- **Learn:** stereo rendering is two cameras per frame; per-eye sorting differs slightly. vnexr exists in
  the stack.
- **Build:** a side-by-side stereo mode in the viewer.
- [ ] done

## Read

- **Mip-Splatting** (Yu et al., CVPR 2024), for item C.
- **gsplat docs**, the `rasterization()` parameters (`rasterize_mode`, `packed`, `sparse_grad`), to see
  which knobs production renderers expose.
- For item B, the Niantic SPZ repository README (layout and quantization choices).

## My notes

_Fill in after finishing._
