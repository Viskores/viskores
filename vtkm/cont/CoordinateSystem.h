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

#include <vtkm/cont/Field.h>

namespace vtkm {
namespace cont {

class CoordinateSystem : public vtkm::cont::Field
{
public:
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const vtkm::cont::DynamicArrayHandle &data)
    : Field(name, order, ASSOC_POINTS, data) {  }

  template<typename T, typename Storage>
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const ArrayHandle<T, Storage> &data)
    : Field(name, order, ASSOC_POINTS, data) {  }

  template<typename T>
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const std::vector<T> &data)
    : Field(name, order, ASSOC_POINTS, data) {  }

  template<typename T>
  VTKM_CONT_EXPORT
  CoordinateSystem(std::string name,
                   vtkm::IdComponent order,
                   const T *data,
                   vtkm::Id numberOfValues)
    : Field(name, order, ASSOC_POINTS, data, numberOfValues) {  }

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


