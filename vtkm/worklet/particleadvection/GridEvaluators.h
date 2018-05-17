//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_GridEvaluators_h
#define vtk_m_worklet_particleadvection_GridEvaluators_h

#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

// Constant vector
template <typename FieldType>
class ConstantField
{
public:
  VTKM_CONT
  ConstantField(const vtkm::Bounds& bb, const vtkm::Vec<FieldType, 3>& v)
    : bounds{ bb }
    , vector{ v }
  {
  }

  VTKM_EXEC_CONT
  bool IsWithinSpatialBoundary(const vtkm::Vec<FieldType, 3>& position) const
  {
    if (!bounds.Contains(position))
      return false;
    return true;
  }

  VTKM_EXEC_CONT
  bool IsWithinTemporalBoundary(const FieldType vtkmNotUsed(time)) const { return true; }

  VTKM_EXEC_CONT
  void GetSpatialBoundary(vtkm::Vec<FieldType, 3>& dir, vtkm::Vec<FieldType, 3>& boundary) const
  {
    // Based on the direction of the velocity we need to be able to tell where
    // the particle will exit the domain from to actually push it out of domain.
    boundary[0] = static_cast<FieldType>(dir[0] > 0 ? bounds.X.Max : bounds.X.Min);
    boundary[1] = static_cast<FieldType>(dir[1] > 0 ? bounds.Y.Max : bounds.Y.Min);
    boundary[2] = static_cast<FieldType>(dir[2] > 0 ? bounds.Z.Max : bounds.Z.Min);
  }

  VTKM_EXEC_CONT
  void GetTemporalBoundary(FieldType& boundary) const
  {
    // Return the time of the newest time slice
    boundary = 0;
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                FieldType vtkmNotUsed(time),
                vtkm::Vec<FieldType, 3>& out) const
  {
    return Evaluate(pos, out);
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos, vtkm::Vec<FieldType, 3>& out) const
  {
    if (!bounds.Contains(pos))
      return false;
    out[0] = vector[0];
    out[1] = vector[1];
    out[2] = vector[2];

    return true;
  }

private:
  vtkm::Bounds bounds;
  vtkm::Vec<FieldType, 3> vector;
};

// Circular Orbit
template <typename FieldType>
class AnalyticalOrbitEvaluate
{
public:
  VTKM_CONT
  AnalyticalOrbitEvaluate(const vtkm::Bounds& bb)
    : bounds{ bb }
  {
  }

  VTKM_EXEC_CONT
  bool IsWithinSpatialBoundary(const vtkm::Vec<FieldType, 3>& position) const
  {
    if (!bounds.Contains(position))
      return false;
    return true;
  }

  VTKM_EXEC_CONT
  bool IsWithinTemporalBoundary(const FieldType vtkmNotUsed(time)) const { return true; }

  VTKM_EXEC_CONT
  void GetSpatialBoundary(vtkm::Vec<FieldType, 3>& dir, vtkm::Vec<FieldType, 3>& boundary) const
  {
    // Based on the direction of the velocity we need to be able to tell where
    // the particle will exit the domain from to actually push it out of domain.
    boundary[0] = static_cast<FieldType>(dir[0] > 0 ? bounds.X.Max : bounds.X.Min);
    boundary[1] = static_cast<FieldType>(dir[1] > 0 ? bounds.Y.Max : bounds.Y.Min);
    boundary[2] = static_cast<FieldType>(dir[2] > 0 ? bounds.Z.Max : bounds.Z.Min);
  }

  VTKM_EXEC_CONT
  void GetTemporalBoundary(FieldType& boundary) const
  {
    // Return the time of the newest time slice
    boundary = 0;
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                FieldType vtkmNotUsed(time),
                vtkm::Vec<FieldType, 3>& out) const
  {
    return Evaluate(pos, out);
  }


  VTKM_EXEC bool Evaluate(const vtkm::Vec<FieldType, 3>& pos, vtkm::Vec<FieldType, 3>& out) const
  {
    if (!bounds.Contains(pos))
      return false;

    //statically return a value which is orthogonal to the input pos in the xy plane.
    FieldType oneDivLen = 1.0f / Magnitude(pos);
    out[0] = -1.0f * pos[1] * oneDivLen;
    out[1] = pos[0] * oneDivLen;
    out[2] = pos[2] * oneDivLen;
    return true;
  }

private:
  vtkm::Bounds bounds;
};

//Uniform Grid Evaluator
template <typename PortalType,
          typename FieldType,
          typename DeviceAdapterTag,
          typename StorageTag = VTKM_DEFAULT_STORAGE_TAG>
class UniformGridEvaluate
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, StorageTag>;

public:
  VTKM_CONT
  UniformGridEvaluate() {}

  VTKM_CONT
  UniformGridEvaluate(const vtkm::cont::CoordinateSystem& coords,
                      const vtkm::cont::DynamicCellSet& cellSet,
                      const FieldHandle& vectorField)
  {
    vectors = vectorField.PrepareForInput(DeviceAdapterTag());

    using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
    using StructuredType = vtkm::cont::CellSetStructured<3>;

    if (!coords.GetData().IsType<UniformType>())
      throw vtkm::cont::ErrorInternal("Coordinates are not uniform.");
    if (!cellSet.IsSameType(StructuredType()))
      throw vtkm::cont::ErrorInternal("Cells are not 3D structured.");

    bounds = coords.GetBounds();

    vtkm::cont::CellSetStructured<3> cells;
    cellSet.CopyTo(cells);
    dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());

    // For a Unifrom Grid, the calculation of the Cell for a point is just
    // mapping the point inside the volume into the range 0 to dim - 2.
    // scale is the multiplier for the new point to map into the new range.
    // The mathematics behind this
    //
    // scale = (output_max - output_min) / (input_max - input_min)
    // output = (input - input_min) * scale + output_min
    //
    // In our case output_min is 0
    scale[0] =
      static_cast<FieldType>(dims[0] - 1) / static_cast<FieldType>(bounds.X.Max - bounds.X.Min);
    scale[1] =
      static_cast<FieldType>(dims[1] - 1) / static_cast<FieldType>(bounds.Y.Max - bounds.Y.Min);
    scale[2] =
      static_cast<FieldType>(dims[2] - 1) / static_cast<FieldType>(bounds.Z.Max - bounds.Z.Min);

    planeSize = dims[0] * dims[1];
    rowSize = dims[0];
  }

  VTKM_CONT
  UniformGridEvaluate(const vtkm::cont::DataSet& ds)
  {
    using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;

    auto coordArray = ds.GetCoordinateSystem().GetData();
    if (!coordArray.IsType<UniformType>())
      throw vtkm::cont::ErrorInternal("Given dataset is was not uniform.");

    bounds = ds.GetCoordinateSystem(0).GetBounds();
    vtkm::cont::CellSetStructured<3> cells;
    ds.GetCellSet(0).CopyTo(cells);
    dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());

    // For a Unifrom Grid, the calculation of the Cell for a point is just
    // mapping the point inside the volume into the range 0 to dim - 2.
    // scale is the multiplier for the new point to map into the new range.
    // The mathematics behind this
    //
    // scale = (output_max - output_min) / (input_max - input_min)
    // output = (input - input_min) * scale + output_min
    //
    // In our case output_min is 0
    scale[0] =
      static_cast<FieldType>(dims[0] - 1) / static_cast<FieldType>(bounds.X.Max - bounds.X.Min);
    scale[1] =
      static_cast<FieldType>(dims[1] - 1) / static_cast<FieldType>(bounds.Y.Max - bounds.Y.Min);
    scale[2] =
      static_cast<FieldType>(dims[2] - 1) / static_cast<FieldType>(bounds.Z.Max - bounds.Z.Min);

    planeSize = dims[0] * dims[1];
    rowSize = dims[0];
  }

  VTKM_EXEC_CONT
  bool IsWithinSpatialBoundary(const vtkm::Vec<FieldType, 3>& position) const
  {
    if (!bounds.Contains(position))
      return false;
    return true;
  }

  VTKM_EXEC_CONT
  bool IsWithinTemporalBoundary(const FieldType vtkmNotUsed(time)) const { return true; }

  VTKM_EXEC_CONT
  void GetSpatialBoundary(vtkm::Vec<FieldType, 3>& dir, vtkm::Vec<FieldType, 3>& boundary) const
  {
    // Based on the direction of the velocity we need to be able to tell where
    // the particle will exit the domain from to actually push it out of domain.
    boundary[0] = static_cast<FieldType>(dir[0] > 0 ? bounds.X.Max : bounds.X.Min);
    boundary[1] = static_cast<FieldType>(dir[1] > 0 ? bounds.Y.Max : bounds.Y.Min);
    boundary[2] = static_cast<FieldType>(dir[2] > 0 ? bounds.Z.Max : bounds.Z.Min);
  }

  VTKM_EXEC_CONT
  void GetTemporalBoundary(FieldType& boundary) const
  {
    // Return the time of the newest time slice
    boundary = 0;
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                FieldType vtkmNotUsed(time),
                vtkm::Vec<FieldType, 3>& out) const
  {
    return Evaluate(pos, out);
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos, vtkm::Vec<FieldType, 3>& out) const
  {
    if (!bounds.Contains(pos))
      return false;

    // Set the eight corner indices with no wraparound
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;

    // The normalized point is the result of mapping the input point of the volume
    // to a unit spacing volume with origin as (0,0,0)
    // The method used is described in the constructor.
    vtkm::Vec<FieldType, 3> normalizedPos;
    normalizedPos[0] = static_cast<FieldType>((pos[0] - bounds.X.Min) * scale[0]);
    normalizedPos[1] = static_cast<FieldType>((pos[1] - bounds.Y.Min) * scale[1]);
    normalizedPos[2] = static_cast<FieldType>((pos[2] - bounds.Z.Min) * scale[2]);

    idx000[0] = static_cast<vtkm::IdComponent>(floor(normalizedPos[0]));
    idx000[1] = static_cast<vtkm::IdComponent>(floor(normalizedPos[1]));
    idx000[2] = static_cast<vtkm::IdComponent>(floor(normalizedPos[2]));

    idx001 = idx000;
    idx001[0] = (idx001[0] + 1) <= dims[0] - 1 ? idx001[0] + 1 : dims[0] - 1;
    idx010 = idx000;
    idx010[1] = (idx010[1] + 1) <= dims[1] - 1 ? idx010[1] + 1 : dims[1] - 1;
    idx011 = idx010;
    idx011[0] = (idx011[0] + 1) <= dims[0] - 1 ? idx011[0] + 1 : dims[0] - 1;
    idx100 = idx000;
    idx100[2] = (idx100[2] + 1) <= dims[2] - 1 ? idx100[2] + 1 : dims[2] - 1;
    idx101 = idx100;
    idx101[0] = (idx101[0] + 1) <= dims[0] - 1 ? idx101[0] + 1 : dims[0] - 1;
    idx110 = idx100;
    idx110[1] = (idx110[1] + 1) <= dims[1] - 1 ? idx110[1] + 1 : dims[1] - 1;
    idx111 = idx110;
    idx111[0] = (idx111[0] + 1) <= dims[0] - 1 ? idx111[0] + 1 : dims[0] - 1;

    // Get the vecdata at the eight corners
    vtkm::Vec<FieldType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
    v000 = vectors.Get(idx000[2] * planeSize + idx000[1] * rowSize + idx000[0]);
    v001 = vectors.Get(idx001[2] * planeSize + idx001[1] * rowSize + idx001[0]);
    v010 = vectors.Get(idx010[2] * planeSize + idx010[1] * rowSize + idx010[0]);
    v011 = vectors.Get(idx011[2] * planeSize + idx011[1] * rowSize + idx011[0]);
    v100 = vectors.Get(idx100[2] * planeSize + idx100[1] * rowSize + idx100[0]);
    v101 = vectors.Get(idx101[2] * planeSize + idx101[1] * rowSize + idx101[0]);
    v110 = vectors.Get(idx110[2] * planeSize + idx110[1] * rowSize + idx110[0]);
    v111 = vectors.Get(idx111[2] * planeSize + idx111[1] * rowSize + idx111[0]);

    // Interpolation in X
    vtkm::Vec<FieldType, 3> v00, v01, v10, v11;

    FieldType a = normalizedPos[0] - static_cast<FieldType>(floor(normalizedPos[0]));
    v00[0] = (1.0f - a) * v000[0] + a * v001[0];
    v00[1] = (1.0f - a) * v000[1] + a * v001[1];
    v00[2] = (1.0f - a) * v000[2] + a * v001[2];

    v01[0] = (1.0f - a) * v010[0] + a * v011[0];
    v01[1] = (1.0f - a) * v010[1] + a * v011[1];
    v01[2] = (1.0f - a) * v010[2] + a * v011[2];

    v10[0] = (1.0f - a) * v100[0] + a * v101[0];
    v10[1] = (1.0f - a) * v100[1] + a * v101[1];
    v10[2] = (1.0f - a) * v100[2] + a * v101[2];

    v11[0] = (1.0f - a) * v110[0] + a * v111[0];
    v11[1] = (1.0f - a) * v110[1] + a * v111[1];
    v11[2] = (1.0f - a) * v110[2] + a * v111[2];

    // Interpolation in Y
    vtkm::Vec<FieldType, 3> v0, v1;

    a = normalizedPos[1] - static_cast<FieldType>(floor(normalizedPos[1]));
    v0[0] = (1.0f - a) * v00[0] + a * v01[0];
    v0[1] = (1.0f - a) * v00[1] + a * v01[1];
    v0[2] = (1.0f - a) * v00[2] + a * v01[2];

    v1[0] = (1.0f - a) * v10[0] + a * v11[0];
    v1[1] = (1.0f - a) * v10[1] + a * v11[1];
    v1[2] = (1.0f - a) * v10[2] + a * v11[2];

    a = normalizedPos[2] - static_cast<FieldType>(floor(normalizedPos[2]));
    out[0] = (1.0f - a) * v0[0] + a * v1[0];
    out[1] = (1.0f - a) * v0[1] + a * v1[1];
    out[2] = (1.0f - a) * v0[2] + a * v1[2];
    return true;
  }

private:
  vtkm::Bounds bounds;
  vtkm::Id3 dims;
  PortalType vectors;
  vtkm::Id planeSize;
  vtkm::Id rowSize;
  vtkm::Vec<FieldType, 3> scale;
};

template <typename PortalType,
          typename FieldType,
          typename DeviceAdapterTag,
          typename StorageTag = VTKM_DEFAULT_STORAGE_TAG>
class RectilinearGridEvaluate
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>, StorageTag>;

public:
  VTKM_CONT RectilinearGridEvaluate() = default;

  VTKM_CONT
  RectilinearGridEvaluate(const vtkm::cont::CoordinateSystem& coords,
                          const vtkm::cont::DynamicCellSet& cellSet,
                          const FieldHandle& vectorField)
  {
    using StructuredType = vtkm::cont::CellSetStructured<3>;

    if (!coords.GetData().IsType<RectilinearType>())
      throw vtkm::cont::ErrorInternal("Coordinates are not rectilinear.");
    if (!cellSet.IsSameType(StructuredType()))
      throw vtkm::cont::ErrorInternal("Cells are not 3D structured.");

    vectors = vectorField.PrepareForInput(DeviceAdapterTag());

    bounds = coords.GetBounds();
    vtkm::cont::CellSetStructured<3> cells;
    cellSet.CopyTo(cells);
    dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    planeSize = dims[0] * dims[1];
    rowSize = dims[0];

    RectilinearType gridPoints = coords.GetData().Cast<RectilinearType>();
    RectilinearConstPortal gridPointsPortal = gridPoints.PrepareForInput(DeviceAdapterTag());
    xAxis = gridPointsPortal.GetFirstPortal();
    yAxis = gridPointsPortal.GetSecondPortal();
    zAxis = gridPointsPortal.GetThirdPortal();
  }

  VTKM_CONT
  RectilinearGridEvaluate(const vtkm::cont::DataSet& dataset)
  {
    bounds = dataset.GetCoordinateSystem(0).GetBounds();
    vtkm::cont::CellSetStructured<3> cells;
    dataset.GetCellSet(0).CopyTo(cells);
    dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    planeSize = dims[0] * dims[1];
    rowSize = dims[0];
    auto coordArray = dataset.GetCoordinateSystem().GetData();
    if (coordArray.IsType<RectilinearType>())
    {
      RectilinearType gridPoints = coordArray.Cast<RectilinearType>();
      xAxis = gridPoints.GetPortalConstControl().GetFirstPortal();
      yAxis = gridPoints.GetPortalConstControl().GetSecondPortal();
      zAxis = gridPoints.GetPortalConstControl().GetThirdPortal();
    }
    else
    {
      // As the data is not in the rectilinear format.
      // The code will not be able to continue unless
      // the data is in the required format.
      throw vtkm::cont::ErrorInternal("Given dataset is was not rectilinear.");
    }
  }

  VTKM_EXEC_CONT
  bool IsWithinSpatialBoundary(const vtkm::Vec<FieldType, 3>& position) const
  {
    if (!bounds.Contains(position))
      return false;
    return true;
  }

  VTKM_EXEC_CONT
  bool IsWithinTemporalBoundary(const FieldType vtkmNotUsed(time)) const { return true; }

  VTKM_EXEC_CONT
  void GetSpatialBoundary(vtkm::Vec<FieldType, 3>& dir, vtkm::Vec<FieldType, 3>& boundary) const
  {
    // Based on the direction of the velocity we need to be able to tell where
    // the particle will exit the domain from to actually push it out of domain.
    boundary[0] = static_cast<FieldType>(dir[0] > 0 ? bounds.X.Max : bounds.X.Min);
    boundary[1] = static_cast<FieldType>(dir[1] > 0 ? bounds.Y.Max : bounds.Y.Min);
    boundary[2] = static_cast<FieldType>(dir[2] > 0 ? bounds.Z.Max : bounds.Z.Min);
  }

  VTKM_EXEC_CONT
  void GetTemporalBoundary(FieldType& boundary) const
  {
    // Return the time of the newest time slice
    boundary = 0;
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                FieldType vtkmNotUsed(time),
                vtkm::Vec<FieldType, 3>& out) const
  {
    return Evaluate(pos, out);
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos, vtkm::Vec<FieldType, 3>& out) const
  {
    if (!bounds.Contains(pos))
      return false;
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;

    // Currently the cell search for the Rectilinear Grid is done linearly
    // along all the axes. There needs to be a fast cell lookup method to
    // expedite this.
    vtkm::Vec<vtkm::Id, 3> cellPos(-1, -1, -1);
    vtkm::Id index;
    /*Get floor X location*/
    for (index = 0; index < dims[0] - 1; index++)
      if (xAxis.Get(index) <= pos[0] && pos[0] <= xAxis.Get(index + 1))
      {
        cellPos[0] = index;
        break;
      }
    /*Get floor Y location*/
    for (index = 0; index < dims[1] - 1; index++)
      if (yAxis.Get(index) <= pos[1] && pos[1] <= yAxis.Get(index + 1))
      {
        cellPos[1] = index;
        break;
      }
    /*Get floor Z location*/
    for (index = 0; index < dims[2] - 1; index++)
      if (zAxis.Get(index) <= pos[2] && pos[2] <= zAxis.Get(index + 1))
      {
        cellPos[2] = index;
        break;
      }

    if (cellPos[0] == -1 || cellPos[1] == -1 || cellPos[2] == -1)
      return false;

    idx000[0] = cellPos[0];
    idx000[1] = cellPos[1];
    idx000[2] = cellPos[2];

    idx001 = idx000;
    idx001[0] = (idx001[0] + 1) <= dims[0] - 1 ? idx001[0] + 1 : dims[0] - 1;
    idx010 = idx000;
    idx010[1] = (idx010[1] + 1) <= dims[1] - 1 ? idx010[1] + 1 : dims[1] - 1;
    idx011 = idx010;
    idx011[0] = (idx011[0] + 1) <= dims[0] - 1 ? idx011[0] + 1 : dims[0] - 1;
    idx100 = idx000;
    idx100[2] = (idx100[2] + 1) <= dims[2] - 1 ? idx100[2] + 1 : dims[2] - 1;
    idx101 = idx100;
    idx101[0] = (idx101[0] + 1) <= dims[0] - 1 ? idx101[0] + 1 : dims[0] - 1;
    idx110 = idx100;
    idx110[1] = (idx110[1] + 1) <= dims[1] - 1 ? idx110[1] + 1 : dims[1] - 1;
    idx111 = idx110;
    idx111[0] = (idx111[0] + 1) <= dims[0] - 1 ? idx111[0] + 1 : dims[0] - 1;

    // Get the vecdata at the eight corners
    vtkm::Vec<FieldType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
    v000 = vectors.Get(idx000[2] * planeSize + idx000[1] * rowSize + idx000[0]);
    v001 = vectors.Get(idx001[2] * planeSize + idx001[1] * rowSize + idx001[0]);
    v010 = vectors.Get(idx010[2] * planeSize + idx010[1] * rowSize + idx010[0]);
    v011 = vectors.Get(idx011[2] * planeSize + idx011[1] * rowSize + idx011[0]);
    v100 = vectors.Get(idx100[2] * planeSize + idx100[1] * rowSize + idx100[0]);
    v101 = vectors.Get(idx101[2] * planeSize + idx101[1] * rowSize + idx101[0]);
    v110 = vectors.Get(idx110[2] * planeSize + idx110[1] * rowSize + idx110[0]);
    v111 = vectors.Get(idx111[2] * planeSize + idx111[1] * rowSize + idx111[0]);

    // Interpolation in X
    vtkm::Vec<FieldType, 3> v00, v01, v10, v11;
    FieldType a = pos[0] - static_cast<FieldType>(floor(pos[0]));

    v00[0] = (1.0f - a) * v000[0] + a * v001[0];
    v00[1] = (1.0f - a) * v000[1] + a * v001[1];
    v00[2] = (1.0f - a) * v000[2] + a * v001[2];

    v01[0] = (1.0f - a) * v010[0] + a * v011[0];
    v01[1] = (1.0f - a) * v010[1] + a * v011[1];
    v01[2] = (1.0f - a) * v010[2] + a * v011[2];

    v10[0] = (1.0f - a) * v100[0] + a * v101[0];
    v10[1] = (1.0f - a) * v100[1] + a * v101[1];
    v10[2] = (1.0f - a) * v100[2] + a * v101[2];

    v11[0] = (1.0f - a) * v110[0] + a * v111[0];
    v11[1] = (1.0f - a) * v110[1] + a * v111[1];
    v11[2] = (1.0f - a) * v110[2] + a * v111[2];

    // Interpolation in Y
    vtkm::Vec<FieldType, 3> v0, v1;
    a = pos[1] - static_cast<FieldType>(floor(pos[1]));
    v0[0] = (1.0f - a) * v00[0] + a * v01[0];
    v0[1] = (1.0f - a) * v00[1] + a * v01[1];
    v0[2] = (1.0f - a) * v00[2] + a * v01[2];

    v1[0] = (1.0f - a) * v10[0] + a * v11[0];
    v1[1] = (1.0f - a) * v10[1] + a * v11[1];
    v1[2] = (1.0f - a) * v10[2] + a * v11[2];

    // Interpolation in Z
    a = pos[2] - static_cast<FieldType>(floor(pos[2]));
    out[0] = (1.0f - a) * v0[0] + a * v1[0];
    out[1] = (1.0f - a) * v0[1] + a * v1[1];
    out[2] = (1.0f - a) * v0[2] + a * v1[2];

    return true;
  }

private:
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using RectilinearConstPortal =
    typename RectilinearType::template ExecutionTypes<DeviceAdapterTag>::PortalConst;
  typename AxisHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst xAxis;
  typename AxisHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst yAxis;
  typename AxisHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst zAxis;
  vtkm::Bounds bounds;
  vtkm::Id3 dims;
  PortalType vectors;
  vtkm::Id planeSize;
  vtkm::Id rowSize;

}; //RectilinearGridEvaluate

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_GridEvaluators_h
