//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_ConnectivityTracerBase_h
#define vtk_m_rendering_raytracing_ConnectivityTracerBase_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/raytracing/ConnectivityBase.h>
#include <vtkm/rendering/raytracing/RayOperations.h>

#include <vtkm/Bounds.h>
#include <vtkm/Range.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>

#include <iomanip>
#include <iostream>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT ConnectivityTracerBase : public ConnectivityBase
{
public:
  ConnectivityTracerBase();

  virtual ~ConnectivityTracerBase();

  void Init();

  virtual vtkm::Id GetNumberOfMeshCells() = 0;

  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap) override;

  void SetVolumeData(const vtkm::cont::Field& scalarField,
                     const vtkm::Range& scalarBounds) override;

  void SetEnergyData(const vtkm::cont::Field& absorption,
                     const vtkm::Int32 numBins,
                     const vtkm::cont::Field& emission) override;


  void SetBackgroundColor(const vtkm::Vec<vtkm::Float32, 4>& backgroundColor) override;

  void SetSampleDistance(const vtkm::Float32& distance) override;

protected:
  vtkm::cont::Field ScalarField;
  vtkm::cont::Field EmissionField;
  vtkm::Range ScalarBounds;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorMap;
  vtkm::Float32 BoundingBox[6];
  vtkm::cont::ArrayHandle<vtkm::Id> PreviousCellIds;

  vtkm::Vec<vtkm::Float32, 4> BackgroundColor;
  vtkm::Float32 SampleDistance;
  bool CountRayStatus;
  vtkm::Id RaysLost;
  IntegrationMode Integrator;
  bool DebugFiltersOn;
  bool ReEnterMesh; // Do not try to re-enter the mesh
  bool CreatePartialComposites;
  bool FieldAssocPoints;
  bool HasEmission; // Mode for integrating through energy bins

  // timers
  vtkm::Float64 IntersectTime;
  vtkm::Float64 IntegrateTime;
  vtkm::Float64 SampleTime;
  vtkm::Float64 LostRayTime;
  vtkm::Float64 MeshEntryTime;

  template <typename FloatType, typename Device>
  void PrintRayStatus(Ray<FloatType>& rays, Device)
  {
    vtkm::Id raysExited = RayOperations::GetStatusCount(rays, RAY_EXITED_MESH, Device());
    vtkm::Id raysActive = RayOperations::GetStatusCount(rays, RAY_ACTIVE, Device());
    vtkm::Id raysAbandoned = RayOperations::GetStatusCount(rays, RAY_ABANDONED, Device());
    vtkm::Id raysExitedDom = RayOperations::GetStatusCount(rays, RAY_EXITED_DOMAIN, Device());
    std::cout << "\r Ray Status " << std::setw(10) << std::left << " Lost " << std::setw(10)
              << std::left << RaysLost << std::setw(10) << std::left << " Exited " << std::setw(10)
              << std::left << raysExited << std::setw(10) << std::left << " Active "
              << std::setw(10) << raysActive << std::setw(10) << std::left << " Abandoned "
              << std::setw(10) << raysAbandoned << " Exited Domain " << std::setw(10) << std::left
              << raysExitedDom << "\n";
  }

  void ResetTimers();

  void LogTimers();
};
}
}
}
#endif
