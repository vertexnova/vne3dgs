# Glossary

Terms in alphabetical order. The task where each one first matters is in brackets.

**Adam** [18] — The optimizer 3DGS training uses. Gradient descent with a per-parameter step size, adapted
from running averages of the gradient (first moment) and squared gradient (second moment).

**Alpha (α)** [01] — How much of the light arriving from behind a splat it blocks at one pixel.
`α = opacity × Gaussian falloff at that pixel`, capped at 0.99. Not the same as opacity: opacity is a
per-Gaussian constant, alpha varies per pixel.

**Alpha compositing / "over" operator** [01] — Combining semi-transparent layers. Front to back:
`C += T·α·c; T *= (1 − α)`. Back to front: `C = α·c + (1 − α)·C`. Both give the same image.

**Anisotropic** [02] — Different sizes along different axes. 3DGS Gaussians are anisotropic ellipsoids,
unlike the isotropic spheres of classic point splatting.

**Conic** [01] — The inverse of a 2×2 covariance, stored as three numbers `(A, B, C)`. It's called a conic
because `A·dx² + 2B·dx·dy + C·dy² = const` is an ellipse. The renderer evaluates the Gaussian with it.

**COLMAP** [15] — The standard structure-from-motion tool. From photos it recovers camera intrinsics, each
photo's pose and a sparse 3D point cloud. 3DGS training starts from its output.

**Covariance (Σ)** [01, 02] — A symmetric positive semi-definite matrix (2×2 in 2D, 3×3 in 3D) that sets
the Gaussian's shape. Its eigenvectors are the ellipse axes, and its eigenvalues are the squared lengths
along those axes (in σ units).

**D-SSIM** [16] — `1 − SSIM`, a structural-similarity image loss. 3DGS uses `0.8·L1 + 0.2·D-SSIM`.

**Densification** [16] — Adding Gaussians during training where the scene is under-reconstructed. Small
Gaussians with large position gradients are **cloned**; large ones are **split** into two smaller ones.

**Depth sorting** [06] — Ordering splats by camera-space depth so blending is front to back. 3DGS sorts
by the depth of each Gaussian's center, one global order per tile. That is an approximation: overlapping
Gaussians are not sorted per pixel.

**Early termination** [06] — Stop blending a pixel once transmittance `T` drops below 1e-4; nothing behind
can contribute visibly.

**EWA splatting** [05] — Elliptical Weighted Average splatting (Zwicker et al. 2001). It projects a 3D
Gaussian to a 2D Gaussian by linearizing the perspective projection around the Gaussian's center:
`Σ₂D = J W Σ Wᵀ Jᵀ`.

**Extrinsics** [04] — The camera pose: the rotation and translation mapping world to camera coordinates
(the view matrix).

**Intrinsics** [04] — The camera's internal parameters: focal lengths `fx, fy` and principal point
`cx, cy` in pixels. They map camera coordinates to pixels.

**Jacobian (J)** [05] — The matrix of partial derivatives of the projection (camera space → pixels). It is
the local linear approximation EWA uses.

**Logit / sigmoid** [03] — `sigmoid(x) = 1/(1+e⁻ˣ)` maps any real number to (0, 1); logit is its inverse.
Opacity is stored as a logit so the optimizer can move freely without leaving (0, 1).

**Novel view synthesis** [overview] — Rendering a scene from a viewpoint no photo was taken from.

**NDC** [04] — Normalized device coordinates. 3DGS papers go camera → NDC → pixels. You can go camera →
pixels directly with intrinsics.

**Opacity (o)** [01] — Per-Gaussian peak alpha, in (0, 1).

**PLY** [03] — Polygon File Format. Trained 3DGS scenes are saved as a PLY "vertex" list whose custom
properties are the Gaussian parameters.

**Prefix sum / scan** [11] — Computing running totals over an array in parallel. The building block of
GPU radix sort and of computing per-tile ranges.

**Pruning** [16] — Removing Gaussians during training (near-transparent, or too large).

**PSNR** [16] — Peak signal-to-noise ratio, the standard image quality metric (higher is better; about 27
dB is typical for 3DGS on Mip-NeRF 360).

**Radiance field** [overview] — A function giving the color of light leaving any point in any direction.
NeRF learns one implicitly; 3DGS represents one with Gaussians plus SH.

**Radix sort** [11] — A non-comparison sort that processes keys a few bits at a time. The standard GPU sort
for millions of keys.

**Radius (screen-space)** [05] — How far, in pixels, a splat's footprint extends: `ceil(3·√λmax)` of the
2D covariance (3σ along the longest axis).

**Spherical harmonics (SH)** [08] — An orthonormal basis for functions on the sphere, the angular
counterpart of a Fourier series. 3DGS stores color as SH coefficients (degree 3: 16 per channel), so color
can change with viewing direction.

**SfM (structure from motion)** [15] — Recovering camera poses and 3D structure from overlapping photos.

**Splat** [overview] — A 3D Gaussian after projection to the screen: a 2D elliptical footprint.

**Tile** [07] — A 16×16 pixel block. Splats are binned into tiles so each tile blends only its own short,
sorted list.

**Transmittance (T)** [01] — The fraction of light that still gets through all splats in front of the
current one. Starts at 1 and multiplies by `(1 − α)` after each splat.

**View-dependent color** [08] — Color that changes with the viewing direction (highlights, reflections).
In 3DGS this comes from SH degrees 1–3.
