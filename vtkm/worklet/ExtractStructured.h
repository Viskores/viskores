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
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/ScatterCounting.h>

namespace vtkm {
namespace worklet {

//
// Distribute input point/cell data to subset output point/cell data
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
  ExtractStructured() :
    DoSubset(false),
    DoSubsample(false) {}

  // Determine if index is within range of the subset
  class CreateDataMap : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> index,
                                  FieldOut<IdComponentType> passValue);
    typedef   _2 ExecutionSignature(_1);

    vtkm::Id RowSize;
    vtkm::Id PlaneSize;
    vtkm::Id3 MinBound;
    vtkm::Id3 MaxBound;

    VTKM_CONT
    CreateDataMap(const vtkm::Id3 dimensions,
                  const vtkm::Id3 &minBound,
                  const vtkm::Id3 &maxBound) :
                         RowSize(dimensions[1]),
                         PlaneSize(dimensions[1] * dimensions[0]),
                         MinBound(minBound),
                         MaxBound(maxBound) {}

    VTKM_EXEC
    vtkm::IdComponent operator()(const vtkm::Id index) const
    {
      vtkm::IdComponent passValue = 0;
      vtkm::IdComponent z = index / PlaneSize;
      vtkm::IdComponent y = (index % PlaneSize) / RowSize; 
      vtkm::IdComponent x = index % RowSize;
      if (MinBound[0] <= x && x <= MaxBound[0] &&
          MinBound[1] <= y && y <= MaxBound[1] &&
          MinBound[2] <= z && z <= MaxBound[2])
      {
            passValue = 1;
      }
      return passValue;
    }
  };

  // Create maps for mapping point and cell data to subset
  template <typename DeviceAdapter>
  void CreatePointCellMaps(
                      const vtkm::Id3 &Dimensions,
                      const vtkm::Id &numberOfPoints,
                      const vtkm::Id &numberOfCells,
                      const vtkm::Id3 &minBound,
                      const vtkm::Id3 &maxBound,
                      const DeviceAdapter)
  {
    vtkm::cont::ArrayHandleIndex pointIndices(numberOfPoints);
    vtkm::cont::ArrayHandleIndex cellIndices(numberOfCells);

    // Create the map for the point data
    CreateDataMap pointWorklet(Dimensions, minBound, maxBound);
    vtkm::worklet::DispatcherMapField<CreateDataMap> pointDispatcher(pointWorklet);
    pointDispatcher.Invoke(pointIndices,
                           this->OutPointMap);
for (vtkm::Id i = 0; i < numberOfPoints; i++)
std::cout << "Point " << i << " passed " << OutPointMap.GetPortalControl().Get(i) << std::endl;

    // Create the map for the cell data
    vtkm::Id3 CellDimensions(Dimensions[0] - 1, Dimensions[1] - 1, Dimensions[2] - 1);
/*
    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellBounds;
    DeviceAlgorithm::Copy(bounds, cellBounds);
    cellBounds.GetPortalControl().Set(1, bounds.GetPortalConstControl().Get(1) - 1);
    cellBounds.GetPortalControl().Set(3, bounds.GetPortalConstControl().Get(3) - 1);
    cellBounds.GetPortalControl().Set(5, bounds.GetPortalConstControl().Get(5) - 1);
*/

    CreateDataMap cellWorklet(CellDimensions, minBound, maxBound - vtkm::Id3(1,1,1));
    vtkm::worklet::DispatcherMapField<CreateDataMap> cellDispatcher(cellWorklet);
    cellDispatcher.Invoke(cellIndices,
                          this->OutCellMap);
for (vtkm::Id i = 0; i < numberOfCells; i++)
std::cout << "Cell " << i << " passed " << OutCellMap.GetPortalControl().Get(i) << std::endl;
  }

  // Uniform Structured
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::DataSet ExtractUniform(
                          const vtkm::IdComponent dimension,
                          const CellSetType &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::Id3 &minBound,
                          const vtkm::Id3 &maxBound,
                          const vtkm::Id3 &sample,
                          DeviceAdapter)
  {
    typedef vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;

    // Data in the Field attached to CoordinateSystem is dynamic
    vtkm::cont::DynamicArrayHandleCoordinateSystem coordinateData = coordinates.GetData();

    // Cast dynamic coordinate data to Uniform type
    UniformArrayHandle vertices;
    vertices = coordinateData.Cast<UniformArrayHandle>();

    std::cout << "Uniform vertices:" << std::endl;
    printSummary_ArrayHandle(vertices, std::cout);

    // Portal to access data in the input coordinate system
    UniformConstPortal Coordinates = vertices.PrepareForInput(DeviceAdapter());

    // Sizes and values of input Uniform Structured
    vtkm::Id3 Dimension = Coordinates.GetDimensions();
    vtkm::Vec<vtkm::FloatDefault,3> Origin = Coordinates.GetOrigin();
    vtkm::Vec<vtkm::FloatDefault,3> Spacing = Coordinates.GetSpacing();
    std::cout << "UNIFORM DIMENSIONS " << Dimension << std::endl;
    std::cout << "UNIFORM ORIGIN " << Origin << std::endl;
    std::cout << "UNIFORM SPACING " << Spacing << std::endl;

    // Sizes and values of output Uniform Structured
    vtkm::Id3 OutDimension = maxBound - minBound + vtkm::Id3(1,1,1);
    vtkm::Vec<vtkm::FloatDefault,3> OutOrigin = Origin;
    vtkm::Vec<vtkm::FloatDefault,3> OutSpacing = Spacing;
    std::cout << "UNIFORM OUT DIMENSIONS " << OutDimension << std::endl;
    std::cout << "UNIFORM OUT ORIGIN " << OutOrigin << std::endl;
    std::cout << "UNIFORM OUT SPACING " << OutSpacing << std::endl;

    // Verify requested bounds are contained within the original input dataset
    VTKM_ASSERT((minBound[0] >= 0 && maxBound[0] <= Dimension[0]) &&
                (minBound[1] >= 0 && maxBound[1] <= Dimension[1]) &&
                (minBound[2] >= 0 && maxBound[2] <= Dimension[2]));

    // Is the output a subset
    if (OutDimension[0] <= Dimension[0] &&
        OutDimension[1] <= Dimension[1] &&
        OutDimension[2] <= Dimension[2])
    {
      this->DoSubset = true;
std::cout << "SUBSETTING" << std::endl;
    }
    else
    {
std::cout << "No SUBSETTING" << std::endl;
    }
         
    vtkm::cont::DataSet output;

    // Set the output CoordinateSystem information
    UniformArrayHandle outCoordinateData(OutDimension, OutOrigin, OutSpacing);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    std::cout << "CoordinateSystem for output:" << std::endl;
    outCoordinates.PrintSummary(std::cout);

    // Set the size of the cell set for Uniform
    if (dimension == 1) {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(OutDimension[0]);
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(OutDimension[0], 
                                                   OutDimension[1]));
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(OutDimension[0], 
                                                   OutDimension[1], 
                                                   OutDimension[2]));
      output.AddCellSet(outCellSet);
    }

std::cout << "Number of Cells " << output.GetCellSet(0).GetNumberOfCells() << std::endl;
std::cout << "Number of Points " << output.GetCellSet(0).GetNumberOfPoints() << std::endl;

    // Calculate and save the maps of point and cell data to subset
    CreatePointCellMaps(Dimension, 
                        cellSet.GetNumberOfPoints(),
                        cellSet.GetNumberOfCells(),
                        minBound,
                        maxBound,
                        DeviceAdapter());

    return output;
  }

  // Rectilinear Structured
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::DataSet ExtractRectilinear(
                          const vtkm::IdComponent dimension,
                          const CellSetType &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::Id3 &minBound,
                          const vtkm::Id3 &maxBound,
                          const vtkm::Id3 &sample,
                          DeviceAdapter)
  {
    typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> DefaultHandle;
    typedef vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle,DefaultHandle,DefaultHandle> CartesianArrayHandle;
    typedef typename DefaultHandle::ExecutionTypes<DeviceAdapter>::PortalConst DefaultConstHandle;
    typedef typename CartesianArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst CartesianConstPortal;

    // Data in the Field attached to CoordinateSystem is dynamic
    vtkm::cont::DynamicArrayHandleCoordinateSystem coordinateData = coordinates.GetData();

    // Cast dynamic coordinate data to Rectilinear type
    CartesianArrayHandle vertices;
    vertices = coordinateData.Cast<CartesianArrayHandle>();

    std::cout << "Recilinear vertices:" << std::endl;
    printSummary_ArrayHandle(vertices, std::cout);

    CartesianConstPortal Coordinates = vertices.PrepareForInput(DeviceAdapter());

    vtkm::Id NumberOfValues = Coordinates.GetNumberOfValues();
    std::cout << "RECTILINEAR NumberOfValues " << NumberOfValues << std::endl;

    DefaultConstHandle X = Coordinates.GetFirstPortal();
    DefaultConstHandle Y = Coordinates.GetSecondPortal();
    DefaultConstHandle Z = Coordinates.GetThirdPortal();

    vtkm::Id3 Dimension(X.GetNumberOfValues(), 
                        Y.GetNumberOfValues(), 
                        Z.GetNumberOfValues());
    std::cout << "Number of x coordinates " << Dimension[0] << std::endl;
    std::cout << "Number of y coordinates " << Dimension[1] << std::endl;
    std::cout << "Number of z coordinates " << Dimension[2] << std::endl;

    for (vtkm::Id x = 0; x < Dimension[0]; x++)
      std::cout << "X " << x << " = " << X.Get(x) << std::endl;
    for (vtkm::Id y = 0; y < Dimension[1]; y++)
      std::cout << "Y " << y << " = " << Y.Get(y) << std::endl;
    for (vtkm::Id z = 0; z < Dimension[2]; z++)
      std::cout << "Z " << z << " = " << Z.Get(z) << std::endl;

    // Verify requested bounds are contained within the original input dataset
    VTKM_ASSERT((minBound[0] >= 0 && maxBound[0] <= Dimension[0]) &&
                (minBound[1] >= 0 && maxBound[1] <= Dimension[1]) &&
                (minBound[2] >= 0 && maxBound[2] <= Dimension[2]));

    vtkm::cont::DataSet output;

    // Sizes and values of output Rectilinear Structured
    vtkm::Id3 OutDimension = maxBound - minBound + vtkm::Id3(1,1,1);
    std::cout << "RECTILINEAR OUT DIMENSIONS " << OutDimension << std::endl;

    // Set output coordinate system
    DefaultHandle Xc, Yc, Zc;
    Xc.Allocate(OutDimension[0]);
    Yc.Allocate(OutDimension[1]);
    Zc.Allocate(OutDimension[2]);

    vtkm::Id indx = 0;
    for (vtkm::Id x = minBound[0]; x <= maxBound[0]; x++)
    {
      Xc.GetPortalControl().Set(indx++, X.Get(x));
    }
    indx = 0;
    for (vtkm::Id y = minBound[1]; y <= maxBound[1]; y++)
    {
      Yc.GetPortalControl().Set(indx++, Y.Get(y));
    }
    indx = 0;
    for (vtkm::Id z = minBound[2]; z <= maxBound[2]; z++)
    {
      Zc.GetPortalControl().Set(indx++, Z.Get(z));
    }

    for (vtkm::Id x = 0; x < OutDimension[0]; x++)
      std::cout << "Xc " << x << " = " << Xc.GetPortalControl().Get(x) << std::endl;
    for (vtkm::Id y = 0; y < OutDimension[1]; y++)
      std::cout << "Yc " << y << " = " << Yc.GetPortalControl().Get(y) << std::endl;
    for (vtkm::Id z = 0; z < OutDimension[2]; z++)
      std::cout << "Zc " << z << " = " << Zc.GetPortalControl().Get(z) << std::endl;

    CartesianArrayHandle outCoordinateData(Xc, Yc, Zc);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    std::cout << "CoordinateSystem for output:" << std::endl;
    outCoordinates.PrintSummary(std::cout);

    // Set the size of the cell set for Rectilinear
    if (dimension == 1) {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(OutDimension[0]);
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(OutDimension[0], 
                                                   OutDimension[1]));
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(OutDimension[0], 
                                                   OutDimension[1], 
                                                   OutDimension[2]));
      output.AddCellSet(outCellSet);
    }

std::cout << "Number of Cells " << output.GetCellSet(0).GetNumberOfCells() << std::endl;
std::cout << "Number of Points " << output.GetCellSet(0).GetNumberOfPoints() << std::endl;

    // Calculate and save the maps of point and cell data to subset
    CreatePointCellMaps(Dimension, 
                        cellSet.GetNumberOfPoints(),
                        cellSet.GetNumberOfCells(),
                        minBound,
                        maxBound,
                        DeviceAdapter());

    return output;
  }

  // Run extract structured on uniform or rectilinear, subset and subsample
  template <typename DeviceAdapter>
  vtkm::cont::DataSet Run(const vtkm::cont::DynamicCellSet &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::Id3 &minBound,
                          const vtkm::Id3 &maxBound,
                          const vtkm::Id3 &sample,
                          DeviceAdapter)
  {
    vtkm::IdComponent dimension;
    if (cellSet.IsSameType(vtkm::cont::CellSetStructured<1>()))
    {
      dimension = 1;
    }
    else if (cellSet.IsSameType(vtkm::cont::CellSetStructured<2>()))
    {
      dimension = 2;
    }
    else if (cellSet.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      dimension = 3;
    }
    else
    {
      throw vtkm::cont::ErrorBadType("Only Structured cell sets allowed");
      return vtkm::cont::DataSet();
    }
    std::cout << "DIMENSION " << dimension << std::endl;

    // Subsample required
    if (sample == vtkm::Id3(1,1,1))
    {
      this->DoSubsample = false;
std::cout << "No SUBSAMPLING" << std::endl;
    }
    else
    {
      VTKM_ASSERT(sample[0] >= 1 && sample[1] >= 1 && sample[2] >= 1);
      this->DoSubsample = true;
std::cout << "SUBSAMPLING" << std::endl;
    }

    // Uniform Structured
    typedef vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    bool IsUniformDataSet = 0;
    if (coordinates.GetData().IsSameType(UniformArrayHandle()))
    {
      IsUniformDataSet = true;
    }
    std::cout << "IsUniformDataSet " << IsUniformDataSet << std::endl;

    std::cout << "CoordinateSystem::Field GetName " << coordinates.GetName() << std::endl;
    std::cout << "CoordinateSystem::Field GetAssociation " << coordinates.GetAssociation() << std::endl;
    vtkm::Bounds inBounds = coordinates.GetBounds();
    std::cout << "Bounds " << inBounds << std::endl;
    std::cout << "CoordinateSystem for input:" << std::endl;
    coordinates.PrintSummary(std::cout);
    std::cout << std::endl;

    if (IsUniformDataSet)
    {
      return ExtractUniform(dimension,
                            cellSet,
                            coordinates,
                            minBound,
                            maxBound,
                            sample,
                            DeviceAdapter());
    }
    else
    {
      return ExtractRectilinear(dimension,
                                cellSet,
                                coordinates,
                                minBound,
                                maxBound,
                                sample,
                                DeviceAdapter());
    }
  }

  // Subset of Point Data
  template <typename T,
            typename StorageType,
            typename DeviceAdapter>
  vtkm::cont::ArrayHandle<T, StorageType> ProcessPointField(
                                            const vtkm::cont::ArrayHandle<T, StorageType> &input,
                                            const DeviceAdapter& device)
  {
    vtkm::cont::ArrayHandle<T, StorageType> output;

    DistributeData distribute(this->OutPointMap, device);
    vtkm::worklet::DispatcherMapField<DistributeData, DeviceAdapter> dispatcher(distribute);
    dispatcher.Invoke(input, output);

    return output;
  }

  // Subset of Cell Data
  template <typename T,
            typename StorageType,
            typename DeviceAdapter>
  vtkm::cont::ArrayHandle<T, StorageType> ProcessCellField(
                                            const vtkm::cont::ArrayHandle<T, StorageType> &input,
                                            const DeviceAdapter& device)
  {
    vtkm::cont::ArrayHandle<T, StorageType> output;

    DistributeData distribute(this->OutCellMap, device);
    vtkm::worklet::DispatcherMapField<DistributeData, DeviceAdapter> dispatcher(distribute);
    dispatcher.Invoke(input, output);

    return output;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::IdComponent> OutPointMap;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> OutCellMap;
  bool DoSubset;
  bool DoSubsample;
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ExtractStructured_h
