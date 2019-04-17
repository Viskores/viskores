//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleUniformPointCoordinates_h
#define vtk_m_cont_ArrayHandleUniformPointCoordinates_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>
#include <vtkm/internal/ArrayPortalUniformPointCoordinates.h>

namespace vtkm
{
namespace cont
{

/// ArrayHandleUniformPointCoordinates is a specialization of ArrayHandle. It
/// contains the information necessary to compute the point coordinates in a
/// uniform orthogonal grid (extent, origin, and spacing) and implicitly
/// computes these coordinates in its array portal.
///
class VTKM_ALWAYS_EXPORT ArrayHandleUniformPointCoordinates
  : public vtkm::cont::ArrayHandle<
      vtkm::Vec<vtkm::FloatDefault, 3>,
      vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS_NT(
    ArrayHandleUniformPointCoordinates,
    (vtkm::cont::ArrayHandle<
      vtkm::Vec<vtkm::FloatDefault, 3>,
      vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleUniformPointCoordinates(vtkm::Id3 dimensions,
                                     ValueType origin = ValueType(0.0f, 0.0f, 0.0f),
                                     ValueType spacing = ValueType(1.0f, 1.0f, 1.0f))
    : Superclass(StorageType(
        vtkm::internal::ArrayPortalUniformPointCoordinates(dimensions, origin, spacing)))
  {
  }
};
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
namespace vtkm
{
namespace cont
{

template <>
struct SerializableTypeString<vtkm::cont::ArrayHandleUniformPointCoordinates>
{
  static VTKM_CONT const std::string Get() { return "AH_UniformPointCoordinates"; }
};

template <>
struct SerializableTypeString<vtkm::cont::ArrayHandle<
  vtkm::Vec<vtkm::FloatDefault, 3>,
  vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleUniformPointCoordinates>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<vtkm::cont::ArrayHandleUniformPointCoordinates>
{
private:
  using Type = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto portal = obj.GetPortalConstControl();
    vtkmdiy::save(bb, portal.GetDimensions());
    vtkmdiy::save(bb, portal.GetOrigin());
    vtkmdiy::save(bb, portal.GetSpacing());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Id3 dims;
    typename BaseType::ValueType origin, spacing;

    vtkmdiy::load(bb, dims);
    vtkmdiy::load(bb, origin);
    vtkmdiy::load(bb, spacing);

    obj = vtkm::cont::ArrayHandleUniformPointCoordinates(dims, origin, spacing);
  }
};

template <>
struct Serialization<vtkm::cont::ArrayHandle<
  vtkm::Vec<vtkm::FloatDefault, 3>,
  vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>>>
  : Serialization<vtkm::cont::ArrayHandleUniformPointCoordinates>
{
};

} // diy

#endif //vtk_+m_cont_ArrayHandleUniformPointCoordinates_h
