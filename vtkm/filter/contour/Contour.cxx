//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/contour/ContourFlyingEdges.h>
#include <vtkm/filter/contour/ContourMarchingCells.h>

namespace vtkm
{
namespace filter
{

using SupportedTypes = vtkm::List<vtkm::UInt8, vtkm::Int8, vtkm::Float32, vtkm::Float64>;

namespace contour
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet Contour::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  // Switch between Marching Cubes and Flying Edges implementation of contour,
  // depending on the type of CellSet we are processing

  vtkm::cont::UnknownCellSet inCellSet = inDataSet.GetCellSet();
  auto inCoords = inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()).GetData();
  std::unique_ptr<vtkm::filter::contour::AbstractContour> implementation;

  // Flying Edges is only used for 3D Structured CellSets
  if (inCellSet.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Using flying edges");
    implementation.reset(new vtkm::filter::contour::ContourFlyingEdges);
    implementation->SetComputeFastNormals(this->GetComputeFastNormals());
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Using marching cells");
    implementation.reset(new vtkm::filter::contour::ContourMarchingCells);
    implementation->SetComputeFastNormals(this->GetComputeFastNormals());
  }

  implementation->SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());
  implementation->SetGenerateNormals(this->GetGenerateNormals());
  implementation->SetAddInterpolationEdgeIds(this->GetAddInterpolationEdgeIds());
  implementation->SetNormalArrayName(this->GetNormalArrayName());
  implementation->SetActiveField(this->GetActiveFieldName());
  implementation->SetFieldsToPass(this->GetFieldsToPass());
  implementation->SetNumberOfIsoValues(this->GetNumberOfIsoValues());
  for (int i = 0; i < this->GetNumberOfIsoValues(); i++)
  {
    implementation->SetIsoValue(i, this->GetIsoValue(i));
  }

  return implementation->Execute(inDataSet);
}
} // namespace contour
} // namespace filter
} // namespace vtkm
