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
#ifndef vtk_m_exec_TopologyData_h
#define vtk_m_exec_TopologyData_h

#include <vtkm/Types.h>

namespace vtkm {
namespace exec {

template<typename T, vtkm::IdComponent ITEM_TUPLE_LENGTH = 8>
class TopologyData
{
public:
  VTKM_EXEC_EXPORT T &operator[](vtkm::Id index) { return vec[index]; }
  VTKM_EXEC_EXPORT const T &operator[](vtkm::Id index) const { return vec[index]; }

  VTKM_EXEC_EXPORT TopologyData()
  {
  }
  template <typename T2>
  VTKM_EXEC_EXPORT TopologyData(const TopologyData<T2,ITEM_TUPLE_LENGTH> &other)
      : vec(other.vec)
  {
  }

  vtkm::Vec<T, ITEM_TUPLE_LENGTH> vec;
};


}
} // namespace vtkm::exec

#endif //vtk_m_exec_FunctorBase_h
