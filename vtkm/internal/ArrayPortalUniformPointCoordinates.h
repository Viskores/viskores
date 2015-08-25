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
#ifndef vtk_m_internal_ArrayPortalUniformPointCoordinates_h
#define vtk_m_internal_ArrayPortalUniformPointCoordinates_h

#include <vtkm/Types.h>

namespace vtkm {
namespace internal {

/// \brief An implicit array port that computes point coordinates for a uniform grid.
///
class ArrayPortalUniformPointCoordinates
{
public:
  typedef vtkm::Vec<vtkm::FloatDefault,3> ValueType;

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates() : NumberOfValues(0) {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates(vtkm::Id3 dimensions,
                                     ValueType origin,
                                     ValueType spacing)
    : Dimensions(dimensions),
      NumberOfValues(dimensions[0]*dimensions[1]*dimensions[2]),
      Origin(origin),
      Spacing(spacing)
  {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates(
      const ArrayPortalUniformPointCoordinates &src)
    : Dimensions(src.Dimensions),
      NumberOfValues(src.NumberOfValues),
      Origin(src.Origin),
      Spacing(src.Spacing)
  {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalUniformPointCoordinates &
  operator=(const ArrayPortalUniformPointCoordinates &src)
  {
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
    return this->Get(
          vtkm::Id3(index%this->Dimensions[0],
                    (index/this->Dimensions[0])%this->Dimensions[1],
                    index/(this->Dimensions[0]*this->Dimensions[1])));
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 GetRange3() const { return this->Dimensions; }

  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id3 index) const {
    return ValueType(this->Origin[0] + this->Spacing[0] * static_cast<vtkm::FloatDefault>(index[0]),
                     this->Origin[1] + this->Spacing[1] * static_cast<vtkm::FloatDefault>(index[1]),
                     this->Origin[2] + this->Spacing[2] * static_cast<vtkm::FloatDefault>(index[2]));
  }

  VTKM_EXEC_CONT_EXPORT
  const vtkm::Id3 &GetDimensions() const { return this->Dimensions; }

  VTKM_EXEC_CONT_EXPORT
  const ValueType &GetOrigin() const { return this->Origin; }

  VTKM_EXEC_CONT_EXPORT
  const ValueType &GetSpacing() const { return this->Spacing; }

private:
  vtkm::Id3 Dimensions;
  vtkm::Id NumberOfValues;
  ValueType Origin;
  ValueType Spacing;
};

}
} // namespace vtkm::internal

#endif //vtk_m_internal_ArrayPortalUniformPointCoordinates_h
