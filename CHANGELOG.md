# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Releases are managed by release-please from Conventional Commits.

## [Unreleased]

### Changed

* Rebuilt vne3dgs as a C++20 VertexNova library scaffolded from vnetemplate (`vne::gs`).
  The earlier Python/gsplat prototype is preserved at git tag `python-prototype`.
* License changed from MIT to Apache 2.0 to match the rest of the VertexNova stack.

### Added

* Learning roadmap and per-task specs with learning material under `docs/vertexnova/gs/`.
* vnemath dependency (`deps/internal/vnemath`).
