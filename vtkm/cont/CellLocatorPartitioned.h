//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_cont_CellLocatorPartitioned_h
#define vtkm_cont_CellLocatorPartitioned_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/exec/CellLocatorPartitioned.h>

namespace vtkm
{
namespace cont
{
class VTKM_CONT_EXPORT CellLocatorPartitioned : public vtkm::cont::ExecutionObjectBase
{

public:
  virtual ~CellLocatorPartitioned() = default;

  VTKM_CONT CellLocatorPartitioned() = default;

  void SetPartitions(const vtkm::cont::PartitionedDataSet& partitions)
  {
    this->Partitions = partitions;
    this->SetModified();
  }
  const vtkm::cont::PartitionedDataSet& GetPartitions() const { return this->Partitions; }

  void Update();

  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  void Build();

  VTKM_CONT const vtkm::exec::CellLocatorPartitioned PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token);

private:
  vtkm::cont::PartitionedDataSet Partitions;
  std::vector<CellLocatorGeneral> LocatorsCont;
  std::vector<vtkm::cont::ArrayHandleStride<vtkm::UInt8>> GhostsCont;
  vtkm::cont::ArrayHandle<vtkm::cont::CellLocatorGeneral::ExecObjType> LocatorsExec;
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandleStride<vtkm::UInt8>::ReadPortalType> GhostsExec;
  bool Modified = true;
};
} // namespace cont
} //namespace vtkm

#endif //vtkm_cont_CellLocatorPartitioned_h
