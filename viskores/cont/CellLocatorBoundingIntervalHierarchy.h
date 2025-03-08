//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_cont_CellLocatorBoundingIntervalHierarchy_h
#define viskores_cont_CellLocatorBoundingIntervalHierarchy_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/cont/CellLocatorBase.h>

#include <viskores/exec/CellLocatorBoundingIntervalHierarchy.h>
#include <viskores/exec/CellLocatorMultiplexer.h>

namespace viskores
{
namespace cont
{

class VISKORES_CONT_EXPORT CellLocatorBoundingIntervalHierarchy
  : public viskores::cont::CellLocatorBase
{
public:
  using SupportedCellSets = VISKORES_DEFAULT_CELL_SET_LIST;

  using CellLocatorExecList =
    viskores::ListTransform<SupportedCellSets,
                            viskores::exec::CellLocatorBoundingIntervalHierarchy>;

  using ExecObjType =
    viskores::ListApply<CellLocatorExecList, viskores::exec::CellLocatorMultiplexer>;
  using LastCell = typename ExecObjType::LastCell;

  VISKORES_CONT
  CellLocatorBoundingIntervalHierarchy(viskores::IdComponent numPlanes = 4,
                                       viskores::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Nodes()
    , ProcessedCellIds()
  {
  }

  VISKORES_CONT
  void SetNumberOfSplittingPlanes(viskores::IdComponent numPlanes)
  {
    this->NumPlanes = numPlanes;
    this->SetModified();
  }

  VISKORES_CONT
  viskores::IdComponent GetNumberOfSplittingPlanes() { return this->NumPlanes; }

  VISKORES_CONT
  void SetMaxLeafSize(viskores::IdComponent maxLeafSize)
  {
    this->MaxLeafSize = maxLeafSize;
    this->SetModified();
  }

  VISKORES_CONT
  viskores::Id GetMaxLeafSize() { return this->MaxLeafSize; }

  VISKORES_CONT ExecObjType PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                viskores::cont::Token& token) const;

private:
  viskores::IdComponent NumPlanes;
  viskores::IdComponent MaxLeafSize;
  viskores::cont::ArrayHandle<viskores::exec::CellLocatorBoundingIntervalHierarchyNode> Nodes;
  viskores::cont::ArrayHandle<viskores::Id> ProcessedCellIds;

  VISKORES_CONT void Build() override;

  struct MakeExecObject;
};

} // namespace cont
} // namespace viskores

#endif // viskores_cont_CellLocatorBoundingIntervalHierarchy_h
