#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   September 2026
 * ----------------------------------------------------------------------
 */

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
/* Building the vne3dgs shared library (CMake sets VNE_GS_BUILDING_DLL). */
#if defined(VNE_GS_BUILDING_DLL)
#define VNE_GS_API __declspec(dllexport)
/* Consuming the vne3dgs shared library (CMake sets VNE_GS_DLL on dependents). */
#elif defined(VNE_GS_DLL)
#define VNE_GS_API __declspec(dllimport)
/* Static build or unknown: no export/import. */
#else
#define VNE_GS_API
#endif
#else
/* Non-Windows: no dllexport/dllimport. */
#define VNE_GS_API
#endif
