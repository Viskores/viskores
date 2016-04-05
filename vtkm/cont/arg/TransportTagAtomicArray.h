//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagAtomicArray_h
#define vtk_m_cont_arg_TransportTagAtomicArray_h

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/arg/Transport.h>

#include <vtkm/exec/AtomicArray.h>

namespace vtkm {
namespace cont {
namespace arg {

/// \brief \c Transport tag for in-place arrays with atomic operations.
///
/// \c TransportTagAtomicArray is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for data that is both input and output
/// (that is, in place modification of array data). The array will be wrapped
/// in a vtkm::exec::AtomicArray class that provides atomic operations (like
/// add and compare/swap).
///
struct TransportTagAtomicArray {  };

template<typename T, typename Device>
struct Transport<
    vtkm::cont::arg::TransportTagAtomicArray,
    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>,
    Device>
{
  typedef vtkm::exec::AtomicArray<T, Device> ExecObjectType;

  VTKM_CONT_EXPORT
  ExecObjectType operator()(
      vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> array,
      vtkm::Id) const
  {
    // Note: we ignore the size of the domain because the randomly accessed
    // array might not have the same size depending on how the user is using
    // the array.

    return ExecObjectType(array);
  }
};

}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagAtomicArray_h
