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

#include <vtkm/worklet/particleadvection/Integrators.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

// Constant vector
class ConstantField : public vtkm::cont::ExecutionObjectBase
{
public:
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

  struct ExecObject
  {
    ExecObject() = default;

    VTKM_CONT
    ExecObject(const vtkm::Bounds& bounds, const vtkm::Vec<ScalarType, 3>& vector)
      : Bounds{ bounds }
      , Vector{ vector }
    {
    }

    VTKM_EXEC_CONT
    bool IsWithinSpatialBoundary(const vtkm::Vec<ScalarType, 3>& position) const
    {
      if (!this->Bounds.Contains(position))
        return false;
      return true;
    }

    VTKM_EXEC_CONT
    bool IsWithinTemporalBoundary(const ScalarType vtkmNotUsed(time)) const { return true; }

    VTKM_EXEC_CONT
    void GetSpatialBoundary(vtkm::Vec<ScalarType, 3>& dir, vtkm::Vec<ScalarType, 3>& boundary) const
    {
      // Based on the direction of the velocity we need to be able to tell where
      // the particle will exit the domain from to actually push it out of domain.
      boundary[0] = static_cast<ScalarType>(dir[0] > 0 ? this->Bounds.X.Max : this->Bounds.X.Min);
      boundary[1] = static_cast<ScalarType>(dir[1] > 0 ? this->Bounds.Y.Max : this->Bounds.Y.Min);
      boundary[2] = static_cast<ScalarType>(dir[2] > 0 ? this->Bounds.Z.Max : this->Bounds.Z.Min);
    }

    VTKM_EXEC_CONT
    void GetTemporalBoundary(ScalarType& boundary) const
    {
      // Return the time of the newest time slice
      boundary = 0;
    }

    VTKM_EXEC
    bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos,
                  ScalarType vtkmNotUsed(time),
                  vtkm::Vec<ScalarType, 3>& out) const
    {
      return this->Evaluate(pos, out);
    }

    VTKM_EXEC
    bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos, vtkm::Vec<ScalarType, 3>& out) const
    {
      if (!this->Bounds.Contains(pos))
        return false;
      out[0] = this->Vector[0];
      out[1] = this->Vector[1];
      out[2] = this->Vector[2];

      return true;
    }

    vtkm::Bounds Bounds;
    vtkm::Vec<ScalarType, 3> Vector;
  };

  ConstantField() = default;

  VTKM_CONT
  ConstantField(const vtkm::Bounds& bounds, const vtkm::Vec<ScalarType, 3>& vector)
    : Bounds{ bounds }
    , Vector{ vector }
  {
  }

  template <typename Device>
  VTKM_CONT ExecObject PrepareForExecution(Device) const
  {
    return ExecObject(this->Bounds, this->Vector);
  }

private:
  vtkm::Bounds Bounds;
  vtkm::Vec<ScalarType, 3> Vector;
};

// Circular Orbit
class AnalyticalOrbitEvaluate
{
public:
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

  struct ExecObject
  {
    VTKM_CONT
    ExecObject(const vtkm::Bounds& bounds)
      : Bounds{ bounds }
    {
    }

    VTKM_EXEC_CONT
    bool IsWithinSpatialBoundary(const vtkm::Vec<ScalarType, 3>& position) const
    {
      if (!this->Bounds.Contains(position))
        return false;
      return true;
    }

    VTKM_EXEC_CONT
    bool IsWithinTemporalBoundary(const ScalarType vtkmNotUsed(time)) const { return true; }

    VTKM_EXEC_CONT
    void GetSpatialBoundary(vtkm::Vec<ScalarType, 3>& dir, vtkm::Vec<ScalarType, 3>& boundary) const
    {
      // Based on the direction of the velocity we need to be able to tell where
      // the particle will exit the domain from to actually push it out of domain.
      boundary[0] = static_cast<ScalarType>(dir[0] > 0 ? this->Bounds.X.Max : this->Bounds.X.Min);
      boundary[1] = static_cast<ScalarType>(dir[1] > 0 ? this->Bounds.Y.Max : this->Bounds.Y.Min);
      boundary[2] = static_cast<ScalarType>(dir[2] > 0 ? this->Bounds.Z.Max : this->Bounds.Z.Min);
    }

    VTKM_EXEC_CONT
    void GetTemporalBoundary(ScalarType& boundary) const
    {
      // Return the time of the newest time slice
      boundary = 0;
    }

    VTKM_EXEC
    bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos,
                  ScalarType vtkmNotUsed(time),
                  vtkm::Vec<ScalarType, 3>& out) const
    {
      return this->Evaluate(pos, out);
    }


    VTKM_EXEC bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos,
                            vtkm::Vec<ScalarType, 3>& out) const
    {
      if (!this->Bounds.Contains(pos))
        return false;

      //statically return a value which is orthogonal to the input pos in the xy plane.
      ScalarType oneDivLen = 1.0f / vtkm::Magnitude(pos);
      out[0] = -1.0f * pos[1] * oneDivLen;
      out[1] = pos[0] * oneDivLen;
      out[2] = pos[2] * oneDivLen;
      return true;
    }

    vtkm::Bounds Bounds;
  };

  VTKM_CONT
  AnalyticalOrbitEvaluate(const vtkm::Bounds& bounds)
    : Bounds{ bounds }
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT ExecObject PrepareForExecution(DeviceAdapter)
  {
    return ExecObject(this->Bounds);
  }

private:
  vtkm::Bounds Bounds;
};

//Uniform Grid Evaluator
template <typename FieldArrayType>
class UniformGridEvaluate : public vtkm::cont::ExecutionObjectBase
{
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

public:
  UniformGridEvaluate() = default;

  VTKM_CONT
  UniformGridEvaluate(const vtkm::cont::CoordinateSystem& coords,
                      const vtkm::cont::DynamicCellSet& cellSet,
                      const FieldArrayType& vectorField)
    : Bounds(coords.GetBounds())
    , Vectors(vectorField)
  {
    using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
    using StructuredType = vtkm::cont::CellSetStructured<3>;

    if (!coords.GetData().IsType<UniformType>())
      throw vtkm::cont::ErrorInternal("Coordinates are not uniform.");
    if (!cellSet.IsSameType(StructuredType()))
      throw vtkm::cont::ErrorInternal("Cells are not 3D structured.");

    vtkm::cont::CellSetStructured<3> cells;
    cellSet.CopyTo(cells);
    this->Dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());

    // For a Unifrom Grid, the calculation of the Cell for a point is just
    // mapping the point inside the volume into the range 0 to dim - 2.
    // scale is the multiplier for the new point to map into the new range.
    // The mathematics behind this
    //
    // scale = (output_max - output_min) / (input_max - input_min)
    // output = (input - input_min) * scale + output_min
    //
    // In our case output_min is 0
    this->Scale[0] = static_cast<ScalarType>(this->Dims[0] - 1) /
      static_cast<ScalarType>(this->Bounds.X.Max - this->Bounds.X.Min);
    this->Scale[1] = static_cast<ScalarType>(this->Dims[1] - 1) /
      static_cast<ScalarType>(this->Bounds.Y.Max - this->Bounds.Y.Min);
    this->Scale[2] = static_cast<ScalarType>(this->Dims[2] - 1) /
      static_cast<ScalarType>(this->Bounds.Z.Max - this->Bounds.Z.Min);

    this->PlaneSize = this->Dims[0] * this->Dims[1];
    this->RowSize = this->Dims[0];
  }

  template <typename DeviceAdapter>
  struct ExecObject
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);

    using FieldPortalType =
      typename FieldArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

    ExecObject() = default;

    ExecObject(const vtkm::Bounds& bounds,
               const vtkm::Id3& dims,
               const FieldArrayType& vectors,
               vtkm::Id planeSize,
               vtkm::Id rowSize,
               const vtkm::Vec<ScalarType, 3>& scale)
      : Bounds(bounds)
      , Dims(dims)
      , Vectors(vectors.PrepareForInput(DeviceAdapter()))
      , PlaneSize(planeSize)
      , RowSize(rowSize)
      , Scale(scale)
    {
    }

    VTKM_EXEC_CONT
    bool IsWithinSpatialBoundary(const vtkm::Vec<ScalarType, 3>& position) const
    {
      if (!this->Bounds.Contains(position))
        return false;
      return true;
    }

    VTKM_EXEC_CONT
    bool IsWithinTemporalBoundary(const ScalarType vtkmNotUsed(time)) const { return true; }

    VTKM_EXEC_CONT
    void GetSpatialBoundary(vtkm::Vec<ScalarType, 3>& dir, vtkm::Vec<ScalarType, 3>& boundary) const
    {
      // Based on the direction of the velocity we need to be able to tell where
      // the particle will exit the domain from to actually push it out of domain.
      boundary[0] = static_cast<ScalarType>(dir[0] > 0 ? this->Bounds.X.Max : this->Bounds.X.Min);
      boundary[1] = static_cast<ScalarType>(dir[1] > 0 ? this->Bounds.Y.Max : this->Bounds.Y.Min);
      boundary[2] = static_cast<ScalarType>(dir[2] > 0 ? this->Bounds.Z.Max : this->Bounds.Z.Min);
    }

    VTKM_EXEC_CONT
    void GetTemporalBoundary(ScalarType& boundary) const
    {
      // Return the time of the newest time slice
      boundary = 0;
    }

    VTKM_EXEC
    bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos,
                  ScalarType vtkmNotUsed(time),
                  vtkm::Vec<ScalarType, 3>& out) const
    {
      return Evaluate(pos, out);
    }

    VTKM_EXEC
    bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos, vtkm::Vec<ScalarType, 3>& out) const
    {
      if (!this->Bounds.Contains(pos))
        return false;

      // Set the eight corner indices with no wraparound
      vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;

      // The normalized point is the result of mapping the input point of the volume
      // to a unit spacing volume with origin as (0,0,0)
      // The method used is described in the constructor.
      vtkm::Vec<ScalarType, 3> normalizedPos;
      normalizedPos[0] = static_cast<ScalarType>((pos[0] - this->Bounds.X.Min) * this->Scale[0]);
      normalizedPos[1] = static_cast<ScalarType>((pos[1] - this->Bounds.Y.Min) * this->Scale[1]);
      normalizedPos[2] = static_cast<ScalarType>((pos[2] - this->Bounds.Z.Min) * this->Scale[2]);

      idx000[0] = static_cast<vtkm::IdComponent>(floor(normalizedPos[0]));
      idx000[1] = static_cast<vtkm::IdComponent>(floor(normalizedPos[1]));
      idx000[2] = static_cast<vtkm::IdComponent>(floor(normalizedPos[2]));

      idx001 = idx000;
      idx001[0] = (idx001[0] + 1) <= this->Dims[0] - 1 ? idx001[0] + 1 : this->Dims[0] - 1;
      idx010 = idx000;
      idx010[1] = (idx010[1] + 1) <= this->Dims[1] - 1 ? idx010[1] + 1 : this->Dims[1] - 1;
      idx011 = idx010;
      idx011[0] = (idx011[0] + 1) <= this->Dims[0] - 1 ? idx011[0] + 1 : this->Dims[0] - 1;
      idx100 = idx000;
      idx100[2] = (idx100[2] + 1) <= this->Dims[2] - 1 ? idx100[2] + 1 : this->Dims[2] - 1;
      idx101 = idx100;
      idx101[0] = (idx101[0] + 1) <= this->Dims[0] - 1 ? idx101[0] + 1 : this->Dims[0] - 1;
      idx110 = idx100;
      idx110[1] = (idx110[1] + 1) <= this->Dims[1] - 1 ? idx110[1] + 1 : this->Dims[1] - 1;
      idx111 = idx110;
      idx111[0] = (idx111[0] + 1) <= this->Dims[0] - 1 ? idx111[0] + 1 : this->Dims[0] - 1;

      // Get the vecdata at the eight corners
      vtkm::Vec<ScalarType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
      v000 = this->Vectors.Get(idx000[2] * this->PlaneSize + idx000[1] * this->RowSize + idx000[0]);
      v001 = this->Vectors.Get(idx001[2] * this->PlaneSize + idx001[1] * this->RowSize + idx001[0]);
      v010 = this->Vectors.Get(idx010[2] * this->PlaneSize + idx010[1] * this->RowSize + idx010[0]);
      v011 = this->Vectors.Get(idx011[2] * this->PlaneSize + idx011[1] * this->RowSize + idx011[0]);
      v100 = this->Vectors.Get(idx100[2] * this->PlaneSize + idx100[1] * this->RowSize + idx100[0]);
      v101 = this->Vectors.Get(idx101[2] * this->PlaneSize + idx101[1] * this->RowSize + idx101[0]);
      v110 = this->Vectors.Get(idx110[2] * this->PlaneSize + idx110[1] * this->RowSize + idx110[0]);
      v111 = this->Vectors.Get(idx111[2] * this->PlaneSize + idx111[1] * this->RowSize + idx111[0]);

      // Interpolation in X
      vtkm::Vec<ScalarType, 3> v00, v01, v10, v11;

      ScalarType a = normalizedPos[0] - static_cast<ScalarType>(floor(normalizedPos[0]));
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
      vtkm::Vec<ScalarType, 3> v0, v1;

      a = normalizedPos[1] - static_cast<ScalarType>(floor(normalizedPos[1]));
      v0[0] = (1.0f - a) * v00[0] + a * v01[0];
      v0[1] = (1.0f - a) * v00[1] + a * v01[1];
      v0[2] = (1.0f - a) * v00[2] + a * v01[2];

      v1[0] = (1.0f - a) * v10[0] + a * v11[0];
      v1[1] = (1.0f - a) * v10[1] + a * v11[1];
      v1[2] = (1.0f - a) * v10[2] + a * v11[2];

      a = normalizedPos[2] - static_cast<ScalarType>(floor(normalizedPos[2]));
      out[0] = (1.0f - a) * v0[0] + a * v1[0];
      out[1] = (1.0f - a) * v0[1] + a * v1[1];
      out[2] = (1.0f - a) * v0[2] + a * v1[2];
      return true;
    }

    vtkm::Bounds Bounds;
    vtkm::Id3 Dims;
    FieldPortalType Vectors;
    vtkm::Id PlaneSize;
    vtkm::Id RowSize;
    vtkm::Vec<ScalarType, 3> Scale;
  };

  template <typename DeviceAdapter>
  VTKM_CONT ExecObject<DeviceAdapter> PrepareForExecution(DeviceAdapter) const
  {
    return ExecObject<DeviceAdapter>(
      this->Bounds, this->Dims, this->Vectors, this->PlaneSize, this->RowSize, this->Scale);
  }

private:
  vtkm::Bounds Bounds;
  vtkm::Id3 Dims;
  FieldArrayType Vectors;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  vtkm::Vec<ScalarType, 3> Scale;
};

template <typename FieldArrayType>
class RectilinearGridEvaluate : public vtkm::cont::ExecutionObjectBase
{
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;

  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

public:
  RectilinearGridEvaluate() = default;

  VTKM_CONT
  RectilinearGridEvaluate(const vtkm::cont::CoordinateSystem& coords,
                          const vtkm::cont::DynamicCellSet& cellSet,
                          const FieldArrayType& vectorField)
    : Bounds(coords.GetBounds())
    , Vectors(vectorField)
  {
    using StructuredType = vtkm::cont::CellSetStructured<3>;

    if (!coords.GetData().IsType<RectilinearType>())
      throw vtkm::cont::ErrorInternal("Coordinates are not rectilinear.");
    if (!cellSet.IsSameType(StructuredType()))
      throw vtkm::cont::ErrorInternal("Cells are not 3D structured.");

    vtkm::cont::CellSetStructured<3> cells;
    cellSet.CopyTo(cells);
    this->Dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    this->PlaneSize = this->Dims[0] * this->Dims[1];
    this->RowSize = this->Dims[0];

    this->CoordinatesArray = coords.GetData().Cast<RectilinearType>();
  }

  template <typename DeviceAdapter>
  struct ExecObject
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);

    using FieldPortalType =
      typename FieldArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using RectilinearPortalType =
      typename RectilinearType::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using AxisPortalType = typename AxisHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;

    ExecObject() = default;

    VTKM_CONT
    ExecObject(const vtkm::Bounds& bounds,
               const vtkm::Id3& dims,
               const FieldArrayType& vectors,
               vtkm::Id planeSize,
               vtkm::Id rowSize,
               const RectilinearType& coordinatesArray)
      : Bounds(bounds)
      , Dims(dims)
      , Vectors(vectors.PrepareForInput(DeviceAdapter()))
      , PlaneSize(planeSize)
      , RowSize(rowSize)
    {
      RectilinearPortalType coordinatesPortal = coordinatesArray.PrepareForInput(DeviceAdapter());
      xAxis = coordinatesPortal.GetFirstPortal();
      yAxis = coordinatesPortal.GetSecondPortal();
      zAxis = coordinatesPortal.GetThirdPortal();
    }

    VTKM_EXEC_CONT
    bool IsWithinSpatialBoundary(const vtkm::Vec<ScalarType, 3>& position) const
    {
      if (!this->Bounds.Contains(position))
        return false;
      return true;
    }

    VTKM_EXEC_CONT
    bool IsWithinTemporalBoundary(const ScalarType vtkmNotUsed(time)) const { return true; }

    VTKM_EXEC_CONT
    void GetSpatialBoundary(vtkm::Vec<ScalarType, 3>& dir, vtkm::Vec<ScalarType, 3>& boundary) const
    {
      // Based on the direction of the velocity we need to be able to tell where
      // the particle will exit the domain from to actually push it out of domain.
      boundary[0] = static_cast<ScalarType>(dir[0] > 0 ? this->Bounds.X.Max : this->Bounds.X.Min);
      boundary[1] = static_cast<ScalarType>(dir[1] > 0 ? this->Bounds.Y.Max : this->Bounds.Y.Min);
      boundary[2] = static_cast<ScalarType>(dir[2] > 0 ? this->Bounds.Z.Max : this->Bounds.Z.Min);
    }

    VTKM_EXEC_CONT
    void GetTemporalBoundary(ScalarType& boundary) const
    {
      // Return the time of the newest time slice
      boundary = 0;
    }

    VTKM_EXEC
    bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos,
                  ScalarType vtkmNotUsed(time),
                  vtkm::Vec<ScalarType, 3>& out) const
    {
      return Evaluate(pos, out);
    }

    VTKM_EXEC
    bool Evaluate(const vtkm::Vec<ScalarType, 3>& pos, vtkm::Vec<ScalarType, 3>& out) const
    {
      if (!this->Bounds.Contains(pos))
        return false;
      vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;

      // Currently the cell search for the Rectilinear Grid is done linearly
      // along all the axes. There needs to be a fast cell lookup method to
      // expedite this.
      vtkm::Vec<vtkm::Id, 3> cellPos(-1, -1, -1);
      vtkm::Id index;
      /*Get floor X location*/
      for (index = 0; index < this->Dims[0] - 1; index++)
        if (xAxis.Get(index) <= pos[0] && pos[0] <= xAxis.Get(index + 1))
        {
          cellPos[0] = index;
          break;
        }
      /*Get floor Y location*/
      for (index = 0; index < this->Dims[1] - 1; index++)
        if (yAxis.Get(index) <= pos[1] && pos[1] <= yAxis.Get(index + 1))
        {
          cellPos[1] = index;
          break;
        }
      /*Get floor Z location*/
      for (index = 0; index < this->Dims[2] - 1; index++)
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
      idx001[0] = (idx001[0] + 1) <= this->Dims[0] - 1 ? idx001[0] + 1 : this->Dims[0] - 1;
      idx010 = idx000;
      idx010[1] = (idx010[1] + 1) <= this->Dims[1] - 1 ? idx010[1] + 1 : this->Dims[1] - 1;
      idx011 = idx010;
      idx011[0] = (idx011[0] + 1) <= this->Dims[0] - 1 ? idx011[0] + 1 : this->Dims[0] - 1;
      idx100 = idx000;
      idx100[2] = (idx100[2] + 1) <= this->Dims[2] - 1 ? idx100[2] + 1 : this->Dims[2] - 1;
      idx101 = idx100;
      idx101[0] = (idx101[0] + 1) <= this->Dims[0] - 1 ? idx101[0] + 1 : this->Dims[0] - 1;
      idx110 = idx100;
      idx110[1] = (idx110[1] + 1) <= this->Dims[1] - 1 ? idx110[1] + 1 : this->Dims[1] - 1;
      idx111 = idx110;
      idx111[0] = (idx111[0] + 1) <= this->Dims[0] - 1 ? idx111[0] + 1 : this->Dims[0] - 1;

      // Get the vecdata at the eight corners
      vtkm::Vec<ScalarType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
      v000 = this->Vectors.Get(idx000[2] * this->PlaneSize + idx000[1] * this->RowSize + idx000[0]);
      v001 = this->Vectors.Get(idx001[2] * this->PlaneSize + idx001[1] * this->RowSize + idx001[0]);
      v010 = this->Vectors.Get(idx010[2] * this->PlaneSize + idx010[1] * this->RowSize + idx010[0]);
      v011 = this->Vectors.Get(idx011[2] * this->PlaneSize + idx011[1] * this->RowSize + idx011[0]);
      v100 = this->Vectors.Get(idx100[2] * this->PlaneSize + idx100[1] * this->RowSize + idx100[0]);
      v101 = this->Vectors.Get(idx101[2] * this->PlaneSize + idx101[1] * this->RowSize + idx101[0]);
      v110 = this->Vectors.Get(idx110[2] * this->PlaneSize + idx110[1] * this->RowSize + idx110[0]);
      v111 = this->Vectors.Get(idx111[2] * this->PlaneSize + idx111[1] * this->RowSize + idx111[0]);

      // Interpolation in X
      vtkm::Vec<ScalarType, 3> v00, v01, v10, v11;
      ScalarType a = pos[0] - static_cast<ScalarType>(floor(pos[0]));

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
      vtkm::Vec<ScalarType, 3> v0, v1;
      a = pos[1] - static_cast<ScalarType>(floor(pos[1]));
      v0[0] = (1.0f - a) * v00[0] + a * v01[0];
      v0[1] = (1.0f - a) * v00[1] + a * v01[1];
      v0[2] = (1.0f - a) * v00[2] + a * v01[2];

      v1[0] = (1.0f - a) * v10[0] + a * v11[0];
      v1[1] = (1.0f - a) * v10[1] + a * v11[1];
      v1[2] = (1.0f - a) * v10[2] + a * v11[2];

      // Interpolation in Z
      a = pos[2] - static_cast<ScalarType>(floor(pos[2]));
      out[0] = (1.0f - a) * v0[0] + a * v1[0];
      out[1] = (1.0f - a) * v0[1] + a * v1[1];
      out[2] = (1.0f - a) * v0[2] + a * v1[2];

      return true;
    }

    vtkm::Bounds Bounds;
    vtkm::Id3 Dims;
    FieldPortalType Vectors;
    vtkm::Id PlaneSize;
    vtkm::Id RowSize;
    AxisPortalType xAxis;
    AxisPortalType yAxis;
    AxisPortalType zAxis;
  };

  template <typename DeviceAdapter>
  VTKM_CONT ExecObject<DeviceAdapter> PrepareForExecution(DeviceAdapter) const
  {
    return ExecObject<DeviceAdapter>(this->Bounds,
                                     this->Dims,
                                     this->Vectors,
                                     this->PlaneSize,
                                     this->RowSize,
                                     this->CoordinatesArray);
  }

private:
  vtkm::Bounds Bounds;
  vtkm::Id3 Dims;
  FieldArrayType Vectors;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  RectilinearType CoordinatesArray;
}; //RectilinearGridEvaluate

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_GridEvaluators_h
