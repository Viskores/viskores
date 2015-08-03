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

#ifndef vtk_m_worklet_CellAverage_h
#define vtk_m_worklet_CellAverage_h

#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/exec/TopologyData.h>

namespace vtkm {
namespace worklet {

//simple functor that returns the average point value.
class CellAverage : public vtkm::worklet::WorkletMapTopology
{
  static const int LEN_IDS = 8;
public:
  typedef void ControlSignature(FieldSrcIn<Scalar> inPoints,
                                TopologyIn<LEN_IDS> topology,
                                FieldDestOut<Scalar> outCells);
  typedef void ExecutionSignature(_1, FromCount, _3);
  typedef _2 InputDomain;

  template<typename T1, typename T2>
  VTKM_EXEC_EXPORT
  void operator()(const vtkm::exec::TopologyData<T1,LEN_IDS> &pointValues,
                  const vtkm::Id &count,
                  T2 &average) const
  {
    T1 sum = pointValues[0];
    for (vtkm::IdComponent i=1; i< count; ++i)
      {
      sum += pointValues[i];
      }

    average = static_cast<T2>(sum / static_cast<T1>(count));
  }

};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_CellAverage_h
