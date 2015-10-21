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
#ifndef vtk_m_cont_ArrayHandleUniformPointCoordinates_h
#define vtk_m_cont_ArrayHandleUniformPointCoordinates_h

#include <vtkm/internal/ArrayPortalUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

namespace vtkm {
namespace cont {

/// ArrayHandleUniformPointCoordinates is a specialization of ArrayHandle. It
/// contains the information necessary to compute the point coordinates in a
/// uniform orthogonal grid (extent, origin, and spacing) and implicitly
/// computes these coordinates in its array portal.
///
class ArrayHandleUniformPointCoordinates
    : public vtkm::cont::ArrayHandle<
        vtkm::Vec<vtkm::FloatDefault,3>,
        vtkm::cont::StorageTagImplicit<
          vtkm::internal::ArrayPortalUniformPointCoordinates> >
{
public:
  typedef vtkm::Vec<vtkm::FloatDefault,3> ValueType;
  typedef vtkm::cont::StorageTagImplicit<
      vtkm::internal::ArrayPortalUniformPointCoordinates> StorageTag;

  typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> Superclass;

private:
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  VTKM_CONT_EXPORT
  ArrayHandleUniformPointCoordinates() : Superclass() {  }

  VTKM_CONT_EXPORT
  ArrayHandleUniformPointCoordinates(
      const ArrayHandleUniformPointCoordinates &src)
    : Superclass(src)
  {  }

  VTKM_CONT_EXPORT
  ArrayHandleUniformPointCoordinates(
      vtkm::Id3 dimensions,
      ValueType origin = ValueType(0.0f, 0.0f, 0.0f),
      ValueType spacing = ValueType(1.0f, 1.0f, 1.0f))
    : Superclass(
        StorageType(vtkm::internal::ArrayPortalUniformPointCoordinates(
                      dimensions, origin, spacing)))
  {  }

  VTKM_CONT_EXPORT
  ArrayHandleUniformPointCoordinates(
      const vtkm::cont::ArrayHandle<ValueType,StorageTag> &src)
    : Superclass(src)
  {  }

  VTKM_CONT_EXPORT
  virtual ~ArrayHandleUniformPointCoordinates() {  }
};

}
} // namespace vtkm::cont

#endif //vtk_+m_cont_ArrayHandleUniformPointCoordinates_h
