//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/TriangleExtractor.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

void TriangleExtractor::ExtractCells(const vtkm::cont::DynamicCellSet& cells)
{
  vtkm::Id numberOfTriangles;
  vtkm::rendering::internal::RunTriangulator(cells, this->Triangles, numberOfTriangles);
}

vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> TriangleExtractor::GetTriangles()
{
  return this->Triangles;
}

vtkm::Id TriangleExtractor::GetNumberOfTriangles() const
{
  return this->Triangles.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
