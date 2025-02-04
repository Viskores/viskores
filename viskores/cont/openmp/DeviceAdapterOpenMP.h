//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_cont_openmp_DeviceAdapterOpenMP_h
#define viskores_cont_openmp_DeviceAdapterOpenMP_h

#include <viskores/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#ifdef VISKORES_ENABLE_OPENMP
#include <viskores/cont/openmp/internal/DeviceAdapterAlgorithmOpenMP.h>
#include <viskores/cont/openmp/internal/DeviceAdapterMemoryManagerOpenMP.h>
#include <viskores/cont/openmp/internal/RuntimeDeviceConfigurationOpenMP.h>
#endif

#endif //viskores_cont_openmp_DeviceAdapterOpenMP_h
