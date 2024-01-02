//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Field.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/TriangleExtractor.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

void TriangleExtractor::ExtractCells(const vtkm::cont::UnknownCellSet& cells)
{
  ExtractCells(
    cells,
    make_FieldCell(vtkm::cont::GetGlobalGhostCellFieldName(),
                   vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(0, cells.GetNumberOfCells())));
}

void TriangleExtractor::ExtractCells(const vtkm::cont::UnknownCellSet& cells,
                                     const vtkm::cont::Field& ghostField)
{
  vtkm::Id numberOfTriangles;
  vtkm::rendering::internal::RunTriangulator(cells, this->Triangles, numberOfTriangles, ghostField);
}

vtkm::cont::ArrayHandle<vtkm::Id4> TriangleExtractor::GetTriangles()
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
