//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayCopyUnknown_h
#define vtk_m_cont_internal_ArrayCopyUnknown_h

#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

// Rather than include UnknownArrayHandle.h, we just forward declare the class so
// we can declare our functions and prevent any circular header dependencies from
// core classes.
class UnknownArrayHandle;

namespace internal
{

/// Same as `ArrayCopy` with `UnknownArrayHandle` except that it can be used without
/// using a device compiler.
///
VTKM_CONT_EXPORT void ArrayCopyUnknown(const vtkm::cont::UnknownArrayHandle& source,
                                       vtkm::cont::UnknownArrayHandle& destination);

VTKM_CONT_EXPORT void ArrayCopyUnknown(const vtkm::cont::UnknownArrayHandle& source,
                                       const vtkm::cont::UnknownArrayHandle& destination);


} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_internal_ArrayCopyUnknown_h
