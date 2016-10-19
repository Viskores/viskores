//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h
#define vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h

#include <vtkm/cont/internal/ArrayManagerExecution.h>
#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace vtkm {
namespace cont {
namespace internal {

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, vtkm::cont::DeviceAdapterTagSerial>
    : public vtkm::cont::internal::ArrayManagerExecutionShareWithControl
          <T, StorageTag>
{
public:
  typedef vtkm::cont::internal::ArrayManagerExecutionShareWithControl
      <T, StorageTag> Superclass;
  typedef typename Superclass::ValueType ValueType;
  typedef typename Superclass::PortalType PortalType;
  typedef typename Superclass::PortalConstType PortalConstType;

  VTKM_CONT
  ArrayManagerExecution(typename Superclass::StorageType *storage)
    : Superclass(storage) {  }
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h
