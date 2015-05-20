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
#ifndef vtk_m_cont_CellSet_h
#define vtk_m_cont_CellSet_h

#include <vtkm/CellType.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/LogicalStructure.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

class CellSet
{
public:
  CellSet(const std::string &n, int d)
    : name(n), dimensionality(d), logicalStructure(NULL)
  {
  }

  virtual ~CellSet()
  {
  }

  virtual std::string GetName()
  {
    return name;
  }
  virtual int GetDimensionality()
  {
    return dimensionality;
  }

  virtual int GetNumCells() = 0;

  virtual int GetNumFaces()
  {
    return 0;
  }

  virtual int GetNumEdges()
  {
    return 0;
  }

  virtual void PrintSummary(std::ostream&) = 0;

protected:
    std::string name;
    int dimensionality;
    vtkm::cont::LogicalStructure *logicalStructure;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSet_h
