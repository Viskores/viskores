//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_PointCoordinatesArray_h
#define vtk_m_cont_PointCoordinatesArray_h

#include <vtkm/TypeListTag.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/internal/PointCoordinatesBase.h>

namespace vtkm {
namespace cont {

/// \brief Point coordinates stored in a \c Vector3 array.
///
/// The \c PointCoordinatesArray class is a simple PointCoordinates class
/// that stores the point coordinates in a single array. The array is managed
/// by a \c DynamicArrayHandle.
///
/// Like other PointCoordinates classes, \c PointCoordinatesArray is intended
/// to be used in conjunction with \c DynamicPointCoordinates.
///
class PointCoordinatesArray : public internal::PointCoordinatesBase
{
public:
  VTKM_CONT_EXPORT
  PointCoordinatesArray() {  }

  VTKM_CONT_EXPORT
  PointCoordinatesArray(const vtkm::cont::DynamicArrayHandle &array)
    : Array(array) {  }

  template<typename Storage>
  VTKM_CONT_EXPORT
  PointCoordinatesArray(
      const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,3>,Storage> &array)
    : Array(array) {  }

  template<typename Storage>
  VTKM_CONT_EXPORT
  PointCoordinatesArray(
      const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64,3>,Storage> &array)
    : Array(array) {  }

  /// In this \c CastAndCall, \c TypeList is ignored. All point coordinates are
  /// expressed as Vector3, so that must be how the array is represented.
  ///
  template<typename Functor, typename TypeList, typename StorageList>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f, TypeList, StorageList) const
  {
    this->Array.CastAndCall(f, vtkm::TypeListTagVec3(), StorageList());
  }

private:
  vtkm::cont::DynamicArrayHandle Array;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_PointCoordinatesArray_h
