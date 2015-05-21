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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_PointCoordinatesListTag_h
#define vtk_m_cont_PointCoordinatesListTag_h

#ifndef VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG
#define VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG \
  ::vtkm::cont::PointCoordinatesListTagCommon
#endif
#include <vtkm/ListTag.h>

#include <vtkm/cont/PointCoordinatesArray.h>
#include <vtkm/cont/PointCoordinatesUniform.h>

namespace vtkm {
namespace cont {

struct PointCoordinatesListTagArray :
    vtkm::ListTagBase<vtkm::cont::PointCoordinatesArray> {  };
struct PointCoordinatesListTagUniform :
    vtkm::ListTagBase<vtkm::cont::PointCoordinatesUniform> {  };

/// A list of the most commonly used point coordinate types. Includes \c
/// PointCoordinatesArray.
///
struct PointCoordinatesListTagCommon
    : vtkm::ListTagBase<
        vtkm::cont::PointCoordinatesArray,
        vtkm::cont::PointCoordinatesUniform>
{  };

}
} // namespace vtkm::cont

#endif //vtk_m_cont_PointCoordinatesListTag_h
