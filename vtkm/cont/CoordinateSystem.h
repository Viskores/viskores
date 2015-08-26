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
#ifndef vtk_m_cont_CoordinateSystem_h
#define vtk_m_cont_CoordinateSystem_h

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/Field.h>

#ifndef VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG
#define VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG \
  ::vtkm::TypeListTagFieldVec3
#endif

#ifndef VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG
#define VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG \
  ::vtkm::cont::StorageListTagCoordinateSystemDefault
#endif

namespace vtkm {
namespace cont {

/// \brief Default storage list for CoordinateSystem arrays.
///
/// \c VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG is set to this value
/// by default (unless it is defined before including VTK-m headers.
///
struct StorageListTagCoordinateSystemDefault
    : vtkm::ListTagJoin<
        VTKM_DEFAULT_STORAGE_LIST_TAG,
        vtkm::ListTagBase<vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag> >
{ };

typedef vtkm::cont::DynamicArrayHandleBase<
    VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG,
    VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG>
  DynamicArrayHandleCoordinateSystem;

class CoordinateSystem : public vtkm::cont::Field
{
  typedef vtkm::cont::Field Superclass;

public:
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const vtkm::cont::DynamicArrayHandle &data)
    : Superclass(name, order, ASSOC_POINTS, data) {  }

  template<typename T, typename Storage>
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const ArrayHandle<T, Storage> &data)
    : Superclass(name, order, ASSOC_POINTS, data) {  }

  template<typename T>
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const std::vector<T> &data)
    : Superclass(name, order, ASSOC_POINTS, data) {  }

  template<typename T>
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const T *data,
                   vtkm::Id numberOfValues)
    : Superclass(name, order, ASSOC_POINTS, data, numberOfValues) {  }

  /// This constructor of coordinate system sets up a regular grid of points.
  ///
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   vtkm::Id3 dimensions,
                   vtkm::Vec<vtkm::FloatDefault,3> origin
                     = vtkm::Vec<vtkm::FloatDefault,3>(0.0f, 0.0f, 0.0f),
                   vtkm::Vec<vtkm::FloatDefault,3> spacing
                     = vtkm::Vec<vtkm::FloatDefault,3>(1.0f, 1.0f, 1.0f))
    : Superclass(name,
                 order,
                 ASSOC_POINTS,
                 vtkm::cont::DynamicArrayHandle(
                   vtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing)))
  {  }

  VTKM_CONT_EXPORT
  vtkm::cont::DynamicArrayHandleCoordinateSystem GetData() const
  {
    return vtkm::cont::DynamicArrayHandleCoordinateSystem(
          this->Superclass::GetData());
  }

  VTKM_CONT_EXPORT
  vtkm::cont::DynamicArrayHandleCoordinateSystem GetData()
  {
    return vtkm::cont::DynamicArrayHandleCoordinateSystem(
          this->Superclass::GetData());
  }

  template<typename DeviceAdapterTag, typename TypeList>
  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Float64>& GetBounds(DeviceAdapterTag,
                                                          TypeList) const
  {
    return this->Superclass::GetBounds(
          DeviceAdapterTag(),
          TypeList(),
          VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag, typename TypeList>
  VTKM_CONT_EXPORT
  void GetBounds(vtkm::Float64 *bounds, DeviceAdapterTag, TypeList) const
  {
    this->Superclass::GetBounds(
          bounds, DeviceAdapterTag(),
          TypeList(),
          VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Float64>& GetBounds(DeviceAdapterTag) const
  {
    return this->Superclass::GetBounds(
          DeviceAdapterTag(),
          VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG(),
          VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  void GetBounds(vtkm::Float64 *bounds, DeviceAdapterTag) const
  {
    this->Superclass::GetBounds(
          bounds,
          DeviceAdapterTag(),
          VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG(),
          VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
  }

  VTKM_CONT_EXPORT
  virtual void PrintSummary(std::ostream &out) const
  {
    out << "    Coordinate System ";
    this->PrintSummary(out);
  }
};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_CoordinateSystem_h


