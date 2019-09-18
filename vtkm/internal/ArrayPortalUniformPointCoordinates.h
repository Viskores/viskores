//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_ArrayPortalUniformPointCoordinates_h
#define vtk_m_internal_ArrayPortalUniformPointCoordinates_h

#include <vtkm/Assert.h>
#include <vtkm/Types.h>

namespace vtkm
{
namespace internal
{

/// \brief An implicit array port that computes point coordinates for a uniform grid.
///
class VTKM_ALWAYS_EXPORT ArrayPortalUniformPointCoordinates
{
public:
  using ValueType = vtkm::Vec3f;

  VTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates()
    : Dimensions(0)
    , NumberOfValues(0)
    , Origin(0, 0, 0)
    , Spacing(1, 1, 1)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates(vtkm::Id3 dimensions, ValueType origin, ValueType spacing)
    : Dimensions(dimensions)
    , NumberOfValues(dimensions[0] * dimensions[1] * dimensions[2])
    , Origin(origin)
    , Spacing(spacing)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates(const ArrayPortalUniformPointCoordinates& src)
    : Dimensions(src.Dimensions)
    , NumberOfValues(src.NumberOfValues)
    , Origin(src.Origin)
    , Spacing(src.Spacing)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates& operator=(const ArrayPortalUniformPointCoordinates& src)
  {
    this->Dimensions = src.Dimensions;
    this->NumberOfValues = src.NumberOfValues;
    this->Origin = src.Origin;
    this->Spacing = src.Spacing;
    return *this;
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());
    return this->Get(vtkm::Id3(index % this->Dimensions[0],
                               (index / this->Dimensions[0]) % this->Dimensions[1],
                               index / (this->Dimensions[0] * this->Dimensions[1])));
  }

  VTKM_EXEC_CONT
  vtkm::Id3 GetRange3() const { return this->Dimensions; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id3 index) const
  {
    VTKM_ASSERT((index[0] >= 0) && (index[1] >= 0) && (index[2] >= 0));
    VTKM_ASSERT((index[0] < this->Dimensions[0]) && (index[1] < this->Dimensions[1]) &&
                (index[2] < this->Dimensions[2]));
    return ValueType(this->Origin[0] + this->Spacing[0] * static_cast<vtkm::FloatDefault>(index[0]),
                     this->Origin[1] + this->Spacing[1] * static_cast<vtkm::FloatDefault>(index[1]),
                     this->Origin[2] +
                       this->Spacing[2] * static_cast<vtkm::FloatDefault>(index[2]));
  }

  VTKM_EXEC_CONT
  const vtkm::Id3& GetDimensions() const { return this->Dimensions; }

  VTKM_EXEC_CONT
  const ValueType& GetOrigin() const { return this->Origin; }

  VTKM_EXEC_CONT
  const ValueType& GetSpacing() const { return this->Spacing; }

private:
  vtkm::Id3 Dimensions = { 0, 0, 0 };
  vtkm::Id NumberOfValues = 0;
  ValueType Origin = { 0.0f, 0.0f, 0.0f };
  ValueType Spacing = { 0.0f, 0.0f, 0.0f };
};
}
} // namespace vtkm::internal

#endif //vtk_m_internal_ArrayPortalUniformPointCoordinates_h
