//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_OrientNormals_h
#define vtkm_m_worklet_OrientNormals_h

#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/worklet/OrientCellNormals.h>
#include <vtkm/worklet/OrientPointAndCellNormals.h>
#include <vtkm/worklet/OrientPointNormals.h>

namespace vtkm
{
namespace worklet
{

///
/// Orients normals to point outside of the dataset. This requires a closed
/// manifold surface or else the behavior is undefined. This requires an
/// unstructured cellset as input.
///
class OrientNormals
{
public:
  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VTKM_CONT static void RunCellNormals(
    const CellSetType& cells,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalCompType, 3>, CellNormalStorageType>& cellNormals)
  {
    OrientCellNormals::Run(cells, coords, cellNormals);
  }

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType>
  VTKM_CONT static void RunPointNormals(
    const CellSetType& cells,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    vtkm::cont::ArrayHandle<vtkm::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals)
  {
    OrientPointNormals::Run(cells, coords, pointNormals);
  }

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VTKM_CONT static void RunPointAndCellNormals(
    const CellSetType& cells,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    vtkm::cont::ArrayHandle<vtkm::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals,
    vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalCompType, 3>, CellNormalStorageType>& cellNormals)
  {
    OrientPointAndCellNormals::Run(cells, coords, pointNormals, cellNormals);
  }

  struct NegateFunctor
  {
    template <typename T>
    VTKM_EXEC_CONT T operator()(const T& val) const
    {
      return -val;
    }
  };

  ///
  /// Reverse the normals to point in the opposite direction.
  ///
  template <typename NormalCompType, typename NormalStorageType>
  VTKM_CONT static void RunFlipNormals(
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalCompType, 3>, NormalStorageType>& normals)
  {
    const auto flippedAlias = vtkm::cont::make_ArrayHandleTransform(normals, NegateFunctor{});
    vtkm::cont::Algorithm::Copy(flippedAlias, normals);
  }
};
}
} // end namespace vtkm::worklet


#endif // vtkm_m_worklet_OrientNormals_h
