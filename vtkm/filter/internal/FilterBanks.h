//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
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
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#ifndef vtk_m_filter_internal_filterbanks_h
#define vtk_m_filter_internal_filterbanks_h


#include <vtkm/Types.h>

namespace vtkm {
namespace filter {

namespace internal {

  const vtkm::Float64 hm4_44[9] = {
    0.037828455507264,
    -0.023849465019557,
    -0.110624404418437,
    0.377402855612831,
    0.852698679008894,
    0.377402855612831,
    -0.110624404418437,
    -0.023849465019557,
    0.037828455507264
  };

  const vtkm::Float64 h4[9] = {
    0.0,
    -0.064538882628697,
    -0.040689417609164,
    0.418092273221617,
    0.788485616405583,
    0.418092273221617,
    -0.0406894176091641,
    -0.0645388826286971,
    0.0
  };

  const double hm2_22[6] = {
    -0.1767766952966368811002110905262,
    0.3535533905932737622004221810524,
    1.0606601717798212866012665431573,
    0.3535533905932737622004221810524,
    -0.1767766952966368811002110905262
  };

  const double h2[18] = {
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.3535533905932737622004221810524,
    0.7071067811865475244008443621048,
    0.3535533905932737622004221810524,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0
  };

};

}
}

#endif
