//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/CellLocatorPartitioned.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/exec/CellLocatorPartitioned.h>

namespace vtkm
{
namespace cont
{

void CellLocatorPartitioned::Update()
{
  if (this->Modified)
  {
    this->Build();
    this->Modified = false;
  }
}


void CellLocatorPartitioned::Build()
{
  vtkm::Id numPartitions = this->Partitions.GetNumberOfPartitions();
  this->LocatorsCont.resize(numPartitions);
  this->GhostsCont.resize(numPartitions);
  for (vtkm::Id index = 0; index < numPartitions; ++index)
  {
    const vtkm::cont::DataSet& dataset = this->Partitions.GetPartition(index);

    // fill vector of cellLocators
    vtkm::cont::CellLocatorGeneral cellLocator;
    cellLocator.SetCellSet(dataset.GetCellSet());
    cellLocator.SetCoordinates(dataset.GetCoordinateSystem());
    cellLocator.Update();
    this->LocatorsCont.at(index) = cellLocator;

    // fill vector of ghostFields
    this->GhostsCont.at(index) =
      dataset.GetGhostCellField().GetData().ExtractComponent<vtkm::UInt8>(0);
  }
}

const vtkm::exec::CellLocatorPartitioned CellLocatorPartitioned::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token)
{
  this->Update();

  vtkm::Id numPartitions = this->Partitions.GetNumberOfPartitions();
  this->LocatorsExec.Allocate(numPartitions, vtkm::CopyFlag::Off, token);
  auto portalLocators = this->LocatorsExec.WritePortal(token);
  this->GhostsExec.Allocate(numPartitions, vtkm::CopyFlag::Off, token);
  auto portalGhosts = this->GhostsExec.WritePortal(token);
  for (vtkm::Id index = 0; index < numPartitions; ++index)
  {
    // fill arrayhandle of cellLocators
    portalLocators.Set(index, this->LocatorsCont.at(index).PrepareForExecution(device, token));

    // fill arrayhandle of ghostFields
    portalGhosts.Set(index, this->GhostsCont.at(index).PrepareForInput(device, token));
  }
  return vtkm::exec::CellLocatorPartitioned(this->LocatorsExec.PrepareForInput(device, token),
                                            this->GhostsExec.PrepareForInput(device, token));
}

} // namespace cont
} //namespace vtkm
