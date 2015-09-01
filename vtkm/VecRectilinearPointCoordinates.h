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
#ifndef vtk_m_exec_VecRectilinearPointCoordinates_h
#define vtk_m_exec_VecRectilinearPointCoordinates_h

#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

namespace vtkm {

namespace detail {

/// Specifies the size of VecRectilinearPointCoordinates for the given
/// dimension.
///
template<vtkm::IdComponent NumDimensions>
struct VecRectilinearPointCoordinatesNumComponents;

template<>
struct VecRectilinearPointCoordinatesNumComponents<1>
{
  static const vtkm::IdComponent NUM_COMPONENTS = 2;
};

template<>
struct VecRectilinearPointCoordinatesNumComponents<2>
{
  static const vtkm::IdComponent NUM_COMPONENTS = 4;
};

template<>
struct VecRectilinearPointCoordinatesNumComponents<3>
{
  static const vtkm::IdComponent NUM_COMPONENTS = 8;
};

VTKM_EXEC_CONSTANT_EXPORT
const vtkm::FloatDefault VecRectilinearPointCoordinatesOffsetTable[8][3] = {
  { 0.0f, 0.0f, 0.0f },
  { 1.0f, 0.0f, 0.0f },
  { 1.0f, 1.0f, 0.0f },
  { 0.0f, 1.0f, 0.0f },
  { 0.0f, 0.0f, 1.0f },
  { 1.0f, 0.0f, 1.0f },
  { 1.0f, 1.0f, 1.0f },
  { 0.0f, 1.0f, 1.0f }
};

} // namespace detail

/// \brief An implicit vector for point coordinates in rectilinear cells.
///
/// The \C VecRectilinearPointCoordinates class is a Vec-like class that holds
/// the point coordinates for a rectilinear cell. The class is templated on the
/// dimensions of the cell, which can be 1 (for a line), 2 (for a pixel), or 3
/// (for a voxel).
///
template<vtkm::IdComponent NumDimensions>
class VecRectilinearPointCoordinates
{
public:
  typedef vtkm::Vec<vtkm::FloatDefault,3> ComponentType;

  static const vtkm::IdComponent NUM_COMPONENTS =
      detail::VecRectilinearPointCoordinatesNumComponents<NumDimensions>::NUM_COMPONENTS;

  VTKM_EXEC_CONT_EXPORT
  VecRectilinearPointCoordinates(ComponentType origin = ComponentType(0,0,0),
                                 ComponentType spacing = ComponentType(1,1,1))
    : Origin(origin), Spacing(spacing) {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfComponents() const { return NUM_COMPONENTS; }

  template<vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT_EXPORT
  void CopyInto(vtkm::Vec<ComponentType,DestSize> &dest) const
  {
    vtkm::IdComponent numComponents =
        vtkm::Min(DestSize, this->GetNumberOfComponents());
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VTKM_EXEC_CONT_EXPORT
  ComponentType operator[](vtkm::IdComponent index) const
  {
    const vtkm::FloatDefault *offset =
        detail::VecRectilinearPointCoordinatesOffsetTable[index];
    return ComponentType(this->Origin[0] + offset[0]*this->Spacing[0],
                         this->Origin[1] + offset[1]*this->Spacing[1],
                         this->Origin[2] + offset[2]*this->Spacing[2]);
  }

  VTKM_EXEC_CONT_EXPORT
  const ComponentType &GetOrigin() const { return this->Origin; }

  VTKM_EXEC_CONT_EXPORT
  const ComponentType &GetSpacing() const { return this->Spacing; }

private:
  // Position of lower left point.
  ComponentType Origin;

  // Spacing in the x, y, and z directions.
  ComponentType Spacing;
};

template<vtkm::IdComponent NumDimensions>
struct TypeTraits<vtkm::VecRectilinearPointCoordinates<NumDimensions> >
{
  typedef vtkm::TypeTraitsRealTag NumericTag;
  typedef TypeTraitsVectorTag DimensionalityTag;

  VTKM_EXEC_CONT_EXPORT
  static vtkm::VecRectilinearPointCoordinates<NumDimensions>
  ZeroInitialization()
  {
    return vtkm::VecRectilinearPointCoordinates<NumDimensions>(
          vtkm::Vec<vtkm::FloatDefault,3>(0,0,0),
          vtkm::Vec<vtkm::FloatDefault,3>(0,0,0));
  }
};

template<vtkm::IdComponent NumDimensions>
struct VecTraits<vtkm::VecRectilinearPointCoordinates<NumDimensions> >
{
  typedef vtkm::VecRectilinearPointCoordinates<NumDimensions> VecType;

  typedef vtkm::Vec<vtkm::FloatDefault,3> ComponentType;
  typedef vtkm::VecTraitsTagMultipleComponents HasMultipleComponents;
  typedef vtkm::VecTraitsTagSizeStatic IsSizeStatic;

  static const vtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  VTKM_EXEC_CONT_EXPORT
  static vtkm::IdComponent GetNumberOfComponents(const VecType &) {
    return NUM_COMPONENTS;
  }

  VTKM_EXEC_CONT_EXPORT
  static ComponentType GetComponent(const VecType &vector,
                                    vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  template<vtkm::IdComponent destSize>
  VTKM_EXEC_CONT_EXPORT
  static void CopyInto(const VecType &src,
                       vtkm::Vec<ComponentType,destSize> &dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace vtkm

#endif //vtk_m_exec_VecRectilinearPointCoordinates_h
