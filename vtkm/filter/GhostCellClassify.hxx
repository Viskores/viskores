//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_GhostCellClassify_hxx
#define vtk_m_filter_GhostCellClassify_hxx

#include <vtkm/CellClassification.h>
#include <vtkm/RangeId.h>
#include <vtkm/RangeId2.h>
#include <vtkm/RangeId3.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/worklet/WorkletPointNeighborhood.h>

namespace
{
struct TypeUInt8 : vtkm::List<vtkm::UInt8>
{
};

class SetStructuredGhostCells1D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  SetStructuredGhostCells1D(vtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary, vtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers);
    value = (notOnBoundary) ? vtkm::CellClassification::NORMAL : vtkm::CellClassification::GHOST;
  }

private:
  vtkm::IdComponent NumLayers;
};

class SetStructuredGhostCells2D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  SetStructuredGhostCells2D(vtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary, vtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers) &&
      boundary.IsRadiusInYBoundary(this->NumLayers);
    value = (notOnBoundary) ? vtkm::CellClassification::NORMAL : vtkm::CellClassification::GHOST;
  }

private:
  vtkm::IdComponent NumLayers;
};

class SetStructuredGhostCells3D : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  SetStructuredGhostCells3D(vtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary, vtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInBoundary(this->NumLayers);
    value = (notOnBoundary) ? vtkm::CellClassification::NORMAL : vtkm::CellClassification::GHOST;
  }

private:
  vtkm::IdComponent NumLayers;
};
};

namespace vtkm
{
namespace filter
{

inline VTKM_CONT GhostCellClassify::GhostCellClassify()
{
}

template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet GhostCellClassify::DoExecute(const vtkm::cont::DataSet& input,
                                                                  vtkm::filter::PolicyBase<Policy>)
{
  const vtkm::cont::DynamicCellSet& cellset = input.GetCellSet();
  vtkm::cont::ArrayHandle<vtkm::UInt8> ghosts;
  const vtkm::Id numCells = cellset.GetNumberOfCells();

  //Structured cases are easy...
  if (cellset.template IsType<vtkm::cont::CellSetStructured<1>>())
  {
    if (numCells <= 2)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    vtkm::cont::CellSetStructured<1> cellset1d = cellset.Cast<vtkm::cont::CellSetStructured<1>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    vtkm::cont::CellSetStructured<3> dual;
    const auto dim = cellset1d.GetCellDimensions();
    dual.SetPointDimensions(vtkm::Id3{ dim, 1, 1 });
    this->Invoke(SetStructuredGhostCells1D{}, dual, ghosts);
  }
  else if (cellset.template IsType<vtkm::cont::CellSetStructured<2>>())
  {
    if (numCells <= 4)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    vtkm::cont::CellSetStructured<2> cellset2d = cellset.Cast<vtkm::cont::CellSetStructured<2>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    vtkm::cont::CellSetStructured<3> dual;
    const auto dims = cellset2d.GetCellDimensions();
    dual.SetPointDimensions(vtkm::Id3{ dims[0], dims[1], 1 });
    this->Invoke(SetStructuredGhostCells2D{}, dual, ghosts);
  }
  else if (cellset.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    if (numCells <= 8)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    vtkm::cont::CellSetStructured<3> cellset3d = cellset.Cast<vtkm::cont::CellSetStructured<3>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    vtkm::cont::CellSetStructured<3> dual;
    dual.SetPointDimensions(cellset3d.GetCellDimensions());
    this->Invoke(SetStructuredGhostCells3D{}, dual, ghosts);
  }
  else
  {
    throw vtkm::cont::ErrorFilterExecution("Unsupported cellset type for GhostCellClassify.");
  }

  return CreateResultFieldCell(input, ghosts, "vtkmGhostCells");
}
}
}
#endif
