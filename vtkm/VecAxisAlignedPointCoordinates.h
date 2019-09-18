//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_VecAxisAlignedPointCoordinates_h
#define vtk_m_VecAxisAlignedPointCoordinates_h

#include <vtkm/Math.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{

namespace detail
{

/// Specifies the size of VecAxisAlignedPointCoordinates for the given
/// dimension.
///
template <vtkm::IdComponent NumDimensions>
struct VecAxisAlignedPointCoordinatesNumComponents;

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<1>
{
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 2;
};

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<2>
{
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 4;
};

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<3>
{
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 8;
};

struct VecAxisAlignedPointCoordinatesOffsetTable
{
  VTKM_EXEC_CONT vtkm::FloatDefault Get(vtkm::Int32 i, vtkm::Int32 j) const
  {
    VTKM_STATIC_CONSTEXPR_ARRAY vtkm::FloatDefault offsetTable[8][3] = {
      { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }
    };
    return offsetTable[i][j];
  }
};

} // namespace detail

/// \brief An implicit vector for point coordinates in axis aligned cells. For
/// internal use only.
///
/// The \c VecAxisAlignedPointCoordinates class is a Vec-like class that holds
/// the point coordinates for a axis aligned cell. The class is templated on the
/// dimensions of the cell, which can be 1 (for a line).
///
/// This is an internal class used to represent coordinates for uniform datasets
/// in an execution environment when executing a WorkletMapPointToCell. Users
/// should not directly construct this class under any circumstances. Use the
/// related ArrayPortalUniformPointCoordinates and
/// ArrayHandleUniformPointCoordinates classes instead.
///
template <vtkm::IdComponent NumDimensions>
class VecAxisAlignedPointCoordinates
{
public:
  using ComponentType = vtkm::Vec3f;

  static constexpr vtkm::IdComponent NUM_COMPONENTS =
    detail::VecAxisAlignedPointCoordinatesNumComponents<NumDimensions>::NUM_COMPONENTS;

  VTKM_EXEC_CONT
  VecAxisAlignedPointCoordinates(ComponentType origin = ComponentType(0, 0, 0),
                                 ComponentType spacing = ComponentType(1, 1, 1))
    : Origin(origin)
    , Spacing(spacing)
  {
  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const { return NUM_COMPONENTS; }

  template <vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT void CopyInto(vtkm::Vec<ComponentType, DestSize>& dest) const
  {
    vtkm::IdComponent numComponents = vtkm::Min(DestSize, this->GetNumberOfComponents());
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VTKM_EXEC_CONT
  ComponentType operator[](vtkm::IdComponent index) const
  {
    detail::VecAxisAlignedPointCoordinatesOffsetTable table;
    return ComponentType(this->Origin[0] + table.Get(index, 0) * this->Spacing[0],
                         this->Origin[1] + table.Get(index, 1) * this->Spacing[1],
                         this->Origin[2] + table.Get(index, 2) * this->Spacing[2]);
  }

  VTKM_EXEC_CONT
  const ComponentType& GetOrigin() const { return this->Origin; }

  VTKM_EXEC_CONT
  const ComponentType& GetSpacing() const { return this->Spacing; }

private:
  // Position of lower left point.
  ComponentType Origin;

  // Spacing in the x, y, and z directions.
  ComponentType Spacing;
};

template <vtkm::IdComponent NumDimensions>
struct TypeTraits<vtkm::VecAxisAlignedPointCoordinates<NumDimensions>>
{
  using NumericTag = vtkm::TypeTraitsRealTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VTKM_EXEC_CONT
  static vtkm::VecAxisAlignedPointCoordinates<NumDimensions> ZeroInitialization()
  {
    return vtkm::VecAxisAlignedPointCoordinates<NumDimensions>(vtkm::Vec3f(0, 0, 0),
                                                               vtkm::Vec3f(0, 0, 0));
  }
};

template <vtkm::IdComponent NumDimensions>
struct VecTraits<vtkm::VecAxisAlignedPointCoordinates<NumDimensions>>
{
  using VecType = vtkm::VecAxisAlignedPointCoordinates<NumDimensions>;

  using ComponentType = vtkm::Vec3f;
  using BaseComponentType = vtkm::FloatDefault;
  using HasMultipleComponents = vtkm::VecTraitsTagMultipleComponents;
  using IsSizeStatic = vtkm::VecTraitsTagSizeStatic;

  static constexpr vtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType&) { return NUM_COMPONENTS; }

  VTKM_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, vtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  // These are a bit of a hack since VecAxisAlignedPointCoordinates only supports one component
  // type. Using these might not work as expected.
  template <typename NewComponentType>
  using ReplaceComponentType = vtkm::Vec<NewComponentType, NUM_COMPONENTS>;
  template <typename NewComponentType>
  using ReplaceBaseComponenttype = vtkm::Vec<vtkm::Vec<NewComponentType, 3>, NUM_COMPONENTS>;

  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace vtkm

#endif //vtk_m_VecAxisAlignedPointCoordinates_h
