//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_PointAverage_h
#define vtk_m_worklet_PointAverage_h

#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/VecTraits.h>

namespace vtkm {
namespace worklet {

//simple functor that returns the average point value of a given
//cell based field.
class PointAverage :
        public vtkm::worklet::WorkletMapCellToPoint
{
public:
  typedef void ControlSignature(CellSetIn cellset,
                                FieldInCell<> inCellField,
                                FieldOutPoint<> outPointField);
  typedef void ExecutionSignature(CellCount, _2, _3);
  typedef _1 InputDomain;

  template<typename CellValueVecType, typename OutType>
  VTKM_EXEC
  void operator()(const vtkm::IdComponent &numCells,
                  const CellValueVecType &cellValues,
                  OutType &average) const
  {
    using CellValueType = typename CellValueVecType::ComponentType;
    using InVecSize =
        std::integral_constant<
          vtkm::IdComponent,
          vtkm::VecTraits<CellValueType>::NUM_COMPONENTS>;
    using OutVecSize =
        std::integral_constant<
          vtkm::IdComponent,
          vtkm::VecTraits<OutType>::NUM_COMPONENTS>;

    this->DoAverage(numCells,
                    cellValues,
                    average,
                    InVecSize(),
                    OutVecSize());
  }

private:
  template<typename CellValueVecType, typename OutType>
  VTKM_EXEC
  void DoAverage(const vtkm::IdComponent &numCells,
                 const CellValueVecType &cellValues,
                 OutType &average,
                 std::integral_constant<vtkm::IdComponent,1>,
                 std::integral_constant<vtkm::IdComponent,1>) const
  {
    OutType sum = static_cast<OutType>(cellValues[0]);
    for (vtkm::IdComponent cellIndex = 1; cellIndex < numCells; ++cellIndex)
      {
      sum = sum + static_cast<OutType>(cellValues[cellIndex]);
      }

    average = sum / static_cast<OutType>(numCells);
  }

  template<typename CellValueVecType,
           typename OutType,
           vtkm::IdComponent VecSize>
  VTKM_EXEC
  void DoAverage(const vtkm::IdComponent &numCells,
                 const CellValueVecType &cellValues,
                 OutType &average,
                 std::integral_constant<vtkm::IdComponent,VecSize>,
                 std::integral_constant<vtkm::IdComponent,VecSize>) const
  {
    using OutComponentType = typename vtkm::VecTraits<OutType>::ComponentType;
    OutType sum = OutType(cellValues[0]);
    for (vtkm::IdComponent cellIndex = 1; cellIndex < numCells; ++cellIndex)
      {
      sum = sum + OutType(cellValues[cellIndex]);
      }

    average = sum / OutType(static_cast<OutComponentType>(numCells));
  }

  template<typename CellValueVecType,
           typename OutType,
           vtkm::IdComponent InVecSize,
           vtkm::IdComponent OutVecSize>
  VTKM_EXEC
  void DoAverage(const vtkm::IdComponent &vtkmNotUsed(numCells),
                 const CellValueVecType &vtkmNotUsed(cellValues),
                 OutType &vtkmNotUsed(average),
                 std::integral_constant<vtkm::IdComponent,InVecSize>,
                 std::integral_constant<vtkm::IdComponent,OutVecSize>) const
  {
    this->RaiseError(
          "PointAverage called with mismatched Vec sizes for PointAverage.");
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_PointAverage_h
