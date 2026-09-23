# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Releases are managed by release-please from Conventional Commits.

## [0.3.0](https://github.com/vertexnova/vne3dgs/compare/v0.2.0...v0.3.0) (2026-09-23)


### Features

* add vneio submodule and 2D Gaussian example ([#3](https://github.com/vertexnova/vne3dgs/issues/3)) ([ba3659e](https://github.com/vertexnova/vne3dgs/commit/ba3659e4a2d35aaf6279568a1e67db7695a985be))

## [0.2.0](https://github.com/vertexnova/vne3dgs/compare/v0.1.0...v0.2.0) (2026-09-22)


### Features

* initial checkins ([92775fc](https://github.com/vertexnova/vne3dgs/commit/92775fc7b06b5a47e4d628c3949a9954324f671a))
* initial commit for vne3dgs ([2dd04eb](https://github.com/vertexnova/vne3dgs/commit/2dd04eb8ea6be2a7d2e08bee25eed37e04b1b3f4))
* intial task planning for the 3dgs ([4109e9c](https://github.com/vertexnova/vne3dgs/commit/4109e9c595c3e92d5fe68cf7892322c43d903b88))
* setting up the lib ([482883a](https://github.com/vertexnova/vne3dgs/commit/482883a1c9fa2b291f2b38ee9fd532cd25f4f5bd))
* vne3dgs v0.1.0 - first 3DGS render ([9d24acb](https://github.com/vertexnova/vne3dgs/commit/9d24acb152ceaae9fe3854b151cb1c9ff2d9b93b))

## [Unreleased]

### Changed

* Rebuilt vne3dgs as a C++20 VertexNova library scaffolded from vnetemplate (`vne::gs`).
  The earlier Python/gsplat prototype is preserved at git tag `python-prototype`.
* License changed from MIT to Apache 2.0 to match the rest of the VertexNova stack.

### Added

* Learning roadmap and per-task specs with learning material under `docs/vertexnova/gs/`.
* vnemath dependency (`deps/internal/vnemath`).
* 2D Gaussian evaluation, alpha conversion and front-to-back blending (`Conic`, `Gaussian2D`,
  `FrontToBackBlender`; one type per header) with unit tests and `example_01_gaussians_2d`.
* Examples-only vneio image dependency for PNG output (`vne::image::image_utils::saveImage`).
