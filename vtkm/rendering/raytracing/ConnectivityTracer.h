//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_ConnectivityTracer_h
#define vtk_m_rendering_raytracing_ConnectivityTracer_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/rendering/raytracing/MeshConnectivityContainers.h>
#include <vtkm/rendering/raytracing/PartialComposite.h>


namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

//forward declare so we can be friends
struct RenderFunctor;

//
//  Ray tracker manages memory and pointer
//  swapping for current cell intersection data
//
template <typename FloatType>
class RayTracking
{
public:
  vtkm::cont::ArrayHandle<vtkm::Int32> ExitFace;
  vtkm::cont::ArrayHandle<FloatType> CurrentDistance;
  vtkm::cont::ArrayHandle<FloatType> Distance1;
  vtkm::cont::ArrayHandle<FloatType> Distance2;
  vtkm::cont::ArrayHandle<FloatType>* EnterDist;
  vtkm::cont::ArrayHandle<FloatType>* ExitDist;

  RayTracking()
  {
    EnterDist = &Distance1;
    ExitDist = &Distance2;
  }

  void Compact(vtkm::cont::ArrayHandle<FloatType>& compactedDistances,
               vtkm::cont::ArrayHandle<UInt8>& masks);

  void Init(const vtkm::Id size, vtkm::cont::ArrayHandle<FloatType>& distances);

  void Swap();
};

} //namespace detail

/**
 * \brief ConnectivityTracer is volumetric ray tracer for unstructured
 *        grids. Capabilities include volume rendering and integrating
 *        absorption and emission of N energy groups for simulated
 *        radiograhy.
 */
class VTKM_RENDERING_EXPORT ConnectivityTracer
{
public:
  ConnectivityTracer()
    : MeshContainer(nullptr)
    , CountRayStatus(false)
    , UnitScalar(1.f)
  {
  }

  ~ConnectivityTracer()
  {
    if (MeshContainer != nullptr)
    {
      delete MeshContainer;
    }
  }

  enum IntegrationMode
  {
    Volume,
    Energy
  };

  void SetVolumeData(const vtkm::cont::Field& scalarField,
                     const vtkm::Range& scalarBounds,
                     const vtkm::cont::DynamicCellSet& cellSet,
                     const vtkm::cont::CoordinateSystem& coords);

  void SetEnergyData(const vtkm::cont::Field& absorption,
                     const vtkm::Int32 numBins,
                     const vtkm::cont::DynamicCellSet& cellSet,
                     const vtkm::cont::CoordinateSystem& coords,
                     const vtkm::cont::Field& emission);

  void SetBackgroundColor(const vtkm::Vec4f_32& backgroundColor);
  void SetSampleDistance(const vtkm::Float32& distance);
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap);

  MeshConnContainer* GetMeshContainer() { return MeshContainer; }

  void Init();

  void SetDebugOn(bool on) { CountRayStatus = on; }

  void SetUnitScalar(const vtkm::Float32 unitScalar) { UnitScalar = unitScalar; }


  vtkm::Id GetNumberOfMeshCells() const;

  void ResetTimers();
  void LogTimers();

  ///
  /// Traces rays fully through the mesh. Rays can exit and re-enter
  /// multiple times before leaving the domain. This is fast path for
  /// structured meshs or meshes that are not interlocking.
  /// Note: rays will be compacted
  ///
  template <typename FloatType>
  void FullTrace(Ray<FloatType>& rays);

  ///
  /// Integrates rays through the mesh. If rays leave the mesh and
  /// re-enter, then those become two separate partial composites.
  /// This is need to support domain decompositions that are like
  /// puzzle pieces. Note: rays will be compacted
  ///
  template <typename FloatType>
  std::vector<PartialComposite<FloatType>> PartialTrace(Ray<FloatType>& rays);

  ///
  /// Integrates the active rays though the mesh until all rays
  /// have exited.
  ///  Precondition: rays.HitIdx is set to a valid mesh cell
  ///
  template <typename FloatType>
  void IntegrateMeshSegment(Ray<FloatType>& rays);

  ///
  /// Find the entry point in the mesh
  ///
  template <typename FloatType>
  void FindMeshEntry(Ray<FloatType>& rays);

private:
  template <typename FloatType>
  void IntersectCell(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void AccumulatePathLengths(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void FindLostRays(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void SampleCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void IntegrateCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void OffsetMinDistances(Ray<FloatType>& rays);

  template <typename FloatType>
  void PrintRayStatus(Ray<FloatType>& rays);

protected:
  // Data set info
  vtkm::cont::Field ScalarField;
  vtkm::cont::Field EmissionField;
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::Range ScalarBounds;
  vtkm::Float32 BoundingBox[6];

  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> ColorMap;

  vtkm::Vec4f_32 BackgroundColor;
  vtkm::Float32 SampleDistance;
  vtkm::Id RaysLost;
  IntegrationMode Integrator;

  MeshConnContainer* MeshContainer;
  //
  // flags
  bool CountRayStatus;
  bool MeshConnIsConstructed;
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
  vtkm::Float32 UnitScalar;

}; // class ConnectivityTracer<CellType,ConnectivityType>
}
}
} // namespace vtkm::rendering::raytracing
#endif
