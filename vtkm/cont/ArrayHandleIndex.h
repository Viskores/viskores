//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
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
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_ArrayHandleIndex_h
#define vtk_m_cont_ArrayHandleIndex_h

#include <vtkm/cont/ArrayHandleImplicit.h>

namespace vtkm {
namespace cont {

namespace detail {

struct IndexFunctor {
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id operator()(vtkm::Id index) const { return index; }
};

} // namespace detail

/// \brief An implicit array handle containing the its own indices.
///
/// \c ArrayHandleIndex is an implicit array handle containing the values
/// 0, 1, 2, 3,... to a specified size. Every value in the array is the same
/// as the index to that value.
///
class ArrayHandleIndex
    : public vtkm::cont::ArrayHandleImplicit<vtkm::Id, detail::IndexFunctor>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS_NT(
      ArrayHandleIndex,
      (vtkm::cont::ArrayHandleImplicit<vtkm::Id, detail::IndexFunctor>));

  VTKM_CONT_EXPORT
  ArrayHandleIndex(vtkm::Id length)
    : Superclass(detail::IndexFunctor(), length) {  }
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleIndex_h
