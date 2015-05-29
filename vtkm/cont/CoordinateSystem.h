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
        CoordinateAxis(const std::string &n,
                       vtkm::IdComponent c = 0)
            : FieldName(n), FieldComponent(c)
        {
        }
    };

    CoordinateSystem(std::string nx,
                     std::string ny,
                     std::string nz)
    {
        axes.push_back(CoordinateAxis(nx));
        axes.push_back(CoordinateAxis(ny));
        axes.push_back(CoordinateAxis(nz));
    }

    CoordinateSystem(std::string nx,
                     std::string ny)
    {
        axes.push_back(CoordinateAxis(nx));
        axes.push_back(CoordinateAxis(ny));
    }

    CoordinateSystem(std::string nx)
    {
        axes.push_back(CoordinateAxis(nx));
    }

    void PrintSummary(std::ostream &out)
    {
	out<<"   {";
	for (std::size_t i = 0; i < axes.size(); i++)
	{
	    out<<axes[i].FieldName<<"["<<axes[i].FieldComponent<<"]";
	    if (i < axes.size()-1) out<<", ";
	}
	out<<"}\n";
    }

  private:
    std::vector<CoordinateAxis> axes;
};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_CoordinateSystem_h


