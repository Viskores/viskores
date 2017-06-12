//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_SurfaceNormals_h
#define vtk_m_worklet_SurfaceNormals_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/CellTraits.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace worklet
{

class FacetedSurfaceNormals
{
public:
  class Worklet : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInPoint<Vec3> points,
                                  FieldOutCell<Vec3> normals);
    typedef void ExecutionSignature(CellShape, _2, _3);

    using InputDomain = _1;

    template <typename CellShapeTag, typename PointsVecType, typename T>
    VTKM_EXEC void operator()(CellShapeTag,
                              const PointsVecType& points,
                              vtkm::Vec<T, 3>& normal) const
    {
      normal = vtkm::TypeTraits<vtkm::Vec<T, 3>>::ZeroInitialization();
      if (vtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS == 2)
      {
        normal = vtkm::Normal(vtkm::Cross(points[2] - points[1], points[0] - points[1]));
      }
    }

    template <typename PointsVecType, typename T>
    VTKM_EXEC void operator()(vtkm::CellShapeTagGeneric shape,
                              const PointsVecType& points,
                              vtkm::Vec<T, 3>& normal) const
    {
      switch (shape.Id)
      {
        vtkmGenericCellShapeMacro(this->operator()(CellShapeTag(), points, normal));
        default:
          this->RaiseError("unknown cell type");
          break;
      }
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename NormalCompType,
            typename DeviceAdapter>
  void Run(const CellSetType& cellset,
           const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& points,
           vtkm::cont::ArrayHandle<vtkm::Vec<NormalCompType, 3>>& normals,
           DeviceAdapter)
  {
    vtkm::worklet::DispatcherMapTopology<Worklet, DeviceAdapter> dispatcher(Worklet{});
    dispatcher.Invoke(cellset, points, normals);
  }

  template <typename CellSetType,
            typename CoordsStorageList,
            typename NormalCompType,
            typename DeviceAdapter>
  void Run(
    const CellSetType& cellset,
    const vtkm::cont::DynamicArrayHandleBase<vtkm::TypeListTagFieldVec3, CoordsStorageList>& points,
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalCompType, 3>>& normals,
    DeviceAdapter)
  {
    vtkm::worklet::DispatcherMapTopology<Worklet, DeviceAdapter> dispatcher(Worklet{});
    dispatcher.Invoke(cellset, points, normals);
  }
};

class SmoothSurfaceNormals
{
public:
  class Worklet : public vtkm::worklet::WorkletMapCellToPoint
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInCell<Vec3> faceNormals,
                                  FieldOutPoint<Vec3> pointNormals);
    typedef void ExecutionSignature(CellCount, _2, _3);

    using InputDomain = _1;

    template <typename FaceNormalsVecType, typename T>
    VTKM_EXEC void operator()(vtkm::IdComponent numCells,
                              const FaceNormalsVecType& faceNormals,
                              vtkm::Vec<T, 3>& pointNormal) const
    {
      auto result = faceNormals[0];
      for (vtkm::IdComponent i = 1; i < numCells; ++i)
      {
        result += faceNormals[i];
      }
      pointNormal = vtkm::Normal(result / static_cast<T>(numCells));
    }
  };

  template <typename CellSetType,
            typename NormalCompType,
            typename FaceNormalStorageType,
            typename DeviceAdapter>
  void Run(
    const CellSetType& cellset,
    const vtkm::cont::ArrayHandle<vtkm::Vec<NormalCompType, 3>, FaceNormalStorageType>& faceNormals,
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalCompType, 3>>& pointNormals,
    DeviceAdapter)
  {
    vtkm::worklet::DispatcherMapTopology<Worklet, DeviceAdapter> dispatcher(Worklet{});
    dispatcher.Invoke(cellset, faceNormals, pointNormals);
  }

  template <typename CellSetType,
            typename FaceNormalTypeList,
            typename FaceNormalStorageList,
            typename NormalCompType,
            typename DeviceAdapter>
  void Run(const CellSetType& cellset,
           const vtkm::cont::DynamicArrayHandleBase<FaceNormalTypeList, FaceNormalStorageList>&
             faceNormals,
           vtkm::cont::ArrayHandle<vtkm::Vec<NormalCompType, 3>>& pointNormals,
           DeviceAdapter)
  {
    vtkm::worklet::DispatcherMapTopology<Worklet, DeviceAdapter> dispatcher(Worklet{});
    dispatcher.Invoke(cellset, faceNormals, pointNormals);
  }
};
}
} // vtkm::worklet

#endif // vtk_m_worklet_SurfaceNormals_h
