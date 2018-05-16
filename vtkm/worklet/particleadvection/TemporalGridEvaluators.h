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
#ifndef vtk_m_worklet_particleadvection_TemporalGridEvaluators_h
#define vtk_m_worklet_particleadvection_TemporalGridEvaluators_h

#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename PortalType,
          typename FieldType,
          typename DeviceAdapterTag,
          typename StorageTag = VTKM_DEFAULT_STORAGE_TAG>
class TemporalGridEvaluator
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>;

public:
  VTKM_CONT
  TemporalGridEvaluator(){};

  /*
   * This method is intended to update the initial datasets we are going to
   * use to advect the particels.
   * The components are similar to the normal datasets except now we add times
   * for which the datasets were active to perform temporal interpolation.
   */
  VTKM_CONT
  TemporalGridEvaluator(const vtkm::cont::CoordinateSystem& coords1,
                        const vtkm::cont::DynamicCellSet& cellSet1,
                        const FieldHandle& vectorField1,
                        const FieldType datasettime1,
                        const vtkm::cont::CoordinateSystem& coords2,
                        const vtkm::cont::DynamicCellSet& cellSet2,
                        const FieldHandle& vectorField2,
                        const FieldType datasettime2)

  {
    using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
    using StructuredType = vtkm::cont::CellSetStructured<3>;

    if (!coords1.GetData().IsType<UniformType>() || !coords2.GetData().IsType<UniformType>())
      throw vtkm::cont::ErrorInternal("Coordinates are not uniform.");
    if (!cellSet1.IsSameType(StructuredType()) || !cellSet2.IsSameType(StructuredType()))
      throw vtkm::cont::ErrorInternal("Cells are not 3D structured.");

    vectors1 = vectorField1.PrepareForInput(DeviceAdapterTag());
    vectors2 = vectorField2.PrepareForInput(DeviceAdapterTag());

    bounds1 = coords1.GetBounds();
    bounds2 = coords2.GetBounds();

    vtkm::cont::CellSetStructured<3> cells1;
    vtkm::cont::CellSetStructured<3> cells2;

    cellSet1.CopyTo(cells1);
    dims1 = cells1.GetSchedulingRange(vtkm::TopologyElementTagPoint());

    cellSet2.CopyTo(cells2);
    dims2 = cells2.GetSchedulingRange(vtkm::TopologyElementTagPoint());

    time1 = datasettime1;
    time2 = datasettime2;

    planeSize1 = dims1[1] * dims1[1];
    rowSize1 = dims1[0];

    planeSize2 = dims2[1] * dims2[1];
    rowSize2 = dims2[0];

    scale1[0] =
      static_cast<FieldType>(dims1[0] - 1) / static_cast<FieldType>(bounds1.X.Max - bounds1.X.Min);
    scale1[1] =
      static_cast<FieldType>(dims1[1] - 1) / static_cast<FieldType>(bounds1.Y.Max - bounds1.Y.Min);
    scale1[2] =
      static_cast<FieldType>(dims1[2] - 1) / static_cast<FieldType>(bounds1.Z.Max - bounds1.Z.Min);

    scale2[0] =
      static_cast<FieldType>(dims2[0] - 1) / static_cast<FieldType>(bounds2.X.Max - bounds2.X.Min);
    scale2[1] =
      static_cast<FieldType>(dims2[1] - 1) / static_cast<FieldType>(bounds2.Y.Max - bounds2.Y.Min);
    scale2[2] =
      static_cast<FieldType>(dims2[2] - 1) / static_cast<FieldType>(bounds2.Z.Max - bounds2.Z.Min);
  };


  VTKM_EXEC_CONT
  bool IsWithinSpatialBoundary(const vtkm::Vec<FieldType, 3>& position) const
  {
    if (!bounds1.Contains(position) || !bounds2.Contains(position))
      return false;
    return true;
  }

  VTKM_EXEC_CONT
  bool IsWithinTemporalBoundary(const FieldType time) const
  {
    if (time < time1 || time >= time2)
      return false;
    return true;
  }

  VTKM_EXEC_CONT
  void GetSpatialBoundary(vtkm::Vec<FieldType, 3>& dir, vtkm::Vec<FieldType, 3>& boundary) const
  {
    // Based on the direction of the velocity we need to be able to tell where
    // the particle will exit the domain from to actually push it out of domain.
    boundary[0] = static_cast<FieldType>(dir[0] > 0 ? bounds2.X.Max : bounds2.X.Min);
    boundary[1] = static_cast<FieldType>(dir[1] > 0 ? bounds2.Y.Max : bounds2.Y.Min);
    boundary[2] = static_cast<FieldType>(dir[2] > 0 ? bounds2.Z.Max : bounds2.Z.Min);
  }

  VTKM_EXEC_CONT
  void GetTemporalBoundary(FieldType& boundary) const
  {
    // Return the time of the newest time slice
    boundary = time2;
  }

  /*
   * This method is intended to swap the initial datsets, by adding a new
   * dataset. The first dataset is replaced by the second, also the corresponding
   * times. The second dataset is replaced by a new dataset.
   */
  VTKM_CONT
  void UpdateDataSetForNewTimeSlice(const vtkm::cont::CoordinateSystem& coords,
                                    const vtkm::cont::DynamicCellSet& cellSet,
                                    const FieldHandle& vectorField,
                                    const FieldType time)
  {
    using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
    using StructuredType = vtkm::cont::CellSetStructured<3>;
    if (!coords.GetData().IsType<UniformType>())
      throw vtkm::cont::ErrorInternal("Coordinates are not uniform.");
    if (!cellSet.IsSameType(StructuredType()))
      throw vtkm::cont::ErrorInternal("Cells are not 3D structured.");
    vectors1 = vectors2;
    bounds1 = bounds2;
    dims1 = dims2;
    time1 = time2;
    planeSize1 = planeSize2;
    rowSize1 = rowSize2;
    scale1[0] = scale2[0];
    scale1[1] = scale2[1];
    scale1[2] = scale2[2];

    vectors2 = vectorField.PrepareForInput(DeviceAdapterTag());
    bounds2 = coords.GetBounds();
    vtkm::cont::CellSetStructured<3> cells;
    cellSet.CopyTo(cells);
    dims2 = cells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    time2 = time;
    planeSize2 = dims2[1] * dims2[1];
    rowSize2 = dims2[0];

    scale2[0] =
      static_cast<FieldType>(dims2[0] - 1) / static_cast<FieldType>(bounds2.X.Max - bounds2.X.Min);
    scale2[1] =
      static_cast<FieldType>(dims2[1] - 1) / static_cast<FieldType>(bounds2.Y.Max - bounds2.Y.Min);
    scale2[2] =
      static_cast<FieldType>(dims2[2] - 1) / static_cast<FieldType>(bounds2.Z.Max - bounds2.Z.Min);
  };

  VTKM_EXEC
  bool Interpolate(const vtkm::Vec<FieldType, 3>& position,
                   vtkm::Vec<FieldType, 3>& velocity,
                   const PortalType& vectors,
                   const vtkm::Bounds& bounds,
                   const vtkm::Id3 dims,
                   const vtkm::Id& planeSize,
                   const vtkm::Id& rowSize,
                   const vtkm::Vec<FieldType, 3>& scale) const
  {
    if (!bounds.Contains(position))
      return false;
    //Set the indices for the interpolation.
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;

    vtkm::Vec<FieldType, 3> normalized =
      vtkm::Vec<FieldType, 3>((position[0] - static_cast<FieldType>(bounds.X.Min)) * scale[0],
                              (position[1] - static_cast<FieldType>(bounds.Y.Min)) * scale[1],
                              (position[2] - static_cast<FieldType>(bounds.Z.Min)) * scale[2]);

    idx000[0] = static_cast<vtkm::IdComponent>(floor(normalized[0]));
    idx000[1] = static_cast<vtkm::IdComponent>(floor(normalized[1]));
    idx000[2] = static_cast<vtkm::IdComponent>(floor(normalized[2]));

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
    FieldType a = normalized[0] - static_cast<FieldType>(floor(normalized[0]));
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
    a = normalized[1] - static_cast<FieldType>(floor(normalized[1]));
    v0[0] = (1.0f - a) * v00[0] + a * v01[0];
    v0[1] = (1.0f - a) * v00[1] + a * v01[1];
    v0[2] = (1.0f - a) * v00[2] + a * v01[2];

    v1[0] = (1.0f - a) * v10[0] + a * v11[0];
    v1[1] = (1.0f - a) * v10[1] + a * v11[1];
    v1[2] = (1.0f - a) * v10[2] + a * v11[2];

    a = normalized[2] - static_cast<FieldType>(floor(normalized[2]));
    velocity[0] = (1.0f - a) * v0[0] + a * v1[0];
    velocity[1] = (1.0f - a) * v0[1] + a * v1[1];
    velocity[2] = (1.0f - a) * v0[2] + a * v1[2];
    return true;
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::Vec<FieldType, 3>& position,
                const FieldType particleTime,
                vtkm::Vec<FieldType, 3>& velocity) const
  {
    vtkm::Vec<FieldType, 3> velocity1;
    bool result;
    result =
      Interpolate(position, velocity1, vectors1, bounds1, dims1, planeSize1, rowSize1, scale1);
    if (!result)
      return false;

    vtkm::Vec<FieldType, 3> velocity2;
    result =
      Interpolate(position, velocity2, vectors2, bounds2, dims2, planeSize1, rowSize1, scale2);
    if (!result)
      return false;

    FieldType proportion = (particleTime - time1) / (time2 - time1);

    velocity[0] = (1.0f - proportion) * velocity1[0] + proportion * velocity2[0];
    velocity[1] = (1.0f - proportion) * velocity1[1] + proportion * velocity2[1];
    velocity[2] = (1.0f - proportion) * velocity1[2] + proportion * velocity2[2];
    return true;
  }

private:
  FieldType time1, time2;
  vtkm::Vec<FieldType, 3> scale1, scale2;
  /*
   * Currently only adding functionality to work with unifrom grids.
   * Reason being they are easy to work with.
   */
  vtkm::Bounds bounds1;
  vtkm::Bounds bounds2;
  vtkm::Id3 dims1;
  vtkm::Id3 dims2;
  /*
   * The resolution of the different slices of data may be
   * different, but needs to be uniform.
   */
  vtkm::Id planeSize1, planeSize2;
  vtkm::Id rowSize1, rowSize2;
  /*
   * These are the portals that contain the actual data for the interpolation.
   */
  PortalType vectors1;
  PortalType vectors2;
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm

#endif //vtk_m_worklet_particleadvection_TemporalGridEvaluators_h
