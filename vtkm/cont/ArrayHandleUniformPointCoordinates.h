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

#include <vtkm/Extent.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

namespace vtkm {
namespace cont {

namespace internal {

/// \brief An implicit array port that computes point coordinates for a uniform
/// grid.
///
class ArrayPortalUniformPointCoordinates
{
public:
  typedef vtkm::Vec<vtkm::FloatDefault,3> ValueType;

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates() : NumberOfValues(0) {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates(vtkm::Extent3 extent,
                                     ValueType origin,
                                     ValueType spacing)
    : Extent(extent),
      Dimensions(vtkm::ExtentPointDimensions(extent)),
      NumberOfValues(vtkm::ExtentNumberOfPoints(extent)),
      Origin(origin),
      Spacing(spacing)
  {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates(
      const ArrayPortalUniformPointCoordinates &src)
    : Extent(src.Extent),
      Dimensions(src.Dimensions),
      NumberOfValues(src.NumberOfValues),
      Origin(src.Origin),
      Spacing(src.Spacing)
  {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates &
  operator=(const ArrayPortalUniformPointCoordinates &src)
  {
    this->Extent = src.Extent;
    this->Dimensions = src.Dimensions;
    this->NumberOfValues = src.NumberOfValues;
    this->Origin = src.Origin;
    this->Spacing = src.Spacing;
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id index) const {
    return this->GetCoordinatesForTopologyIndex(
          vtkm::ExtentPointFlatIndexToTopologyIndex(index, this->Extent));
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 GetRange3() const { return this->Dimensions; }

  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id3 index) const {
    return this->GetCoordinatesForTopologyIndex(index + this->Extent.Min);
  }

private:
  vtkm::Extent3 Extent;
  vtkm::Id3 Dimensions;
  vtkm::Id NumberOfValues;
  ValueType Origin;
  ValueType Spacing;

  VTKM_EXEC_CONT_EXPORT
  ValueType GetCoordinatesForTopologyIndex(vtkm::Id3 ijk) const {
    return ValueType(this->Origin[0] + this->Spacing[0] * static_cast<vtkm::FloatDefault>(ijk[0]),
                     this->Origin[1] + this->Spacing[1] * static_cast<vtkm::FloatDefault>(ijk[1]),
                     this->Origin[2] + this->Spacing[2] * static_cast<vtkm::FloatDefault>(ijk[2]));
  }
};

} // namespace internal

/// ArrayHandleUniformPointCoordinates is a specialization of ArrayHandle. It
/// contains the information necessary to compute the point coordinates in a
/// uniform orthogonal grid (extent, origin, and spacing) and implicitly
/// computes these coordinates in its array portal.
///
class ArrayHandleUniformPointCoordinates
    : public vtkm::cont::ArrayHandle<
        vtkm::Vec<vtkm::FloatDefault,3>,
        vtkm::cont::StorageTagImplicit<
          internal::ArrayPortalUniformPointCoordinates> >
{
public:
  typedef vtkm::Vec<vtkm::FloatDefault,3> ValueType;

private:
  typedef vtkm::cont::StorageTagImplicit<
      internal::ArrayPortalUniformPointCoordinates> StorageTag;

  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

  typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> Superclass;

public:
  VTKM_CONT_EXPORT
  ArrayHandleUniformPointCoordinates() : Superclass() {  }

  VTKM_CONT_EXPORT
  ArrayHandleUniformPointCoordinates(vtkm::Extent3 extent,
                                     ValueType origin,
                                     ValueType spacing)
    : Superclass(
        StorageType(internal::ArrayPortalUniformPointCoordinates(extent,
                                                                 origin,
                                                                 spacing)))
  {  }
};

}
} // namespace vtkm::cont

#endif //vtk_+m_cont_ArrayHandleUniformPointCoordinates_h
