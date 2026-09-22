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

#include <gtest/gtest.h>
#include "vertexnova/gs/gs.h"
#include "config.h"

TEST(Vne3dgsVersion, IsNotEmpty) {
    const char* ver = vne::gs::getVersion();
    ASSERT_NE(ver, nullptr);
    EXPECT_STRNE(ver, "");
}

TEST(Vne3dgsVersion, MatchesProjectVersion) {
    EXPECT_STREQ(vne::gs::getVersion(), PROJECT_VERSION);
}
