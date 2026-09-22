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
 * @file version.h
 * @brief Library version query for vne3dgs.
 * @ingroup vne::gs
 */

#include "vertexnova/gs/export.h"

namespace vne::gs {

/** @brief Returns the vne3dgs version string (e.g. "0.1.0"), taken from the VERSION file. */
VNE_GS_API const char* getVersion();

}  // namespace vne::gs
