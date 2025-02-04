//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_DeviceAdapterList_h
#define viskores_cont_DeviceAdapterList_h

#ifndef VISKORES_DEFAULT_DEVICE_ADAPTER_LIST
#define VISKORES_DEFAULT_DEVICE_ADAPTER_LIST ::viskores::cont::DeviceAdapterListCommon
#endif

#include <viskores/List.h>

#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <viskores/cont/kokkos/internal/DeviceAdapterTagKokkos.h>
#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <viskores/cont/serial/internal/DeviceAdapterTagSerial.h>
#include <viskores/cont/tbb/internal/DeviceAdapterTagTBB.h>

namespace viskores
{
namespace cont
{

using DeviceAdapterListCommon = viskores::List<viskores::cont::DeviceAdapterTagCuda,
                                           viskores::cont::DeviceAdapterTagTBB,
                                           viskores::cont::DeviceAdapterTagOpenMP,
                                           viskores::cont::DeviceAdapterTagKokkos,
                                           viskores::cont::DeviceAdapterTagSerial>;
}
} // namespace viskores::cont

#endif //viskores_cont_DeviceAdapterList_h
