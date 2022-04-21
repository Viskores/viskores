//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/CellClassification.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/mesh_info/GhostCellClassify.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

namespace vtkm
{
namespace filter
{
namespace detail
{

class SetStructuredGhostCells1D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  explicit SetStructuredGhostCells1D(vtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary, vtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers);
    value = (notOnBoundary) ? vtkm::CellClassification::Normal : vtkm::CellClassification::Ghost;
  }

private:
  vtkm::IdComponent NumLayers;
};

class SetStructuredGhostCells2D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  explicit SetStructuredGhostCells2D(vtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary, vtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers) &&
      boundary.IsRadiusInYBoundary(this->NumLayers);
    value = (notOnBoundary) ? vtkm::CellClassification::Normal : vtkm::CellClassification::Ghost;
  }

private:
  vtkm::IdComponent NumLayers;
};

class SetStructuredGhostCells3D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  explicit SetStructuredGhostCells3D(vtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary, vtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInBoundary(this->NumLayers);
    value = (notOnBoundary) ? vtkm::CellClassification::Normal : vtkm::CellClassification::Ghost;
  }

private:
  vtkm::IdComponent NumLayers;
};
} // namespace detail

namespace mesh_info
{
VTKM_CONT vtkm::cont::DataSet GhostCellClassify::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& cellset = input.GetCellSet();
  vtkm::cont::ArrayHandle<vtkm::UInt8> ghosts;
  const vtkm::Id numCells = cellset.GetNumberOfCells();

  //Structured cases are easy...
  if (cellset.template IsType<vtkm::cont::CellSetStructured<1>>())
  {
    if (numCells <= 2)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    vtkm::cont::CellSetStructured<1> cellset1d =
      cellset.AsCellSet<vtkm::cont::CellSetStructured<1>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    vtkm::cont::CellSetStructured<3> dual;
    const auto dim = cellset1d.GetCellDimensions();
    dual.SetPointDimensions(vtkm::Id3{ dim, 1, 1 });
    this->Invoke(vtkm::filter::detail::SetStructuredGhostCells1D{}, dual, ghosts);
  }
  else if (cellset.template IsType<vtkm::cont::CellSetStructured<2>>())
  {
    if (numCells <= 4)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    vtkm::cont::CellSetStructured<2> cellset2d =
      cellset.AsCellSet<vtkm::cont::CellSetStructured<2>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    vtkm::cont::CellSetStructured<3> dual;
    const auto dims = cellset2d.GetCellDimensions();
    dual.SetPointDimensions(vtkm::Id3{ dims[0], dims[1], 1 });
    this->Invoke(vtkm::filter::detail::SetStructuredGhostCells2D{}, dual, ghosts);
  }
  else if (cellset.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    if (numCells <= 8)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    vtkm::cont::CellSetStructured<3> cellset3d =
      cellset.AsCellSet<vtkm::cont::CellSetStructured<3>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    vtkm::cont::CellSetStructured<3> dual;
    dual.SetPointDimensions(cellset3d.GetCellDimensions());
    this->Invoke(vtkm::filter::detail::SetStructuredGhostCells3D{}, dual, ghosts);
  }
  else
  {
    throw vtkm::cont::ErrorFilterExecution("Unsupported cellset type for GhostCellClassify.");
  }

  auto output = this->CreateResult(input);
  output.AddCellField("vtkmGhostCells", ghosts);
  return output;
}
}
}
}
