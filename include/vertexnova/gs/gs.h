#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   September 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file gs.h
 * @brief Umbrella header for vne3dgs (3D Gaussian Splatting for VertexNova).
 * @ingroup vne::gs
 *
 * @details Each learning task (docs/vertexnova/gs/tasks/) adds its public headers here.
 */

#include "vertexnova/gs/camera/camera.h"
#include "vertexnova/gs/camera/conventions.h"
#include "vertexnova/gs/core/conic.h"
#include "vertexnova/gs/core/front_to_back_blender.h"
#include "vertexnova/gs/core/gaussian2d.h"
#include "vertexnova/gs/core/gaussian3d.h"
#include "vertexnova/gs/core/gaussian_cloud.h"
#include "vertexnova/gs/io/ply_reader.h"
#include "vertexnova/gs/io/ply_writer.h"
#include "vertexnova/gs/render/cpu/point_renderer.h"
#include "vertexnova/gs/render/image.h"
#include "vertexnova/gs/export.h"
#include "vertexnova/gs/version.h"
