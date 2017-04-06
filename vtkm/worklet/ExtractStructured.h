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
  ExtractStructured() {}

  struct BoolType : vtkm::ListTagBase<bool> {};

  // Determine if index is within range of the subset
  class CreateDataMap : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> index,
                                  WholeArrayIn<IdType> bounds,
                                  WholeArrayIn<IdType> sample,
                                  FieldOut<IdComponentType> passValue);
    typedef   _4 ExecutionSignature(_1, _2, _3);

    vtkm::Id RowSize;
    vtkm::Id PlaneSize;

    VTKM_CONT
    CreateDataMap(const vtkm::Id3 dimensions) :
                         RowSize(dimensions[1]),
                         PlaneSize(dimensions[1] * dimensions[0]) {}

    template <typename InFieldPortalType>
    VTKM_EXEC
    vtkm::IdComponent operator()(const vtkm::Id index,
                                 const InFieldPortalType bounds,
                                 const InFieldPortalType sample) const
    {
      vtkm::IdComponent passValue = 0;
      vtkm::IdComponent z = index / PlaneSize;
      vtkm::IdComponent y = (index % PlaneSize) / RowSize; 
      vtkm::IdComponent x = index % RowSize;
      if (bounds.Get(0) <= x && x <= bounds.Get(1) &&
          bounds.Get(2) <= y && y <= bounds.Get(3) &&
          bounds.Get(4) <= z && z <= bounds.Get(5))
            passValue = 1;
      return passValue;
    }
  };

  // Create maps for mapping point and cell data to subset
  template <typename DeviceAdapter>
  void CreatePointCellMaps(
                      const vtkm::Id3 &Dimensions,
                      const vtkm::Id &numberOfPoints,
                      const vtkm::Id &numberOfCells,
                      const vtkm::cont::ArrayHandle<vtkm::IdComponent> &bounds,
                      const vtkm::cont::ArrayHandle<vtkm::IdComponent> &sample,
                      const DeviceAdapter)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

    vtkm::cont::ArrayHandleIndex pointIndices(numberOfPoints);
    vtkm::cont::ArrayHandleIndex cellIndices(numberOfCells);

    // Create the map for the point data
    CreateDataMap pointWorklet(Dimensions);
    vtkm::worklet::DispatcherMapField<CreateDataMap> pointDispatcher(pointWorklet);
    pointDispatcher.Invoke(pointIndices,
                           bounds,
                           sample,
                           this->OutPointMap);
for (vtkm::Id i = 0; i < numberOfPoints; i++)
std::cout << "Point " << i << " passed " << OutPointMap.GetPortalControl().Get(i) << std::endl;

    // Create the map for the cell data
    vtkm::Id3 CellDimensions(Dimensions[0] - 1, Dimensions[1] - 1, Dimensions[2] - 1);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellBounds;
    DeviceAlgorithm::Copy(bounds, cellBounds);
    cellBounds.GetPortalControl().Set(1, bounds.GetPortalConstControl().Get(1) - 1);
    cellBounds.GetPortalControl().Set(3, bounds.GetPortalConstControl().Get(3) - 1);
    cellBounds.GetPortalControl().Set(5, bounds.GetPortalConstControl().Get(5) - 1);

    CreateDataMap cellWorklet(CellDimensions);
    vtkm::worklet::DispatcherMapField<CreateDataMap> cellDispatcher(cellWorklet);
    cellDispatcher.Invoke(cellIndices,
                          cellBounds,
                          sample,
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
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &bounds,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &sample,
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
    vtkm::Id3 Dimensions = Coordinates.GetDimensions();
    vtkm::Vec<vtkm::FloatDefault,3> Origin = Coordinates.GetOrigin();
    vtkm::Vec<vtkm::FloatDefault,3> Spacing = Coordinates.GetSpacing();
    std::cout << "UNIFORM DIMENSIONS " << Dimensions << std::endl;
    std::cout << "UNIFORM ORIGIN " << Origin << std::endl;
    std::cout << "UNIFORM SPACING " << Spacing << std::endl;

    // Sizes and values of output Uniform Structured
    vtkm::Id nx = (bounds.GetPortalConstControl().Get(1) - bounds.GetPortalConstControl().Get(0) + 1);
    vtkm::Id ny = (bounds.GetPortalConstControl().Get(3) - bounds.GetPortalConstControl().Get(2) + 1);
    vtkm::Id nz = (bounds.GetPortalConstControl().Get(5) - bounds.GetPortalConstControl().Get(4) + 1);
    vtkm::Vec<vtkm::FloatDefault,3> OutOrigin = Origin;
    vtkm::Vec<vtkm::FloatDefault,3> OutSpacing = Spacing;
    std::cout << "UNIFORM OUT DIMENSIONS " << vtkm::Id3(nx, ny, nz) << std::endl;
    std::cout << "UNIFORM OUT ORIGIN " << OutOrigin << std::endl;
    std::cout << "UNIFORM OUT SPACING " << OutSpacing << std::endl;

    vtkm::cont::DataSet output;

    // Set the output CoordinateSystem information
    UniformArrayHandle outCoordinateData(vtkm::Id3(nx, ny, nz), OutOrigin, OutSpacing);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    std::cout << "CoordinateSystem for output:" << std::endl;
    outCoordinates.PrintSummary(std::cout);

    // Set the size of the cell set for Uniform
    if (dimension == 1) {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(nx);
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny));
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny, nz));
      output.AddCellSet(outCellSet);
    }

std::cout << "Number of Cells " << output.GetCellSet(0).GetNumberOfCells() << std::endl;
std::cout << "Number of Points " << output.GetCellSet(0).GetNumberOfPoints() << std::endl;

    // Calculate and save the maps of point and cell data to subset
    CreatePointCellMaps(Dimensions, 
                        cellSet.GetNumberOfPoints(),
                        cellSet.GetNumberOfCells(),
                        bounds,
                        sample,
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
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &bounds,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &sample,
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

    vtkm::Id dimx = X.GetNumberOfValues();
    vtkm::Id dimy = Y.GetNumberOfValues();
    vtkm::Id dimz = Z.GetNumberOfValues();
    vtkm::Id3 Dimensions(dimx, dimy, dimz);

    std::cout << "Number of x coordinates " << dimx << std::endl;
    std::cout << "Number of y coordinates " << dimy << std::endl;
    std::cout << "Number of z coordinates " << dimz << std::endl;

    for (vtkm::Id x = 0; x < dimx; x++)
      std::cout << "X " << x << " = " << X.Get(x) << std::endl;
    for (vtkm::Id y = 0; y < dimy; y++)
      std::cout << "Y " << y << " = " << Y.Get(y) << std::endl;
    for (vtkm::Id z = 0; z < dimz; z++)
      std::cout << "Z " << z << " = " << Z.Get(z) << std::endl;

    vtkm::cont::DataSet output;

    // Sizes and values of output Rectilinear Structured
    vtkm::Id nx = (bounds.GetPortalConstControl().Get(1) - bounds.GetPortalConstControl().Get(0)) + 1;
    vtkm::Id ny = (bounds.GetPortalConstControl().Get(3) - bounds.GetPortalConstControl().Get(2)) + 1;
    vtkm::Id nz = (bounds.GetPortalConstControl().Get(5) - bounds.GetPortalConstControl().Get(4)) + 1;
    std::cout << "RECTILINEAR OUT DIMENSIONS " << vtkm::Id3(nx, ny, nz) << std::endl;

    // Set output coordinate system
    DefaultHandle Xc, Yc, Zc;
    Xc.Allocate(nx);
    Yc.Allocate(ny);
    Zc.Allocate(nz);

    vtkm::Id indx = 0;
    for (vtkm::Id x = bounds.GetPortalConstControl().Get(0); x <= bounds.GetPortalConstControl().Get(1); x++)
    {
      Xc.GetPortalControl().Set(indx++, X.Get(x));
    }
    indx = 0;
    for (vtkm::Id y = bounds.GetPortalConstControl().Get(2); y <= bounds.GetPortalConstControl().Get(3) ; y++)
    {
      Yc.GetPortalControl().Set(indx++, Y.Get(y));
    }
    indx = 0;
    for (vtkm::Id z = bounds.GetPortalConstControl().Get(4); z <= bounds.GetPortalConstControl().Get(5); z++)
    {
      Zc.GetPortalControl().Set(indx++, Z.Get(z));
    }

    for (vtkm::Id x = 0; x < nx; x++)
      std::cout << "Xc " << x << " = " << Xc.GetPortalControl().Get(x) << std::endl;
    for (vtkm::Id y = 0; y < ny; y++)
      std::cout << "Yc " << y << " = " << Yc.GetPortalControl().Get(y) << std::endl;
    for (vtkm::Id z = 0; z < nz; z++)
      std::cout << "Zc " << z << " = " << Zc.GetPortalControl().Get(z) << std::endl;

    CartesianArrayHandle outCoordinateData(Xc, Yc, Zc);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    std::cout << "CoordinateSystem for output:" << std::endl;
    outCoordinates.PrintSummary(std::cout);

    // Set the size of the cell set for Rectilinear
    if (dimension == 1) {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(nx);
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny));
      output.AddCellSet(outCellSet);
    }
    else if (dimension == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny, nz));
      output.AddCellSet(outCellSet);
    }

std::cout << "Number of Cells " << output.GetCellSet(0).GetNumberOfCells() << std::endl;
std::cout << "Number of Points " << output.GetCellSet(0).GetNumberOfPoints() << std::endl;

    // Calculate and save the maps of point and cell data to subset
    CreatePointCellMaps(Dimensions, 
                        cellSet.GetNumberOfPoints(),
                        cellSet.GetNumberOfCells(),
                        bounds,
                        sample,
                        DeviceAdapter());

    return output;
  }

  // Run extract structured on uniform or rectilinear, subset and subsample
  template <typename DeviceAdapter>
  vtkm::cont::DataSet Run(const vtkm::cont::DynamicCellSet &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &bounds,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &sample,
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
                            bounds,
                            sample,
                            DeviceAdapter());
    }
    else
    {
      return ExtractRectilinear(dimension,
                                cellSet,
                                coordinates,
                                bounds,
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
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ExtractStructured_h
