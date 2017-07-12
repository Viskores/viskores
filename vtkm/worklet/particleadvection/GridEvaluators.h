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

//Base class for all grid evaluators

class GridEvaluate
{
public:
  enum Status
  {
    OK = 0,
    OUTSIDE_SPATIAL,
    OUTSIDE_TEMPORAL,
    FAIL
  };
};

// Constant vector
template <typename PortalType, typename FieldType>
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
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                const PortalType& vtkmNotUsed(vecData),
                vtkm::Vec<FieldType, 3>& out) const
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

// Circular Orbit.
template <typename PortalType>
class AnalyticalOrbitEvaluate
{
public:
  VTKM_CONT
  AnalyticalOrbitEvaluate(const vtkm::Bounds& bb)
    : bounds{ bb }
  {
  }

  template <typename FieldType>
  VTKM_EXEC_CONT bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                               const PortalType& vtkmNotUsed(vecData),
                               vtkm::Vec<FieldType, 3>& out) const
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

//Regular Grid Evaluator
template <typename PortalType, typename FieldType>
class RegularGridEvaluate
{
public:
  VTKM_CONT
  RegularGridEvaluate() {}

  VTKM_CONT
  RegularGridEvaluate(const vtkm::cont::DataSet& ds)
  {
    bounds = ds.GetCoordinateSystem(0).GetBounds();
    vtkm::cont::CellSetStructured<3> cells;
    ds.GetCellSet(0).CopyTo(cells);
    dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    vtkm::Vec<FieldType, 3> castdims;
    castdims[0] = static_cast<FieldType>(dims[0]);
    castdims[1] = static_cast<FieldType>(dims[1]);
    castdims[2] = static_cast<FieldType>(dims[2]);
    spacing[0] = static_cast<FieldType>((bounds.X.Max - bounds.X.Min) / (castdims[0] - 1));
    spacing[1] = static_cast<FieldType>((bounds.Y.Max - bounds.Y.Min) / (castdims[1] - 1));
    spacing[2] = static_cast<FieldType>((bounds.Z.Max - bounds.Z.Min) / (castdims[2] - 1));
    oldMin[0] =
      static_cast<FieldType>(bounds.X.Min / ((bounds.X.Max - bounds.X.Min) / castdims[0]));
    oldMin[1] =
      static_cast<FieldType>(bounds.Y.Min / ((bounds.Y.Max - bounds.Y.Min) / castdims[1]));
    oldMin[2] =
      static_cast<FieldType>(bounds.Z.Min / ((bounds.Z.Max - bounds.Z.Min) / castdims[2]));
    planeSize = dims[0] * dims[1];
    rowSize = dims[0];
  }

  VTKM_EXEC_CONT
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                const PortalType& vecData,
                vtkm::Vec<FieldType, 3>& out) const
  {
    if (!bounds.Contains(pos))
      return false;

    // Set the eight corner indices with no wraparound
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;

    vtkm::Vec<FieldType, 3> normalizedPos;
    normalizedPos[0] = pos[0] / spacing[0];
    normalizedPos[1] = pos[1] / spacing[1];
    normalizedPos[2] = pos[2] / spacing[2];

    idx000[0] = static_cast<vtkm::Id>(floor(normalizedPos[0] - oldMin[0]));
    idx000[1] = static_cast<vtkm::Id>(floor(normalizedPos[1] - oldMin[1]));
    idx000[2] = static_cast<vtkm::Id>(floor(normalizedPos[2] - oldMin[2]));

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
    v000 = vecData.Get(idx000[2] * planeSize + idx000[1] * rowSize + idx000[0]);
    v001 = vecData.Get(idx001[2] * planeSize + idx001[1] * rowSize + idx001[0]);
    v010 = vecData.Get(idx010[2] * planeSize + idx010[1] * rowSize + idx010[0]);
    v011 = vecData.Get(idx011[2] * planeSize + idx011[1] * rowSize + idx011[0]);
    v100 = vecData.Get(idx100[2] * planeSize + idx100[1] * rowSize + idx100[0]);
    v101 = vecData.Get(idx101[2] * planeSize + idx101[1] * rowSize + idx101[0]);
    v110 = vecData.Get(idx110[2] * planeSize + idx110[1] * rowSize + idx110[0]);
    v111 = vecData.Get(idx111[2] * planeSize + idx111[1] * rowSize + idx111[0]);

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

    a = pos[2] - static_cast<FieldType>(floor(pos[2]));
    out[0] = (1.0f - a) * v0[0] + v1[0];
    out[1] = (1.0f - a) * v0[1] + v1[1];
    out[2] = (1.0f - a) * v0[2] + v1[2];
    return true;
  }

private:
  vtkm::Bounds bounds;
  vtkm::Id3 dims;
  vtkm::Id planeSize;
  vtkm::Id rowSize;
  vtkm::Vec<FieldType, 3> spacing;
  vtkm::Vec<FieldType, 3> oldMin;
}; //RegularGridEvaluate



template <typename PortalType, typename FieldType>
class RectilinearGridEvaluate
{
public:
  VTKM_CONT
  RectilinearGridEvaluate(const vtkm::cont::DataSet& dataset)
  {
    bounds = dataset.GetCoordinateSystem(0).GetBounds();
    vtkm::cont::CellSetStructured<3> cells;
    dataset.GetCellSet(0).CopyTo(cells);
    dims = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    planeSize = dims[0] * dims[1];
    rowSize = dims[0];
    vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray =
      dataset.GetCoordinateSystem().GetData();
    if (coordArray.IsSameType(RectilinearType()))
    {
      RectilinearType gridPoints = coordArray.Cast<RectilinearType>();
      xAxis = gridPoints.GetPortalConstControl().GetFirstPortal();
      yAxis = gridPoints.GetPortalConstControl().GetSecondPortal();
      zAxis = gridPoints.GetPortalConstControl().GetThirdPortal();
    }
    else
    {
      /*
             * As the data is not in the rectilinear format.
             * The code will not be able to continue unless
             * the data is in the required format.
             */
      throw vtkm::cont::ErrorInternal("Given dataset is was not rectilinear.");
    }
  }

  VTKM_EXEC_CONT
  bool Evaluate(const vtkm::Vec<FieldType, 3>& pos,
                const PortalType& vecData,
                vtkm::Vec<FieldType, 3>& out) const
  {
    if (!bounds.Contains(pos))
      return false;
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;

    vtkm::Vec<vtkm::Id, 3> cellPos;
    vtkm::Id index;
    /*Get floor X location*/
    for (index = 0; index < dims[0] - 1; index++)
    {
      if (xAxis.Get(index) <= pos[0] && pos[0] < xAxis.Get(index + 1))
      {
        cellPos[0] = index;
        break;
      }
    }
    /*Get floor Y location*/
    for (index = 0; index < dims[1] - 1; index++)
    {
      if (yAxis.Get(index) <= pos[1] && pos[1] < yAxis.Get(index + 1))
      {
        cellPos[1] = index;
        break;
      }
    }
    /*Get floor Z location*/
    for (index = 0; index < dims[2] - 1; index++)
    {
      if (zAxis.Get(index) <= pos[2] && pos[2] < zAxis.Get(index + 1))
      {
        cellPos[2] = index;
        break;
      }
    }

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
    v000 = vecData.Get(idx000[2] * planeSize + idx000[1] * rowSize + idx000[0]);
    v001 = vecData.Get(idx001[2] * planeSize + idx001[1] * rowSize + idx001[0]);
    v010 = vecData.Get(idx010[2] * planeSize + idx010[1] * rowSize + idx010[0]);
    v011 = vecData.Get(idx011[2] * planeSize + idx011[1] * rowSize + idx011[0]);
    v100 = vecData.Get(idx100[2] * planeSize + idx100[1] * rowSize + idx100[0]);
    v101 = vecData.Get(idx101[2] * planeSize + idx101[1] * rowSize + idx101[0]);
    v110 = vecData.Get(idx110[2] * planeSize + idx110[1] * rowSize + idx110[0]);
    v111 = vecData.Get(idx111[2] * planeSize + idx111[1] * rowSize + idx111[0]);

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
    out[0] = (1.0f - a) * v0[0] + v1[0];
    out[1] = (1.0f - a) * v0[1] + v1[1];
    out[2] = (1.0f - a) * v0[2] + v1[2];

    return true;
  }

private:
  typedef vtkm::cont::ArrayHandle<FieldType> AxisHandle;
  typedef vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>
    RectilinearType;
  typename AxisHandle::PortalConstControl xAxis;
  typename AxisHandle::PortalConstControl yAxis;
  typename AxisHandle::PortalConstControl zAxis;
  vtkm::Bounds bounds;
  vtkm::Id3 dims;
  vtkm::Id planeSize;
  vtkm::Id rowSize;

}; //RectilinearGridEvaluate

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif // vtk_m_worklet_particleadvection_GridEvaluators_h
