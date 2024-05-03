//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_ExtractGeometry_h
#define vtkm_m_worklet_ExtractGeometry_h

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/ImplicitFunction.h>

namespace vtkm
{
namespace worklet
{

class ExtractGeometry
{
public:
  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify cells within volume of interest
  class ExtractCellsByVOI : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn coordinates,
                                  ExecObject implicitFunction,
                                  FieldOutCell passFlags);
    using ExecutionSignature = _4(PointCount, PointIndices, _2, _3);

    VTKM_CONT
    ExtractCellsByVOI(bool extractInside, bool extractBoundaryCells, bool extractOnlyBoundaryCells)
      : ExtractInside(extractInside)
      , ExtractBoundaryCells(extractBoundaryCells)
      , ExtractOnlyBoundaryCells(extractOnlyBoundaryCells)
    {
    }

    template <typename ConnectivityInVec, typename InVecFieldPortalType, typename ImplicitFunction>
    VTKM_EXEC bool operator()(vtkm::Id numIndices,
                              const ConnectivityInVec& connectivityIn,
                              const InVecFieldPortalType& coordinates,
                              const ImplicitFunction& function) const
    {
      // Count points inside/outside volume of interest
      vtkm::IdComponent inCnt = 0;
      vtkm::IdComponent outCnt = 0;
      vtkm::Id indx;
      for (indx = 0; indx < numIndices; indx++)
      {
        vtkm::Id ptId = connectivityIn[static_cast<vtkm::IdComponent>(indx)];
        vtkm::Vec<FloatDefault, 3> coordinate = coordinates.Get(ptId);
        vtkm::FloatDefault value = function.Value(coordinate);
        if (value <= 0)
          inCnt++;
        if (value >= 0)
          outCnt++;
      }

      // Decide if cell is extracted
      bool passFlag = false;
      if (inCnt == numIndices && ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (outCnt == numIndices && !ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (inCnt > 0 && outCnt > 0 && (ExtractBoundaryCells || ExtractOnlyBoundaryCells))
      {
        passFlag = true;
      }
      return passFlag;
    }

  private:
    bool ExtractInside;
    bool ExtractBoundaryCells;
    bool ExtractOnlyBoundaryCells;
  };

  class AddPermutationCellSet
  {
    vtkm::cont::UnknownCellSet* Output;
    vtkm::cont::ArrayHandle<vtkm::Id>* ValidIds;

  public:
    AddPermutationCellSet(vtkm::cont::UnknownCellSet& cellOut,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validIds)
      : Output(&cellOut)
      , ValidIds(&validIds)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellset) const
    {
      vtkm::cont::CellSetPermutation<CellSetType> permCellSet(*this->ValidIds, cellset);
      *this->Output = permCellSet;
    }
  };

  template <typename CellSetType, typename ImplicitFunction>
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                    const vtkm::cont::CoordinateSystem& coordinates,
                                    const ImplicitFunction& implicitFunction,
                                    bool extractInside,
                                    bool extractBoundaryCells,
                                    bool extractOnlyBoundaryCells)
  {
    // Worklet output will be a boolean passFlag array
    vtkm::cont::ArrayHandle<bool> passFlags;

    ExtractCellsByVOI worklet(extractInside, extractBoundaryCells, extractOnlyBoundaryCells);
    vtkm::cont::Invoker invoke;
    invoke(worklet, cellSet, coordinates, implicitFunction, passFlags);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::Algorithm::CopyIf(indices, passFlags, this->ValidCellIds);

    // generate the cellset
    vtkm::cont::CellSetPermutation<CellSetType> permutedCellSet(this->ValidCellIds, cellSet);

    vtkm::cont::CellSetExplicit<> outputCells;
    return vtkm::worklet::CellDeepCopy::Run(permutedCellSet);
  }

  vtkm::cont::ArrayHandle<vtkm::Id> GetValidCellIds() const { return this->ValidCellIds; }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractGeometry_h
