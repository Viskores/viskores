//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_openmp_internal_DeviceAdapterMemoryManagerOpenMP_h
#define viskores_cont_openmp_internal_DeviceAdapterMemoryManagerOpenMP_h

#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#include <viskores/cont/internal/DeviceAdapterMemoryManagerShared.h>

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
class DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagOpenMP>
  : public viskores::cont::internal::DeviceAdapterMemoryManagerShared
{
  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override
  {
    return viskores::cont::DeviceAdapterTagOpenMP{};
  }
};
}
}
}

#endif //viskores_cont_openmp_internal_DeviceAdapterMemoryManagerOpenMP_h
