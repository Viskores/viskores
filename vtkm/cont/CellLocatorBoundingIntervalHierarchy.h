//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
#define vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/VirtualObjectHandle.h>

#include <vtkm/worklet/spatialstructure/BoundingIntervalHierarchy.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocatorBoundingIntervalHierarchy : public vtkm::cont::CellLocator
{
public:
  VTKM_CONT
  CellLocatorBoundingIntervalHierarchy(vtkm::IdComponent numPlanes = 4,
                                       vtkm::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Nodes()
    , ProcessedCellIds()
  {
  }

  VTKM_CONT ~CellLocatorBoundingIntervalHierarchy() override;

  VTKM_CONT
  void SetNumberOfSplittingPlanes(vtkm::IdComponent numPlanes)
  {
    this->NumPlanes = numPlanes;
    this->SetModified();
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfSplittingPlanes() { return this->NumPlanes; }

  VTKM_CONT
  void SetMaxLeafSize(vtkm::IdComponent maxLeafSize)
  {
    this->MaxLeafSize = maxLeafSize;
    this->SetModified();
  }

  VTKM_CONT
  vtkm::Id GetMaxLeafSize() { return this->MaxLeafSize; }

  VTKM_CONT
  const vtkm::exec::CellLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) const override;

protected:
  VTKM_CONT
  void Build() override;

private:
  vtkm::IdComponent NumPlanes;
  vtkm::IdComponent MaxLeafSize;
  vtkm::cont::ArrayHandle<vtkm::exec::CellLocatorBoundingIntervalHierarchyNode> Nodes;
  vtkm::cont::ArrayHandle<vtkm::Id> ProcessedCellIds;

  mutable vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator> ExecutionObjectHandle;
};

} // namespace cont
} // namespace vtkm

#endif // vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
