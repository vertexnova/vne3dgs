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

#include "common/logging_guard.h"
#include "vertexnova/gs/gs.h"

int main() {
    vne::gs::examples::LoggingGuard logging_guard;

    VNE_LOG_INFO << "vne3dgs version: " << vne::gs::getVersion();
    VNE_LOG_INFO << "Start learning at docs/vertexnova/gs/roadmap.md";

    return 0;
}
