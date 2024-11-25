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
#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/UncertainArrayHandle.h>

namespace vtkm
{
namespace cont
{

/// @brief Manages a coordinate system for a `DataSet`.
///
/// A coordinate system is really a field with a special meaning, so `CoordinateSystem`
/// class inherits from the `Field` class. `CoordinateSystem` constrains the field to
/// be associated with points and typically has 3D floating point vectors for values.
class VTKM_CONT_EXPORT CoordinateSystem : public vtkm::cont::Field
{
  using Superclass = vtkm::cont::Field;

public:
  VTKM_CONT
  CoordinateSystem();

  // It's OK for regular _point_ fields to become a CoordinateSystem object.
  VTKM_CONT CoordinateSystem(const vtkm::cont::Field& src);

  VTKM_CONT CoordinateSystem(std::string name, const vtkm::cont::UnknownArrayHandle& data);

  template <typename T, typename Storage>
  VTKM_CONT CoordinateSystem(std::string name, const ArrayHandle<T, Storage>& data)
    : Superclass(name, Association::Points, data)
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

  VTKM_CONT vtkm::cont::UncertainArrayHandle<vtkm::TypeListFieldVec3, VTKM_DEFAULT_STORAGE_LIST>
  GetData() const;

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
  void GetRange(vtkm::Range* range) const { this->Superclass::GetRange(range); }

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
    return this->Superclass::GetRange();
  }

  VTKM_CONT
  vtkm::Bounds GetBounds() const
  {
    vtkm::Range ranges[3];
    this->GetRange(ranges);
    return vtkm::Bounds(ranges[0], ranges[1], ranges[2]);
  }

  void PrintSummary(std::ostream& out, bool full = false) const override;

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
struct Serialization<vtkm::cont::CoordinateSystem> : Serialization<vtkm::cont::Field>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_CoordinateSystem_h
