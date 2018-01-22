//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CoordinateSystem_h
#define vtk_m_cont_CoordinateSystem_h

#include <vtkm/Bounds.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

struct MakeArrayHandleVirtualCoordinatesFunctor
{
  VTKM_CONT explicit MakeArrayHandleVirtualCoordinatesFunctor(
    vtkm::cont::ArrayHandleVirtualCoordinates& out)
    : Out(&out)
  {
  }

  template <typename StorageTag>
  VTKM_CONT void operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>, StorageTag>& array) const
  {
    *this->Out = vtkm::cont::ArrayHandleVirtualCoordinates(array);
  }

  template <typename StorageTag>
  VTKM_CONT void operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>, StorageTag>& array) const
  {
    *this->Out = vtkm::cont::ArrayHandleVirtualCoordinates(array);
  }

  template <typename T, typename StorageTag>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, StorageTag>&) const
  {
    throw vtkm::cont::ErrorBadType("CoordinateSystem's value type should be a 3 component Vec "
                                   "of either vtkm::Float32 or vtkm::Float64");
  }

  vtkm::cont::ArrayHandleVirtualCoordinates* Out;
};

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandleVirtualCoordinates MakeArrayHandleVirtualCoordinates(
  const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& array)
{
  vtkm::cont::ArrayHandleVirtualCoordinates out;
  array.CastAndCall(MakeArrayHandleVirtualCoordinatesFunctor(out));
  return out;
}

} // namespace detail

class VTKM_CONT_EXPORT CoordinateSystem : public vtkm::cont::Field
{
  using Superclass = vtkm::cont::Field;

public:
  VTKM_CONT
  CoordinateSystem()
    : Superclass()
  {
  }

  VTKM_CONT CoordinateSystem(std::string name,
                             const vtkm::cont::ArrayHandleVirtualCoordinates::Superclass& data)
    : Superclass(name, ASSOC_POINTS, data)
  {
  }

  template <typename TypeList, typename StorageList>
  VTKM_CONT CoordinateSystem(std::string name,
                             const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& data)
    : Superclass(name, ASSOC_POINTS, detail::MakeArrayHandleVirtualCoordinates(data))
  {
  }

  template <typename T, typename Storage>
  VTKM_CONT CoordinateSystem(std::string name, const ArrayHandle<T, Storage>& data)
    : Superclass(name, ASSOC_POINTS, vtkm::cont::ArrayHandleVirtualCoordinates(data))
  {
  }

  /// This constructor of coordinate system sets up a regular grid of points.
  ///
  VTKM_CONT
  CoordinateSystem(
    std::string name,
    vtkm::Id3 dimensions,
    vtkm::Vec<vtkm::FloatDefault, 3> origin = vtkm::Vec<vtkm::FloatDefault, 3>(0.0f, 0.0f, 0.0f),
    vtkm::Vec<vtkm::FloatDefault, 3> spacing = vtkm::Vec<vtkm::FloatDefault, 3>(1.0f, 1.0f, 1.0f))
    : Superclass(name,
                 ASSOC_POINTS,
                 vtkm::cont::ArrayHandleVirtualCoordinates(
                   vtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing)))
  {
  }

  VTKM_CONT
  CoordinateSystem& operator=(const vtkm::cont::CoordinateSystem& src) = default;

  VTKM_CONT
  vtkm::cont::ArrayHandleVirtualCoordinates GetData() const
  {
    return this->Superclass::GetData().Cast<vtkm::cont::ArrayHandleVirtualCoordinates>();
  }

  VTKM_CONT void SetData(const vtkm::cont::ArrayHandleVirtualCoordinates::Superclass& newdata)
  {
    this->Superclass::SetData(newdata);
  }

  template <typename T, typename StorageTag>
  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<T, StorageTag>& newdata)
  {
    this->Superclass::SetData(vtkm::cont::ArrayHandleVirtualCoordinates(newdata));
  }

  VTKM_CONT
  template <typename TypeList, typename StorageList>
  void SetData(const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& newdata)
  {
    this->Superclass::SetData(detail::MakeArrayHandleVirtualCoordinates(newdata));
  }

  VTKM_CONT
  void GetRange(vtkm::Range* range) const;

  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange() const;

  VTKM_CONT
  vtkm::Bounds GetBounds() const;

  virtual void PrintSummary(std::ostream& out) const;
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

#endif //vtk_m_cont_CoordinateSystem_h
