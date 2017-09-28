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

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/Field.h>

#ifndef VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG
#define VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG ::vtkm::TypeListTagFieldVec3
#endif

#ifndef VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG
#define VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG                                            \
  ::vtkm::cont::StorageListTagCoordinateSystemDefault
#endif

namespace vtkm
{
namespace cont
{

namespace detail
{

using ArrayHandleCompositeVectorFloat32_3Default =
  vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32>,
                                             vtkm::cont::ArrayHandle<vtkm::Float32>>::type;

using ArrayHandleCompositeVectorFloat64_3Default =
  vtkm::cont::ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<vtkm::Float64>,
                                             vtkm::cont::ArrayHandle<vtkm::Float64>,
                                             vtkm::cont::ArrayHandle<vtkm::Float64>>::type;

} // namespace detail

/// \brief Default storage list for CoordinateSystem arrays.
///
/// \c VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG is set to this value
/// by default (unless it is defined before including VTK-m headers.
///
struct StorageListTagCoordinateSystemDefault
  : vtkm::ListTagBase<vtkm::cont::StorageTagBasic,
                      vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                      detail::ArrayHandleCompositeVectorFloat32_3Default::StorageTag,
                      detail::ArrayHandleCompositeVectorFloat64_3Default::StorageTag,
                      vtkm::cont::ArrayHandleCartesianProduct<
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault>>::StorageTag>
{
};

using DynamicArrayHandleCoordinateSystem =
  vtkm::cont::DynamicArrayHandleBase<VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG,
                                     VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG>;

class VTKM_CONT_EXPORT CoordinateSystem : public vtkm::cont::Field
{
  using Superclass = vtkm::cont::Field;

public:
  VTKM_CONT
  CoordinateSystem()
    : Superclass()
  {
  }

  VTKM_CONT
  CoordinateSystem(std::string name, const vtkm::cont::DynamicArrayHandle& data)
    : Superclass(name, ASSOC_POINTS, data)
  {
  }

  template <typename T, typename Storage>
  VTKM_CONT CoordinateSystem(std::string name, const ArrayHandle<T, Storage>& data)
    : Superclass(name, ASSOC_POINTS, data)
  {
  }

  template <typename T>
  VTKM_CONT CoordinateSystem(std::string name, const std::vector<T>& data)
    : Superclass(name, ASSOC_POINTS, data)
  {
  }

  template <typename T>
  VTKM_CONT CoordinateSystem(std::string name, const T* data, vtkm::Id numberOfValues)
    : Superclass(name, ASSOC_POINTS, data, numberOfValues)
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
                 vtkm::cont::DynamicArrayHandle(
                   vtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing)))
  {
  }

  VTKM_CONT
  CoordinateSystem& operator=(const vtkm::cont::CoordinateSystem& src) = default;

  VTKM_CONT
  vtkm::cont::DynamicArrayHandleCoordinateSystem GetData() const
  {
    return vtkm::cont::DynamicArrayHandleCoordinateSystem(this->Superclass::GetData());
  }

  VTKM_CONT
  vtkm::cont::DynamicArrayHandleCoordinateSystem GetData()
  {
    return vtkm::cont::DynamicArrayHandleCoordinateSystem(this->Superclass::GetData());
  }

  VTKM_CONT
  void GetRange(vtkm::Range* range) const;

  template <typename TypeList>
  VTKM_CONT void GetRange(vtkm::Range* range, TypeList) const
  {
    VTKM_IS_LIST_TAG(TypeList);

    this->Superclass::GetRange(
      range, TypeList(), VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
  }

  template <typename TypeList, typename StorageList>
  VTKM_CONT void GetRange(vtkm::Range* range, TypeList, StorageList) const
  {
    VTKM_IS_LIST_TAG(TypeList);
    VTKM_IS_LIST_TAG(StorageList);

    this->Superclass::GetRange(range, TypeList(), StorageList());
  }

  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange() const;

  template <typename TypeList>
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(TypeList) const
  {
    VTKM_IS_LIST_TAG(TypeList);

    return this->Superclass::GetRange(TypeList(),
                                      VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
  }

  template <typename TypeList, typename StorageList>
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(TypeList, StorageList) const
  {
    VTKM_IS_LIST_TAG(TypeList);
    VTKM_IS_LIST_TAG(StorageList);

    return this->Superclass::GetRange(TypeList(), StorageList());
  }

  VTKM_CONT
  vtkm::Bounds GetBounds() const;

  template <typename TypeList>
  VTKM_CONT vtkm::Bounds GetBounds(TypeList) const
  {
    VTKM_IS_LIST_TAG(TypeList);

    return this->GetBounds(TypeList(), VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
  }

  template <typename TypeList, typename StorageList>
  VTKM_CONT vtkm::Bounds GetBounds(TypeList, StorageList) const
  {
    VTKM_IS_LIST_TAG(TypeList);
    VTKM_IS_LIST_TAG(StorageList);

    vtkm::cont::ArrayHandle<vtkm::Range> ranges = this->GetRange(TypeList(), StorageList());

    VTKM_ASSERT(ranges.GetNumberOfValues() == 3);

    vtkm::cont::ArrayHandle<vtkm::Range>::PortalConstControl rangePortal =
      ranges.GetPortalConstControl();

    return vtkm::Bounds(rangePortal.Get(0), rangePortal.Get(1), rangePortal.Get(2));
  }

  virtual void PrintSummary(std::ostream& out) const;
};

template <typename Functor>
void CastAndCall(const vtkm::cont::CoordinateSystem& coords, const Functor& f)
{
  coords.GetData().CastAndCall(f);
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
