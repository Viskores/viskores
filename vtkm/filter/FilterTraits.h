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

#ifndef vtk_m_filter_FilterTraits_h
#define vtk_m_filter_FilterTraits_h

#include <vtkm/TypeListTag.h>

namespace vtkm {
namespace filter {
template<typename Filter>
class FilterTraits
{
public:
  //A filter can specify a set of data type that it supports for
  //the input array
  typedef vtkm::TypeListTagCommon InputFieldTypeList;
};

}
}

#endif //vtk_m_filter_FilterTraits_h
