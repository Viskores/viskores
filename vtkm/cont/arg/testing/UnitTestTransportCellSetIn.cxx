//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/arg/TransportTagCellSetIn.h>

#include <vtkm/cont/CellSetExplicit.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename CellSetInType>
struct TestKernel : public vtkm::exec::FunctorBase
{
  CellSetInType CellSet;

  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    if (this->CellSet.GetNumberOfElements() != 2)
    {
      this->RaiseError("Got bad number of shapes in exec cellset object.");
    }

    if (this->CellSet.GetIndices(0).GetNumberOfComponents() != 3 ||
        this->CellSet.GetIndices(1).GetNumberOfComponents() != 4)
    {
      this->RaiseError("Got bad number of Indices in exec cellset object.");
    }

    if (this->CellSet.GetCellShape(0).Id != 5 || this->CellSet.GetCellShape(1).Id != 9)
    {
      this->RaiseError("Got bad cell shape in exec cellset object.");
    }
  }
};

template <typename Device>
void TransportWholeCellSetIn(Device)
{
  //build a fake cell set
  const int nVerts = 5;
  vtkm::cont::CellSetExplicit<> contObject;
  contObject.PrepareToAddCells(2, 7);
  contObject.AddCell(vtkm::CELL_SHAPE_TRIANGLE, 3, vtkm::make_Vec<vtkm::Id>(0, 1, 2));
  contObject.AddCell(vtkm::CELL_SHAPE_QUAD, 4, vtkm::make_Vec<vtkm::Id>(2, 1, 3, 4));
  contObject.CompleteAddingCells(nVerts);

  using IncidentTopology = vtkm::TopologyElementTagPoint;
  using VisitTopology = vtkm::TopologyElementTagCell;

  using ExecObjectType = typename vtkm::cont::CellSetExplicit<>::
    template ExecutionTypes<Device, VisitTopology, IncidentTopology>::ExecObjectType;

  vtkm::cont::arg::Transport<
    vtkm::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>,
    vtkm::cont::CellSetExplicit<>,
    Device>
    transport;

  TestKernel<ExecObjectType> kernel;
  kernel.CellSet = transport(contObject, nullptr, 1, 1);

  vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, 1);
}

void UnitTestCellSetIn()
{
  std::cout << "Trying CellSetIn transport with serial device." << std::endl;
  TransportWholeCellSetIn(vtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportCellSetIn(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(UnitTestCellSetIn, argc, argv);
}
