//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DeviceAdapterListTag_h
#define vtk_m_cont_DeviceAdapterListTag_h

#ifndef VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
#define VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG ::vtkm::cont::DeviceAdapterListTagCommon
#endif

#include <vtkm/ListTag.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm
{
namespace cont
{

struct DeviceAdapterListTagCommon : vtkm::ListTagBase<vtkm::cont::DeviceAdapterTagCuda,
                                                      vtkm::cont::DeviceAdapterTagTBB,
                                                      vtkm::cont::DeviceAdapterTagSerial>
{
};

namespace detail
{

template <typename FunctorType>
class ExecuteIfValidDeviceTag
{
private:
  template <typename DeviceAdapter>
  using EnableIfValid = std::enable_if<vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::Valid>;

  template <typename DeviceAdapter>
  using EnableIfInvalid = std::enable_if<!vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::Valid>;

public:
  explicit ExecuteIfValidDeviceTag(const FunctorType& functor)
    : Functor(functor)
  {
  }

  template <typename DeviceAdapter>
  typename EnableIfValid<DeviceAdapter>::type operator()(DeviceAdapter) const
  {
    this->Functor(DeviceAdapter());
  }

  template <typename DeviceAdapter>
  typename EnableIfInvalid<DeviceAdapter>::type operator()(DeviceAdapter) const
  {
  }

private:
  FunctorType Functor;
};
} // detail

template <typename DeviceList, typename Functor>
VTKM_CONT void ForEachValidDevice(DeviceList devices, const Functor& functor)
{
  vtkm::ListForEach(detail::ExecuteIfValidDeviceTag<Functor>(functor), devices);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_DeviceAdapterListTag_h
