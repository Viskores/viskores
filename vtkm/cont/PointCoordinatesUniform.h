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
#ifndef vtk_m_cont_PointCoordinatesUniform_h
#define vtk_m_cont_PointCoordinatesUniform_h

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/cont/internal/PointCoordinatesBase.h>

namespace vtkm {
namespace cont {

/// \brief Implicitly defined uniform point coordinates.
///
/// The \c PointCoordinatesUniform class is a PointCoordinates class that
/// implicitly defines the points for a uniform rectilinear grid of data
/// (defined by an extent, an origin, and spacing in each dimension).
///
/// Like other PointCoordinates classes, \c PointCoordinatesArray is intended
/// to be used in conjunction with \c DynamicPointCoordinates.
///
class PointCoordinatesUniform : public internal::PointCoordinatesBase
{
public:
  VTKM_CONT_EXPORT
  PointCoordinatesUniform() {  }

  VTKM_CONT_EXPORT
  PointCoordinatesUniform(const vtkm::Extent3 &extent,
                          const vtkm::Vector3 &origin,
                          const vtkm::Vector3 &spacing)
    : Array(extent, origin, spacing)
  {  }

  /// In this \c CastAndCall, both \c TypeList and \c ContainerList are
  /// ignored. All point coordinates are expressed as Vector3, so that must be
  /// how the array is represented.
  ///
  template<typename Functor, typename TypeList, typename ContainerList>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f, TypeList, ContainerList) const
  {
    f(this->Array);
  }

private:
  vtkm::cont::ArrayHandleUniformPointCoordinates Array;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_PointCoordinatesUniform_h
