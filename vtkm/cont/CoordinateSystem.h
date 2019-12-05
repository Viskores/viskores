//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CoordinateSystem_h
#define vtk_m_cont_CoordinateSystem_h

#include <vtkm/Bounds.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{
class VTKM_CONT_EXPORT CoordinateSystem : public vtkm::cont::Field
{
  using Superclass = vtkm::cont::Field;
  using CoordinatesTypeList = vtkm::List<vtkm::cont::ArrayHandleVirtualCoordinates::ValueType>;

public:
  VTKM_CONT
  CoordinateSystem();

  VTKM_CONT CoordinateSystem(std::string name,
                             const vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>& data);

  template <typename TypeList>
  VTKM_CONT CoordinateSystem(std::string name,
                             const vtkm::cont::VariantArrayHandleBase<TypeList>& data);

  template <typename T, typename Storage>
  VTKM_CONT CoordinateSystem(std::string name, const ArrayHandle<T, Storage>& data);

  /// This constructor of coordinate system sets up a regular grid of points.
  ///
  VTKM_CONT
  CoordinateSystem(std::string name,
                   vtkm::Id3 dimensions,
                   vtkm::Vec3f origin = vtkm::Vec3f(0.0f, 0.0f, 0.0f),
                   vtkm::Vec3f spacing = vtkm::Vec3f(1.0f, 1.0f, 1.0f));

  VTKM_CONT
  vtkm::Id GetNumberOfPoints() const { return this->GetNumberOfValues(); }

  VTKM_CONT
  vtkm::cont::ArrayHandleVirtualCoordinates GetData() const;

  VTKM_CONT void SetData(const vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>& newdata);

  template <typename T, typename Storage>
  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<T, Storage>& newdata);

  VTKM_CONT
  template <typename TypeList>
  void SetData(const vtkm::cont::VariantArrayHandleBase<TypeList>& newdata);

  VTKM_CONT
  void GetRange(vtkm::Range* range) const
  {
    this->Superclass::GetRange(range, CoordinatesTypeList());
  }

  VTKM_CONT
  vtkm::Vec<vtkm::Range, 3> GetRange() const
  {
    vtkm::Vec<vtkm::Range, 3> range;
    this->GetRange(&range[0]);
    return range;
  }

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Range> GetRangeAsArrayHandle() const
  {
    return this->Superclass::GetRange(CoordinatesTypeList());
  }

  VTKM_CONT
  vtkm::Bounds GetBounds() const
  {
    vtkm::Range ranges[3];
    this->GetRange(ranges);
    return vtkm::Bounds(ranges[0], ranges[1], ranges[2]);
  }

  virtual void PrintSummary(std::ostream& out) const override;

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  VTKM_CONT void ReleaseResourcesExecution() override
  {
    this->Superclass::ReleaseResourcesExecution();
    this->GetData().ReleaseResourcesExecution();
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const vtkm::cont::CoordinateSystem& coords, Functor&& f, Args&&... args)
{
  CastAndCall(coords.GetData(), std::forward<Functor>(f), std::forward<Args>(args)...);
}

template <typename T>
vtkm::cont::CoordinateSystem make_CoordinateSystem(std::string name,
                                                   const std::vector<T>& data,
                                                   vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::CoordinateSystem(name, vtkm::cont::make_ArrayHandle(data, copy));
}

template <typename T>
vtkm::cont::CoordinateSystem make_CoordinateSystem(std::string name,
                                                   const T* data,
                                                   vtkm::Id numberOfValues,
                                                   vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::CoordinateSystem(name,
                                      vtkm::cont::make_ArrayHandle(data, numberOfValues, copy));
}

namespace internal
{

template <>
struct DynamicTransformTraits<vtkm::cont::CoordinateSystem>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal
} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <>
struct Serialization<vtkm::cont::CoordinateSystem>
{
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::CoordinateSystem& cs)
  {
    vtkmdiy::save(bb, cs.GetName());
    vtkmdiy::save(bb, cs.GetData());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::CoordinateSystem& cs)
  {
    std::string name;
    vtkmdiy::load(bb, name);
    vtkm::cont::ArrayHandleVirtualCoordinates array;
    vtkmdiy::load(bb, array);
    cs = vtkm::cont::CoordinateSystem(name, array);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_CoordinateSystem_h
