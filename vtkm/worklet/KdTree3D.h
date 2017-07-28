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
#ifndef vtkm_m_worklet_KdTree3D_h
#define vtkm_m_worklet_KdTree3D_h

#include <vtkm/worklet/spatialstructure/KdTree3DConstruction.h>
#include <vtkm/worklet/spatialstructure/KdTree3DNNSearch.h>

namespace vtkm
{
namespace worklet
{

class KdTree3D
{
public:
  KdTree3D() {}

  // Execute the 3d kd tree construction given x y z coordinate vectors
  // Returns:
  // Leaf Node vector and internal node (split) vectpr
  template <typename CoordiType, typename TreeIdType, typename DeviceAdapter>
  void Run(vtkm::cont::ArrayHandle<vtkm::Vec<CoordiType, 3>>& coordi_Handle,
           vtkm::cont::ArrayHandle<TreeIdType>& pointId_Handle,
           vtkm::cont::ArrayHandle<TreeIdType>& splitId_Handle,
           DeviceAdapter device)
  {
    vtkm::worklet::spatialstructure::KdTree3DConstruction kdtree3DConstruction;
    kdtree3DConstruction.Run(coordi_Handle, pointId_Handle, splitId_Handle, device);
  }

  // Execute the Neaseat Neighbor Search given kdtree and search points
  // Returns:
  // Vectors of NN point index and NNpoint distance
  template <typename CoordiType, typename TreeIdType, typename DeviceAdapter>
  void Run(vtkm::cont::ArrayHandle<vtkm::Vec<CoordiType, 3>>& coordi_Handle,
           vtkm::cont::ArrayHandle<TreeIdType>& pointId_Handle,
           vtkm::cont::ArrayHandle<TreeIdType>& splitId_Handle,
           vtkm::cont::ArrayHandle<vtkm::Vec<CoordiType, 3>>& qc_Handle,
           vtkm::cont::ArrayHandle<TreeIdType>& nnId_Handle,
           vtkm::cont::ArrayHandle<CoordiType>& nnDis_Handle,
           DeviceAdapter device)
  {
    vtkm::worklet::spatialstructure::KdTree3DNNSearch kdtree3DNNS;
    kdtree3DNNS.Run(
      coordi_Handle, pointId_Handle, splitId_Handle, qc_Handle, nnId_Handle, nnDis_Handle, device);
  }
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Kdtree3D_h
