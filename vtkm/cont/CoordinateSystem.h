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
#include <vtkm/Deprecated.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/Field.h>

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#endif

namespace vtkm
{
namespace cont
{

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
namespace detail
{

// CoordinateSystem::GetData used to return an ArrayHandleVirtualCoordinates.
// That behavior is deprecated, and CoordianteSystem::GetData now returns a
// VariantArrayHandle similar (although slightly different than) its superclass.
// This wrapper class supports the old deprecated behavior until it is no longer
// supported. Once the behavior is removed (probably when
// ArrayHandleVirtualCoordinates is removed), then this class should be removed.
class VTKM_ALWAYS_EXPORT CoordDataDepWrapper
  : public vtkm::cont::VariantArrayHandleBase<vtkm::TypeListFieldVec3>
{
  using Superclass = vtkm::cont::VariantArrayHandleBase<vtkm::TypeListFieldVec3>;

  VTKM_DEPRECATED_SUPPRESS_BEGIN
  VTKM_CONT_EXPORT VTKM_CONT vtkm::cont::ArrayHandleVirtualCoordinates ToArray() const;
  VTKM_DEPRECATED_SUPPRESS_END

public:
  using Superclass::Superclass;

  // Make the return also behave as ArrayHandleVirtualCoordiantes
  VTKM_DEPRECATED_SUPPRESS_BEGIN

  VTKM_CONT VTKM_DEPRECATED(1.6, "CoordinateSystem::GetData() now returns a VariantArrayHandle.")
  operator vtkm::cont::ArrayHandleVirtualCoordinates() const
  {
    return this->ToArray();
  }

  VTKM_CONT VTKM_DEPRECATED(1.6, "CoordinateSystem::GetData() now returns a VariantArrayHandle.")
  operator vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagVirtual>() const
  {
    return this->ToArray();
  }

  using ValueType VTKM_DEPRECATED(1.6,
                                  "CoordinateSystem::GetData() now returns a VariantArrayHandle.") =
    vtkm::Vec3f;

  VTKM_CONT VTKM_DEPRECATED(1.6, "CoordinateSystem::GetData() now returns a VariantArrayHandle.")
    ArrayHandleVirtualCoordinates::ReadPortalType ReadPortal() const
  {
    return this->ToArray().ReadPortal();
  }

  VTKM_CONT VTKM_DEPRECATED(1.6, "CoordinateSystem::GetData() now returns a VariantArrayHandle.")
    ArrayHandleVirtualCoordinates::WritePortalType WritePortal() const
  {
    return this->ToArray().WritePortal();
  }

  template <typename Device>
  VTKM_CONT VTKM_DEPRECATED(1.6, "CoordinateSystem::GetData() now returns a VariantArrayHandle.")
    typename ArrayHandleVirtualCoordinates::ExecutionTypes<Device>::PortalConst
    PrepareForInput(Device device, vtkm::cont::Token& token) const
  {
    return this->ToArray().PrepareForInput(device, token);
  }

  template <typename Device>
  VTKM_CONT VTKM_DEPRECATED(1.6, "CoordinateSystem::GetData() now returns a VariantArrayHandle.")
    typename ArrayHandleVirtualCoordinates::ExecutionTypes<Device>::Portal
    PrepareForInPlace(Device device, vtkm::cont::Token& token) const
  {
    return this->ToArray().PrepareForInPlace(device, token);
  }

  template <typename Device>
  VTKM_CONT VTKM_DEPRECATED(1.6, "CoordinateSystem::GetData() now returns a VariantArrayHandle.")
    typename ArrayHandleVirtualCoordinates::ExecutionTypes<Device>::Portal
    PrepareForOutput(vtkm::Id numberOfValues, Device device, vtkm::cont::Token& token) const
  {
    return this->ToArray().PrepareForOutput(numberOfValues, device, token);
  }

  VTKM_DEPRECATED_SUPPRESS_END
};

} // namespace detail

VTKM_DEPRECATED_SUPPRESS_BEGIN
VTKM_CONT VTKM_DEPRECATED(
  1.6,
  "CoordinateSystem::GetData() now returns a "
  "VariantArrayHandle.") inline void printSummary_ArrayHandle(const detail::CoordDataDepWrapper&
                                                                array,
                                                              std::ostream& out,
                                                              bool full = false)
{
  vtkm::cont::ArrayHandleVirtualCoordinates coordArray = array;
  vtkm::cont::printSummary_ArrayHandle(coordArray, out, full);
}
VTKM_DEPRECATED_SUPPRESS_END
#endif //VTKM_NO_DEPRECATED_VIRTUAL

class VTKM_CONT_EXPORT CoordinateSystem : public vtkm::cont::Field
{
  using Superclass = vtkm::cont::Field;
  using CoordinatesTypeList = vtkm::List<vtkm::Vec3f_32, vtkm::Vec3f_64>;

public:
  VTKM_CONT
  CoordinateSystem();

  VTKM_CONT CoordinateSystem(std::string name, const vtkm::cont::VariantArrayHandleCommon& data);

  template <typename T, typename Storage>
  VTKM_CONT CoordinateSystem(std::string name, const ArrayHandle<T, Storage>& data)
    : Superclass(name, Association::POINTS, data)
  {
  }

  /// This constructor of coordinate system sets up a regular grid of points.
  ///
  VTKM_CONT
  CoordinateSystem(std::string name,
                   vtkm::Id3 dimensions,
                   vtkm::Vec3f origin = vtkm::Vec3f(0.0f, 0.0f, 0.0f),
                   vtkm::Vec3f spacing = vtkm::Vec3f(1.0f, 1.0f, 1.0f));

  VTKM_CONT
  vtkm::Id GetNumberOfPoints() const { return this->GetNumberOfValues(); }

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
  VTKM_CONT detail::CoordDataDepWrapper GetData() const;
#else
  VTKM_CONT vtkm::cont::VariantArrayHandleBase<vtkm::TypeListFieldVec3> GetData() const;
#endif

private:
#ifdef VTKM_USE_DOUBLE_PRECISION
  using FloatNonDefault = vtkm::Float32;
#else
  using FloatNonDefault = vtkm::Float64;
#endif
  using Vec3f_nd = vtkm::Vec<FloatNonDefault, 3>;

  struct StorageToArrayDefault
  {
    template <typename S>
    using IsInvalid = vtkm::cont::internal::IsInvalidArrayHandle<vtkm::Vec3f, S>;

    template <typename S>
    using Transform = vtkm::cont::ArrayHandle<vtkm::Vec3f, S>;
  };

  struct StorageToArrayNonDefault
  {
    template <typename S>
    using IsInvalid = vtkm::cont::internal::IsInvalidArrayHandle<Vec3f_nd, S>;

    template <typename S>
    using Transform =
      vtkm::cont::ArrayHandleCast<vtkm::Vec3f, vtkm::cont::ArrayHandle<Vec3f_nd, S>>;
  };

  using ArraysFloatDefault = vtkm::ListTransform<
    vtkm::ListRemoveIf<VTKM_DEFAULT_STORAGE_LIST, StorageToArrayDefault::IsInvalid>,
    StorageToArrayDefault::Transform>;
  using ArraysFloatNonDefault = vtkm::ListTransform<
    vtkm::ListRemoveIf<VTKM_DEFAULT_STORAGE_LIST, StorageToArrayNonDefault::IsInvalid>,
    StorageToArrayNonDefault::Transform>;

public:
  using MultiplexerArrayType = //
    vtkm::cont::ArrayHandleMultiplexerFromList<
      vtkm::ListAppend<ArraysFloatDefault, ArraysFloatNonDefault>>;

  /// \brief Returns the data for the coordinate system as an `ArrayHandleMultiplexer`.
  ///
  /// This array will handle all potential types supported by CoordinateSystem, so all types can be
  /// handled with one compile pass. However, using this precludes specialization for special
  /// arrays such as `ArrayHandleUniformPointCoordinates` that could have optimized code paths
  ///
  VTKM_CONT MultiplexerArrayType GetDataAsMultiplexer() const;

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

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
template <>
struct DynamicTransformTraits<vtkm::cont::detail::CoordDataDepWrapper>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};
#endif //VTKM_NO_DEPRECATED_VIRTUAL


} // namespace internal
} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
template <>
struct Serialization<vtkm::cont::detail::CoordDataDepWrapper>
  : public Serialization<
      vtkm::cont::VariantArrayHandleBase<vtkm::List<vtkm::Vec3f_32, vtkm::Vec3f_64>>>
{
};
#endif //VTKM_NO_DEPRECATED_VIRTUAL

template <>
struct Serialization<vtkm::cont::CoordinateSystem>
{
  using CoordinatesTypeList = vtkm::List<vtkm::Vec3f_32, vtkm::Vec3f_64>;

  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::CoordinateSystem& cs)
  {
    vtkmdiy::save(bb, cs.GetName());
    vtkmdiy::save(
      bb, static_cast<vtkm::cont::VariantArrayHandleBase<CoordinatesTypeList>>(cs.GetData()));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::CoordinateSystem& cs)
  {
    std::string name;
    vtkmdiy::load(bb, name);
    vtkm::cont::VariantArrayHandleBase<CoordinatesTypeList> data;
    vtkmdiy::load(bb, data);
    cs = vtkm::cont::CoordinateSystem(name, data);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_CoordinateSystem_h
