//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DeviceAdapterListTag_h
#define vtk_m_cont_DeviceAdapterListTag_h

#ifndef VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
#define VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG ::vtkm::cont::DeviceAdapterListTagCommon
#endif

#include <vtkm/ListTag.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm
{
namespace cont
{

struct DeviceAdapterListTagCommon : vtkm::ListTagBase<vtkm::cont::DeviceAdapterTagCuda,
                                                      vtkm::cont::DeviceAdapterTagTBB,
                                                      vtkm::cont::DeviceAdapterTagOpenMP,
                                                      vtkm::cont::DeviceAdapterTagSerial>
{
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_DeviceAdapterListTag_h
