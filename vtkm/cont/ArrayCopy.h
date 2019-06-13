//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayCopy_h
#define vtk_m_cont_ArrayCopy_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/TryExecute.h>

// TODO: When virtual arrays are available, compile the implementation in a .cxx/.cu file. Common
// arrays are copied directly but anything else would be copied through virtual methods.

namespace vtkm
{
namespace cont
{
/// \brief Does a deep copy from one array to another array.
///
/// Given a source \c ArrayHandle and a destination \c ArrayHandle, this function allocates the
/// destination \c ArrayHandle to the correct size and deeply copies all the values from the source
/// to the destination.
///
template <typename InValueType, typename InStorage, typename OutValueType, typename OutStorage>
VTKM_CONT inline void ArrayCopy(const vtkm::cont::ArrayHandle<InValueType, InStorage>& source,
                                vtkm::cont::ArrayHandle<OutValueType, OutStorage>& destination)
{
  bool isCopied =
    vtkm::cont::Algorithm::Copy(vtkm::cont::DeviceAdapterTagAny(), source, destination);
  if (!isCopied)
  { // If after the second pass, still not valid through an exception
    throw vtkm::cont::ErrorExecution("Failed to run ArrayCopy on any device.");
  }
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayCopy_h
