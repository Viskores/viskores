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

// Everything in this header file is deprecated and movded to DeviceAdapterList.h.

#ifndef VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
#define VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG ::vtkm::cont::detail::DeviceAdapterListTagDefault
#endif

#include <vtkm/List.h>

#include <vtkm/cont/DeviceAdapterList.h>

namespace vtkm
{
namespace cont
{

struct VTKM_DEPRECATED(1.6,
                       "DeviceAdapterListTagCommon replaced by DeviceAdapterListCommon. "
                       "Note that the new DeviceAdapterListCommon cannot be subclassed.")
  DeviceAdapterListTagCommon : vtkm::internal::ListAsListTag<DeviceAdapterListCommon>
{
};

namespace detail
{

struct VTKM_DEPRECATED(
  1.6,
  "VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG replaced by VTKM_DEFAULT_DEVICE_ADAPTER_LIST. "
  "Note that the new VTKM_DEFAULT_DEVICE_ADAPTER_LIST cannot be subclassed.")
  DeviceAdapterListTagDefault : vtkm::internal::ListAsListTag<VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
{
};

} // namespace detail
}
} // namespace vtkm::cont

#endif //vtk_m_cont_DeviceAdapterListTag_h
