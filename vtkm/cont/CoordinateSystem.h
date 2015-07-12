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

#include <vtkm/Types.h>
#include <vector>
#include <string>
#include <ostream>

namespace vtkm {
namespace cont {

class CoordinateSystem
{
public:
  struct CoordinateAxis
  {
    std::string       FieldName;
    vtkm::IdComponent FieldComponent;
    VTKM_CONT_EXPORT
    CoordinateAxis(const std::string &name,
                   vtkm::IdComponent componentIndex = 0)
      : FieldName(name), FieldComponent(componentIndex)
    {
    }
  };

  VTKM_CONT_EXPORT
  CoordinateSystem(std::string nx,
                   std::string ny,
                   std::string nz)
  {
    this->Axes.push_back(CoordinateAxis(nx));
    this->Axes.push_back(CoordinateAxis(ny));
    this->Axes.push_back(CoordinateAxis(nz));
  }

  VTKM_CONT_EXPORT
  CoordinateSystem(std::string nx,
                   std::string ny)
  {
    this->Axes.push_back(CoordinateAxis(nx));
    this->Axes.push_back(CoordinateAxis(ny));
  }

  VTKM_CONT_EXPORT
  CoordinateSystem(std::string nx)
  {
    this->Axes.push_back(CoordinateAxis(nx));
  }

  VTKM_CONT_EXPORT
  void PrintSummary(std::ostream &out) const
  {
    out<<"   {";
    for (std::size_t i = 0; i < this->Axes.size(); i++)
    {
      out<<this->Axes[i].FieldName<<"["<<this->Axes[i].FieldComponent<<"]";
      if (i < this->Axes.size()-1)
      {
        out<<", ";
      }
    }
    out<<"}\n";
  }

  private:
    std::vector<CoordinateAxis> Axes;
};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_CoordinateSystem_h


