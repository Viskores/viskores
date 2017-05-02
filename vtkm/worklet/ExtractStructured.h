//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_ExtractStructured_h
#define vtk_m_worklet_ExtractStructured_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>

namespace vtkm {
namespace worklet {

//
// Distribute input point/cell data to subset/subsample output data
//
struct DistributeData : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<> inIndices,
                                FieldOut<> outIndices);
  typedef void ExecutionSignature(_1, _2);

  typedef vtkm::worklet::ScatterCounting ScatterType;

  VTKM_CONT
  ScatterType GetScatter() const { return this->Scatter; }

  template <typename CountArrayType, typename DeviceAdapter>
  VTKM_CONT
  DistributeData(const CountArrayType &countArray,
                 DeviceAdapter device) :
                         Scatter(countArray, device) {  }

  template <typename T>
  VTKM_EXEC
  void operator()(T inputIndex,
                  T &outputIndex) const
  {
    outputIndex = inputIndex;
  }
private:
  ScatterType Scatter;
};

//
// Extract subset of structured grid and/or resample
//
class ExtractStructured
{
public:
  ExtractStructured() {}

  //
  // Create map of input points to output points with subset/subsample
  //
  class CreatePointMap : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> index,
                                  FieldOut<IdComponentType> passValue);
    typedef   _2 ExecutionSignature(_1);

    vtkm::Id RowSize;
    vtkm::Id PlaneSize;
    vtkm::Bounds OutBounds;
    vtkm::Id3 Sample;
    bool IncludeBoundary;

    VTKM_CONT
    CreatePointMap(const vtkm::Id3 &inDimension,
                   const vtkm::Bounds &outBounds,
                   const vtkm::Id3 &sample,
                         bool includeBoundary) :
                            RowSize(inDimension[0]),
                            PlaneSize(inDimension[0] * inDimension[1]),
                            OutBounds(outBounds),
                            Sample(sample),
                            IncludeBoundary(includeBoundary) {}

    VTKM_EXEC
    vtkm::IdComponent operator()(vtkm::Id index) const
    {
      vtkm::IdComponent passValue = 0;

      // Position of this point in the grid
      vtkm::Id k = index / PlaneSize;
      vtkm::Id j = (index % PlaneSize) / RowSize; 
      vtkm::Id i = index % RowSize;

      // Turn on points if within the subset bounding box
      vtkm::Id3 ijk = vtkm::Id3(i, j, k);
      if (OutBounds.Contains(ijk))
      {
        passValue = 1;
      }

      // Turn off points not within subsampling
      vtkm::Id3 minPt = vtkm::make_Vec(OutBounds.X.Min, OutBounds.Y.Min, OutBounds.Z.Min);
      vtkm::Id3 value = vtkm::make_Vec((i - minPt[0]) % Sample[0],
                                       (j - minPt[1]) % Sample[1],
                                       (k - minPt[2]) % Sample[2]);

      // If include boundary then max boundary is also within subsampling
      if (IncludeBoundary)
      {
        if (i == OutBounds.X.Max) value[0] = 0;
        if (j == OutBounds.Y.Max) value[1] = 0;
        if (k == OutBounds.Z.Max) value[2] = 0;
      }

      // If the value for the point is not 0 in all dimensions it is not in sample
      if (value != vtkm::Id3(0,0,0))
      {
        passValue = 0;
      }
      return passValue;
    }
  };

  //
  // Create map of input cells to output cells with subset/subsample
  //
  class CreateCellMap : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  WholeArrayIn<IdComponentType> pointMap,
                                  FieldOutCell<IdComponentType> passValue);
    typedef   _3 ExecutionSignature(PointCount, PointIndices, _2);

    vtkm::Id3 InDimension;
    vtkm::Id3 OutDimension;
    vtkm::Id RowSize;
    vtkm::Id PlaneSize;

    VTKM_CONT
    CreateCellMap(const vtkm::Id3 &inDimension,
                  const vtkm::Id3 &outDimension) :
                         InDimension(inDimension),
                         OutDimension(outDimension),
                         RowSize(inDimension[0]),
                         PlaneSize(inDimension[0] * inDimension[1]) {}

    template <typename ConnectivityInVec, typename InPointMap>
    VTKM_EXEC
    vtkm::IdComponent operator()(      vtkm::Id numIndices,
                                 const ConnectivityInVec &connectivityIn,
                                 const InPointMap &pointMap) const
    {
      // If all surrounding points are in the subset, cell will be also
      vtkm::IdComponent passValue = 1;
      for (vtkm::IdComponent indx = 0; indx < numIndices; indx++)
      {
        if (pointMap.Get(connectivityIn[indx]) == 0)
        {
          passValue = 0;
        }
      }

      // Cell might still be in subset through subsampling
      if (passValue == 0)
      {
        // If the lower left point is in the sample it is a candidate for subsample
        vtkm::Id ptId = connectivityIn[0];
        if (pointMap.Get(ptId) == 1)
        {
          vtkm::Id3 position((ptId % RowSize), ((ptId % PlaneSize) / RowSize), (ptId / PlaneSize));
          vtkm::Id newPtId;
          vtkm::Id3 foundValidPoint(0,0,0);
          vtkm::Id3 offset(1, RowSize, PlaneSize);

          for (vtkm::IdComponent dim = 0; dim < 3; dim++)
          {
            if (OutDimension[dim] == 1)
            {
              foundValidPoint[dim] = 1;
            }
            else
            {
              // Check down the dimension for one other sampled point to complete cell
              newPtId = ptId + offset[dim];
              vtkm::Id indx = position[dim] + 1;
              bool done = false;
              while (indx < InDimension[dim] && done == false)
              {
                if (pointMap.Get(newPtId) == 1)
                {
                  foundValidPoint[dim] = 1;
                  done = true;
                }
                indx++;
                newPtId += offset[dim];
              }
            }
          }
       
          // If there is a valid point in all dimensions cell is in sample
          if (foundValidPoint == vtkm::Id3(1,1,1))
          {
            passValue = 1;
          }
        }
      }
      return passValue;
    }
  };

  //
  // Uniform Structured
  //
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::DataSet ExtractUniform(
                                vtkm::IdComponent outDim,
                          const CellSetType &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::Bounds &outBounds,
                          const vtkm::Id3 &sample,
                                bool includeBoundary,
                                DeviceAdapter)
  {
    typedef vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;

    // Cast dynamic coordinate data to Uniform type
    vtkm::cont::DynamicArrayHandleCoordinateSystem coordinateData = coordinates.GetData();
    UniformArrayHandle inCoordinates;
    inCoordinates = coordinateData.Cast<UniformArrayHandle>();

    // Portal to access data in the input coordinate system
    UniformConstPortal Coordinates = inCoordinates.PrepareForInput(DeviceAdapter());

    // Sizes and values of input Uniform Structured
    vtkm::Id3 inDimension = Coordinates.GetDimensions();

    // Calculate output subset dimension
    // minBound will not change because first point or cell is always included
    // maxBound is the same if no sampling, or if sample point lands on boundary,
    //          or if include boundary is set
    // Otherwise maxBound will be the last stride point
    vtkm::Id3 lastIndex = vtkm::make_Vec(outBounds.X.Max - outBounds.X.Min,
                                         outBounds.Y.Max - outBounds.Y.Min,
                                         outBounds.Z.Max - outBounds.Z.Min);
    vtkm::Id3 outDimension = lastIndex + vtkm::Id3(1,1,1);

    // Adjust for sampling and include boundary
    for (vtkm::IdComponent dim = 0; dim < outDim; dim++)
    {
      if (sample[dim] != 1)
      {
        outDimension[dim] = 1 + (lastIndex[dim] / sample[dim]);
        if (includeBoundary == true && (lastIndex[dim] % sample[dim] != 0))
        {
          outDimension[dim] += 1;
        }
      }
    }

    vtkm::Vec<vtkm::FloatDefault,3> outOrigin = vtkm::make_Vec(0,0,0);
    vtkm::Vec<vtkm::FloatDefault,3> outSpacing = vtkm::make_Vec(1,1,1);

    // Create output dataset which needs modified coordinate system and cellset
    vtkm::cont::DataSet output;

    // Set the output CoordinateSystem information
    UniformArrayHandle outCoordinateData(outDimension, outOrigin, outSpacing);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    // Set the output cellset
    if (outDim == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[0], 
                                                   outDimension[1], 
                                                   outDimension[2]));
      output.AddCellSet(outCellSet);
    }

    else if (outDim == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      if (outDimension[2] == 1)      // XY plane
      {
        outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[0], 
                                                     outDimension[1]));
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[1] == 1) // XZ plane
      {
        outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[0], 
                                                     outDimension[2]));
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[0] == 1) // YZ plane
      {
        outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[1], 
                                                     outDimension[2]));
        output.AddCellSet(outCellSet);
      }
    }

    else if (outDim == 1)
    {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      if (outDimension[1] == 1 && outDimension[2] == 1)
      {
        outCellSet.SetPointDimensions(outDimension[0]);
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[0] == 1 && outDimension[2] == 1)
      {
        outCellSet.SetPointDimensions(outDimension[1]);
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[0] == 1 && outDimension[1] == 1)
      {
        outCellSet.SetPointDimensions(outDimension[2]);
        output.AddCellSet(outCellSet);
      }
    }

    // Create the map for the input point data to output
    vtkm::cont::ArrayHandleIndex pointIndices(cellSet.GetNumberOfPoints());
    CreatePointMap pointMap(inDimension, 
                            outBounds, 
                            sample, 
                            includeBoundary);
    vtkm::worklet::DispatcherMapField<CreatePointMap> pointDispatcher(pointMap);
    pointDispatcher.Invoke(pointIndices,
                           this->PointMap);

    // Create the map for the input cell data to output
    CreateCellMap cellMap(inDimension, 
                          outDimension);
    vtkm::worklet::DispatcherMapTopology<CreateCellMap> cellDispatcher(cellMap);
    cellDispatcher.Invoke(cellSet,
                          this->PointMap,
                          this->CellMap);

    return output;
  }

  //
  // Rectilinear Structured
  //
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::DataSet ExtractRectilinear(
                                vtkm::IdComponent outDim,
                          const CellSetType &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::Bounds &outBounds,
                          const vtkm::Id3 &sample,
                                bool includeBoundary,
                                DeviceAdapter)
  {
    typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> DefaultHandle;
    typedef vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle,DefaultHandle,DefaultHandle> CartesianArrayHandle;
    typedef typename DefaultHandle::ExecutionTypes<DeviceAdapter>::PortalConst DefaultConstHandle;
    typedef typename CartesianArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst CartesianConstPortal;

    // Cast dynamic coordinate data to Rectilinear type
    vtkm::cont::DynamicArrayHandleCoordinateSystem coordinateData = coordinates.GetData();
    CartesianArrayHandle inCoordinates;
    inCoordinates = coordinateData.Cast<CartesianArrayHandle>();

    CartesianConstPortal Coordinates = inCoordinates.PrepareForInput(DeviceAdapter());
    DefaultConstHandle X = Coordinates.GetFirstPortal();
    DefaultConstHandle Y = Coordinates.GetSecondPortal();
    DefaultConstHandle Z = Coordinates.GetThirdPortal();

    vtkm::Id3 inDimension(X.GetNumberOfValues(), 
                          Y.GetNumberOfValues(), 
                          Z.GetNumberOfValues());

    // Calculate output subset dimension
    vtkm::Id3 lastIndex = vtkm::make_Vec(outBounds.X.Max - outBounds.X.Min,
                                         outBounds.Y.Max - outBounds.Y.Min,
                                         outBounds.Z.Max - outBounds.Z.Min);
    vtkm::Id3 outDimension = lastIndex + vtkm::Id3(1,1,1);

    // Adjust for sampling and include boundary
    for (vtkm::IdComponent dim = 0; dim < outDim; dim++)
    {
      if (sample[dim] != 1)
      {
        outDimension[dim] = 1 + lastIndex[dim] / sample[dim];
        if (includeBoundary == true && (lastIndex[dim] % sample[dim] != 0))
        {
          outDimension[dim] += 1;
        }
      }
    }

    // Set output coordinate system
    DefaultHandle Xc, Yc, Zc;
    Xc.Allocate(outDimension[0]);
    Yc.Allocate(outDimension[1]);
    Zc.Allocate(outDimension[2]);

    vtkm::Id indx = 0;
    vtkm::Id3 minBound = vtkm::make_Vec(outBounds.X.Min, outBounds.Y.Min, outBounds.Z.Min);
    vtkm::Id3 maxBound = vtkm::make_Vec(outBounds.X.Max, outBounds.Y.Max, outBounds.Z.Max);
    for (vtkm::Id x = minBound[0]; x <= maxBound[0]; x++)
    {
      if ((x % sample[0]) == 0)
      {
        Xc.GetPortalControl().Set(indx++, X.Get(x));
      }
    }
    indx = 0;
    for (vtkm::Id y = minBound[1]; y <= maxBound[1]; y++)
    {
      if ((y % sample[1]) == 0)
      {
        Yc.GetPortalControl().Set(indx++, Y.Get(y));
      }
    }
    indx = 0;
    for (vtkm::Id z = minBound[2]; z <= maxBound[2]; z++)
    {
      if ((z % sample[2]) == 0)
      {
        Zc.GetPortalControl().Set(indx++, Z.Get(z));
      }
    }

    // Create output dataset which needs modified coordinate system and cellset
    vtkm::cont::DataSet output;

    // Set the output CoordinateSystem information
    CartesianArrayHandle outCoordinateData(Xc, Yc, Zc);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    // Set the output cellset
    if (outDim == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[0],
                                                   outDimension[1],
                                                   outDimension[2]));
      output.AddCellSet(outCellSet);
    }

    else if (outDim == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      if (outDimension[2] == 1)      // XY plane
      {
        outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[0],
                                                     outDimension[1]));
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[1] == 1) // XZ plane
      {
        outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[0],
                                                     outDimension[2]));
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[0] == 1) // YZ plane
      {
        outCellSet.SetPointDimensions(vtkm::make_Vec(outDimension[1],
                                                     outDimension[2]));
        output.AddCellSet(outCellSet);
      }
    }

    else if (outDim == 1)
    {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      if (outDimension[1] == 1 && outDimension[2] == 1)
      {
        outCellSet.SetPointDimensions(outDimension[0]);
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[0] == 1 && outDimension[2] == 1)
      {
        outCellSet.SetPointDimensions(outDimension[1]);
        output.AddCellSet(outCellSet);
      }
      else if (outDimension[0] == 1 && outDimension[1] == 1)
      {
        outCellSet.SetPointDimensions(outDimension[2]);
        output.AddCellSet(outCellSet);
      }
    }

    // Create the map for the input point data to output
    vtkm::cont::ArrayHandleIndex pointIndices(cellSet.GetNumberOfPoints());
    CreatePointMap pointMap(inDimension, 
                            outBounds, 
                            sample, 
                            includeBoundary);
    vtkm::worklet::DispatcherMapField<CreatePointMap> pointDispatcher(pointMap);
    pointDispatcher.Invoke(pointIndices,
                           this->PointMap);

    // Create the map for the input cell data to output
    CreateCellMap cellMap(inDimension, 
                          outDimension);
    vtkm::worklet::DispatcherMapTopology<CreateCellMap> cellDispatcher(cellMap);
    cellDispatcher.Invoke(cellSet,
                          this->PointMap,
                          this->CellMap);

    return output;
  }

  //
  // Run extract structured on uniform or rectilinear, subset and/or subsample
  //
  template <typename DeviceAdapter>
  vtkm::cont::DataSet Run(const vtkm::cont::DynamicCellSet &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::Bounds &boundingBox,
                          const vtkm::Id3 &sample,
                                bool includeBoundary,
                                DeviceAdapter)
  {
    // Check legality of input cellset and set input dimension
    vtkm::IdComponent inDim = 0;
    if (cellSet.IsSameType(vtkm::cont::CellSetStructured<1>()))
    {
      inDim = 1;
    }
    else if (cellSet.IsSameType(vtkm::cont::CellSetStructured<2>()))
    {
      inDim = 2;
    }
    else if (cellSet.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      inDim = 3;
    }
    else
    {
      throw vtkm::cont::ErrorBadType("Only Structured cell sets allowed");
      return vtkm::cont::DataSet();
    }

    // Check legality of requested bounds
    if (boundingBox.IsNonEmpty() == false)
    {
      throw vtkm::cont::ErrorBadValue("Requested bounding box is not valid");
      return vtkm::cont::DataSet();
    }

    // Check legality of sampling
    if (sample[0] < 1 || sample[1] < 1 || sample[2] < 1)
    {
      throw vtkm::cont::ErrorBadValue("Requested sampling is not valid");
      return vtkm::cont::DataSet();
    }

    // Requested bounding box intersection with input bounding box
    vtkm::Bounds inBounds = coordinates.GetBounds();
    vtkm::Bounds outBounds = boundingBox;

    if (outBounds.X.Min < inBounds.X.Min)
      outBounds.X.Min = inBounds.X.Min;
    if (outBounds.X.Max > inBounds.X.Max)
      outBounds.X.Max = inBounds.X.Max;
    if (outBounds.Y.Min < inBounds.Y.Min)
      outBounds.Y.Min = inBounds.Y.Min;
    if (outBounds.Y.Max > inBounds.Y.Max)
      outBounds.Y.Max = inBounds.Y.Max;
    if (outBounds.Z.Min < inBounds.Z.Min)
      outBounds.Z.Min = inBounds.Z.Min;
    if (outBounds.Z.Max > inBounds.Z.Max)
      outBounds.Z.Max = inBounds.Z.Max;

    // Bounding box intersects
    if (outBounds.IsNonEmpty() == false)
    {
      throw vtkm::cont::ErrorBadValue("Bounding box does not intersect input");
      return vtkm::cont::DataSet();
    }

    // Set output dimension based on bounding box and input dimension
    vtkm::IdComponent outDim = 0;
    if (outBounds.X.Min < outBounds.X.Max)
      outDim++;
    if (outBounds.Y.Min < outBounds.Y.Max)
      outDim++;
    if (outBounds.Z.Min < outBounds.Z.Max)
      outDim++;
    if (outDim > inDim)
      outDim = inDim;

    // Uniform, Regular or Rectilinear
    typedef vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    bool IsUniformDataSet = 0;
    if (coordinates.GetData().IsSameType(UniformArrayHandle()))
    {
      IsUniformDataSet = true;
    }
    if (IsUniformDataSet)
    {
      return ExtractUniform(outDim,
                            cellSet,
                            coordinates,
                            outBounds,
                            sample,
                            includeBoundary,
                            DeviceAdapter());
    }
    else
    {
      return ExtractRectilinear(outDim,
                                cellSet,
                                coordinates,
                                outBounds,
                                sample,
                                includeBoundary,
                                DeviceAdapter());
    }
  }

  //
  // Subset and/or subsampling of Point Data
  //
  template <typename T,
            typename StorageType,
            typename DeviceAdapter>
  vtkm::cont::ArrayHandle<T, StorageType> ProcessPointField(
                                            const vtkm::cont::ArrayHandle<T, StorageType> &input,
                                                  DeviceAdapter device)
  {
    vtkm::cont::ArrayHandle<T, StorageType> output;

    DistributeData distribute(this->PointMap, device);
    vtkm::worklet::DispatcherMapField<DistributeData, DeviceAdapter> dispatch(distribute);
    dispatch.Invoke(input, output);
    return output;
  }

  //
  // Subset and/or subsampling of Cell Data
  //
  template <typename T,
            typename StorageType,
            typename DeviceAdapter>
  vtkm::cont::ArrayHandle<T, StorageType> ProcessCellField(
                                            const vtkm::cont::ArrayHandle<T, StorageType> &input,
                                                  DeviceAdapter device)
  {
    vtkm::cont::ArrayHandle<T, StorageType> output;

    DistributeData distribute(this->CellMap, device);
    vtkm::worklet::DispatcherMapField<DistributeData, DeviceAdapter> dispatch(distribute);
    dispatch.Invoke(input, output);
    return output;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::IdComponent> PointMap;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> CellMap;
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ExtractStructured_h
