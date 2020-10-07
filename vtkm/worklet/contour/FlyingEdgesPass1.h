
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#ifndef vtk_m_worklet_contour_flyingedges_pass1_h
#define vtk_m_worklet_contour_flyingedges_pass1_h

#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/contour/FlyingEdgesHelpers.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{

/*
* Understanding Pass1 in general
*
* PASS 1: Process all of the voxel edges that compose each row. Determine the
* edges case classification, count the number of edge intersections, and
* figure out where intersections along the row begins and ends
* (i.e., gather information for computational trimming).
*
* So in general the algorithm selects a primary axis to stride ( X or Y).
* It does this by forming a plane along the other two axes and marching
* over the sum/primary axis.
*
* So for SumXAxis, this means that we form a YZ plane and march the
* X axis along each point. As we march we are looking at the X axis edge
* that is formed by the current and next point.
*
* So for SumYAxis, this means that we form a XZ plane and march the
* Y axis along each point. As we march we are looking at the Y axis edge
* that is formed by the current and next point.
*
*/

template <typename Device, typename WholeEdgeField>
inline VTKM_EXEC void write_edge(Device,
                                 vtkm::Id write_index,
                                 WholeEdgeField& edges,
                                 vtkm::UInt8 edgeCase)
{
  edges.Set(write_index, edgeCase);
}

template <typename WholeEdgeField>
inline VTKM_EXEC void write_edge(vtkm::cont::DeviceAdapterTagCuda,
                                 vtkm::Id write_index,
                                 WholeEdgeField& edges,
                                 vtkm::UInt8 edgeCase)
{
  if (edgeCase != FlyingEdges3D::Below)
  {
    edges.Set(write_index, edgeCase);
  }
}

template <typename T>
struct ComputePass1 : public vtkm::worklet::WorkletVisitPointsWithCells
{
  vtkm::Id3 PointDims;
  T IsoValue;

  ComputePass1() {}
  ComputePass1(T value, const vtkm::Id3& pdims)
    : PointDims(pdims)
    , IsoValue(value)
  {
  }

  using ControlSignature = void(CellSetIn,
                                FieldOut axis_sum,
                                FieldOut axis_min,
                                FieldOut axis_max,
                                WholeArrayInOut edgeData,
                                WholeArrayIn data);
  using ExecutionSignature = void(ThreadIndices, _2, _3, _4, _5, _6, Device);
  using InputDomain = _1;

  template <typename ThreadIndices,
            typename WholeEdgeField,
            typename WholeDataField,
            typename Device>
  VTKM_EXEC void operator()(const ThreadIndices& threadIndices,
                            vtkm::Id3& axis_sum,
                            vtkm::Id& axis_min,
                            vtkm::Id& axis_max,
                            WholeEdgeField& edges,
                            const WholeDataField& field,
                            Device device) const
  {
    using AxisToSum = typename select_AxisToSum<Device>::type;

    const vtkm::Id3 ijk = compute_ijk(AxisToSum{}, threadIndices.GetInputIndex3D());
    const vtkm::Id3 dims = this->PointDims;
    const vtkm::Id startPos = compute_start(AxisToSum{}, ijk, dims);
    const vtkm::Id offset = compute_inc(AxisToSum{}, dims);

    const T value = this->IsoValue;
    axis_min = this->PointDims[AxisToSum::xindex];
    axis_max = 0;
    T s1 = field.Get(startPos);
    T s0 = s1;
    axis_sum = { 0, 0, 0 };
    const vtkm::Id end = this->PointDims[AxisToSum::xindex] - 1;
    for (vtkm::Id i = 0; i < end; ++i)
    {
      s0 = s1;
      s1 = field.Get(startPos + (offset * (i + 1)));

      vtkm::UInt8 edgeCase = FlyingEdges3D::Below;
      if (s0 >= value)
      {
        edgeCase = FlyingEdges3D::LeftAbove;
      }
      if (s1 >= value)
      {
        edgeCase |= FlyingEdges3D::RightAbove;
      }

      write_edge(device, startPos + (offset * i), edges, edgeCase);

      if (edgeCase == FlyingEdges3D::LeftAbove || edgeCase == FlyingEdges3D::RightAbove)
      {
        axis_sum[AxisToSum::xindex] += 1; // increment number of intersections along axis
        axis_max = i + 1;
        if (axis_min == (end + 1))
        {
          axis_min = i;
        }
      }
    }
  }
};

struct launchComputePass1
{
  template <typename DeviceAdapterTag, typename T, typename StorageTagField, typename... Args>
  VTKM_CONT bool operator()(DeviceAdapterTag device,
                            const ComputePass1<T>& worklet,
                            const vtkm::cont::ArrayHandle<T, StorageTagField>& inputField,
                            vtkm::cont::ArrayHandle<vtkm::UInt8> edgeCases,
                            vtkm::cont::CellSetStructured<2>& metaDataMesh2D,
                            Args&&... args) const
  {
    vtkm::cont::Invoker invoke(device);
    metaDataMesh2D = make_metaDataMesh2D(SumXAxis{}, worklet.PointDims);

    invoke(worklet, metaDataMesh2D, std::forward<Args>(args)..., edgeCases, inputField);
    return true;
  }

  template <typename T, typename StorageTagField, typename... Args>
  VTKM_CONT bool operator()(vtkm::cont::DeviceAdapterTagCuda device,
                            const ComputePass1<T>& worklet,
                            const vtkm::cont::ArrayHandle<T, StorageTagField>& inputField,
                            vtkm::cont::ArrayHandle<vtkm::UInt8> edgeCases,
                            vtkm::cont::CellSetStructured<2>& metaDataMesh2D,
                            Args&&... args) const
  {
    vtkm::cont::Invoker invoke(device);
    metaDataMesh2D = make_metaDataMesh2D(SumYAxis{}, worklet.PointDims);

    vtkm::cont::Algorithm::Fill(edgeCases, static_cast<vtkm::UInt8>(FlyingEdges3D::Below));
    invoke(worklet, metaDataMesh2D, std::forward<Args>(args)..., edgeCases, inputField);
    return true;
  }
};
}
}
}


#endif
