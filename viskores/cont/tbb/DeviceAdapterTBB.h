//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_tbb_DeviceAdapterTBB_h
#define viskores_cont_tbb_DeviceAdapterTBB_h

#include <viskores/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>
#include <viskores/cont/tbb/internal/DeviceAdapterTagTBB.h>

#ifdef VISKORES_ENABLE_TBB
#include <viskores/cont/tbb/internal/DeviceAdapterAlgorithmTBB.h>
#include <viskores/cont/tbb/internal/DeviceAdapterMemoryManagerTBB.h>
#include <viskores/cont/tbb/internal/RuntimeDeviceConfigurationTBB.h>
#endif

#endif //viskores_cont_tbb_DeviceAdapterTBB_h
