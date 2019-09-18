//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_MeshConnectivityBuilder_h
#define vtk_m_rendering_raytracing_MeshConnectivityBuilder_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/MeshConnectivityContainers.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class MeshConnectivityBuilder
{
public:
  MeshConnectivityBuilder();
  ~MeshConnectivityBuilder();

  VTKM_CONT
  MeshConnContainer* BuildConnectivity(const vtkm::cont::DynamicCellSet& cellset,
                                       const vtkm::cont::CoordinateSystem& coordinates);

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id4> ExternalTrianglesStructured(
    vtkm::cont::CellSetStructured<3>& cellSetStructured);

  vtkm::cont::ArrayHandle<vtkm::Id> GetFaceConnectivity();

  vtkm::cont::ArrayHandle<vtkm::Id> GetFaceOffsets();

  vtkm::cont::ArrayHandle<vtkm::Id4> GetTriangles();

protected:
  VTKM_CONT
  void BuildConnectivity(vtkm::cont::CellSetSingleType<>& cellSetUnstructured,
                         const vtkm::cont::ArrayHandleVirtualCoordinates& coordinates,
                         vtkm::Bounds coordsBounds);

  VTKM_CONT
  void BuildConnectivity(vtkm::cont::CellSetExplicit<>& cellSetUnstructured,
                         const vtkm::cont::ArrayHandleVirtualCoordinates& coordinates,
                         vtkm::Bounds coordsBounds);

  vtkm::cont::ArrayHandle<vtkm::Id> FaceConnectivity;
  vtkm::cont::ArrayHandle<vtkm::Id> FaceOffsets;
  vtkm::cont::ArrayHandle<vtkm::Id4> Triangles;
};
}
}
} //namespace vtkm::rendering::raytracing
#endif
