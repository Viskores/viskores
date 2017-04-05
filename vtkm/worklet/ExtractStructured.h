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

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterUniform.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm {
namespace worklet {

/// \brief Extract subset of structured grid and/or resample
class ExtractStructured
{
public:
  ExtractStructured() {}

  // Uniform Structured
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::DataSet ExtractUniform(
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

    // Must know whether 1D, 2D, 3D
    vtkm::IdComponent Dimension = 3;
    if (Dimensions[2] == 1)
    {
      Dimension = 2;
      if (Dimensions[1] == 1)
      {
        Dimension = 1;
      }
    }
    std::cout << "DIMENSION " << Dimension << std::endl;

    // Sizes and values of output Uniform Structured
    vtkm::Id nx = (bounds.GetPortalConstControl().Get(1) - bounds.GetPortalConstControl().Get(0));
    vtkm::Id ny = (bounds.GetPortalConstControl().Get(3) - bounds.GetPortalConstControl().Get(2));
    vtkm::Id nz = (bounds.GetPortalConstControl().Get(5) - bounds.GetPortalConstControl().Get(4));
    vtkm::Vec<vtkm::FloatDefault,3> OutOrigin = Origin;
    vtkm::Vec<vtkm::FloatDefault,3> OutSpacing = Spacing;
    std::cout << "UNIFORM OUT DIMENSIONS " << vtkm::Id3(nx, ny, nz) << std::endl;
    std::cout << "UNIFORM OUT ORIGIN " << OutOrigin << std::endl;
    std::cout << "UNIFORM OUT SPACING " << OutSpacing << std::endl;

/*
    VTKM_ASSERT((Dimension == 1 && nx>1 && ny==1 && nz==1) ||
                (Dimension == 2 && nx>1 && ny>1 && nz==1) ||
                (Dimension == 3 && nx>1 && ny>1 && nz>1));
    VTKM_ASSERT(OutSpacing[0]>0 && OutSpacing[1]>0 && OutSpacing[2]>0);
*/

    vtkm::cont::DataSet output;

    // Set the output CoordinateSystem for Uniform
    UniformArrayHandle outCoordinateData(vtkm::Id3(nx, ny, nz), OutOrigin, OutSpacing);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    std::cout << "CoordinateSystem for output:" << std::endl;
    outCoordinates.PrintSummary(std::cout);

    // Set the size of the cell set for Uniform
    if (Dimension == 1) {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(nx);
      output.AddCellSet(outCellSet);
    }
    else if (Dimension == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny));
      output.AddCellSet(outCellSet);
    }
    else if (Dimension == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny, nz));
      output.AddCellSet(outCellSet);
    }
    else
      throw vtkm::cont::ErrorBadValue("Invalid cell set dimension");

    // At this point I should have a complete Uniform Structured dataset with only geometry
    // Need to calculate the ArrayPermutation for mapping point data
    // Need to calculate the ArrayPermutation for mapping cell data
    // This has to be kept in the worklet so that the calling filter can apply it repeatedly

    // After that need to do subsampling as well as subsetting which changes the geometry
    // and the maps

    return output;
  }

  // Rectilinear Structured
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::DataSet ExtractRectilinear(
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

    std::cout << "Number of x coordinates " << dimx << std::endl;
    std::cout << "Number of y coordinates " << dimy << std::endl;
    std::cout << "Number of z coordinates " << dimz << std::endl;

    // Must know whether 1D, 2D, 3D
    vtkm::IdComponent Dimension = 3;
    if (dimz == 1)
    {
      Dimension = 2;
      if (dimy == 1)
      {
        Dimension = 1;
      }
    }
    std::cout << "DIMENSION " << Dimension << std::endl;

    for (vtkm::Id x = 0; x < dimx; x++)
      std::cout << "X " << x << " = " << X.Get(x) << std::endl;
    for (vtkm::Id y = 0; y < dimy; y++)
      std::cout << "Y " << y << " = " << Y.Get(y) << std::endl;
    for (vtkm::Id z = 0; z < dimz; z++)
      std::cout << "Z " << z << " = " << Z.Get(z) << std::endl;

    vtkm::cont::DataSet output;

    // Sizes and values of output Rectilinear Structured
    vtkm::Id nx = (bounds.GetPortalConstControl().Get(1) - bounds.GetPortalConstControl().Get(0));
    vtkm::Id ny = (bounds.GetPortalConstControl().Get(3) - bounds.GetPortalConstControl().Get(2));
    vtkm::Id nz = (bounds.GetPortalConstControl().Get(5) - bounds.GetPortalConstControl().Get(4));
    std::cout << "RECTILINEAR OUT DIMENSIONS " << vtkm::Id3(nx, ny, nz) << std::endl;

    VTKM_ASSERT((Dimension == 1 && nx>1 && ny==1 && nz==1) ||
                (Dimension == 2 && nx>1 && ny>1 && nz==1) ||
                (Dimension == 3 && nx>1 && ny>1 && nz>1));

    // Set output coordinate system
    DefaultHandle Xc, Yc, Zc;
    Xc.Allocate(nx);
    Yc.Allocate(ny);
    Zc.Allocate(nz);

    vtkm::Id indx = 0;
    for (vtkm::Id x = bounds.GetPortalConstControl().Get(0); x <= bounds.GetPortalConstControl().Get(1); x++)
    {
      std::cout << "Copy x from " << x << " to " << indx << std::endl;
      Xc.GetPortalControl().Set(indx++, X.Get(x));
    }
    indx = 0;
    for (vtkm::Id y = bounds.GetPortalConstControl().Get(2); y <= bounds.GetPortalConstControl().Get(3) ; y++)
    {
      std::cout << "Copy y from " << y << " to " << indx << std::endl;
      Yc.GetPortalControl().Set(indx++, Y.Get(y));
    }
    indx = 0;
    for (vtkm::Id z = bounds.GetPortalConstControl().Get(4); z <= bounds.GetPortalConstControl().Get(5); z++)
    {
      std::cout << "Copy z from " << z << " to " << indx << std::endl;
      Zc.GetPortalControl().Set(indx++, Z.Get(z));
    }

/*
    for (vtkm::Id x = 0; x < nx; x++)
      std::cout << "Xc " << x << " = " << Xc.GetPortalControl().Get(x) << std::endl;
    for (vtkm::Id y = 0; y < ny; y++)
      std::cout << "Yc " << y << " = " << Yc.GetPortalControl().Get(y) << std::endl;
    for (vtkm::Id z = 0; z < nz; z++)
      std::cout << "Zc " << z << " = " << Zc.GetPortalControl().Get(z) << std::endl;
*/

    CartesianArrayHandle outCoordinateData(Xc, Yc, Zc);
    vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
    output.AddCoordinateSystem(outCoordinates);

    std::cout << "CoordinateSystem for output:" << std::endl;
    outCoordinates.PrintSummary(std::cout);

    // Set the size of the cell set for Rectilinear
    if (Dimension == 1) {
      vtkm::cont::CellSetStructured<1> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(nx);
      output.AddCellSet(outCellSet);
    }
    else if (Dimension == 2)
    {
      vtkm::cont::CellSetStructured<2> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny));
      output.AddCellSet(outCellSet);
    }
    else if (Dimension == 3)
    {
      vtkm::cont::CellSetStructured<3> outCellSet(cellSet.GetName());
      outCellSet.SetPointDimensions(vtkm::make_Vec(nx, ny, nz));
      output.AddCellSet(outCellSet);
    }
    else
      throw vtkm::cont::ErrorBadValue("Invalid cell set dimension");
 
    return output;
  }

  // Run extract structured on uniform or rectilinear, subset and subsample
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::DataSet Run(const CellSetType &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &bounds,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &sample,
                          DeviceAdapter)
  {
    // Uniform Structured
    typedef vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
    bool IsUniformDataSet;
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
      return ExtractUniform(cellSet,
                            coordinates,
                            bounds,
                            sample,
                            DeviceAdapter());
    }
    else
    {
      return ExtractRectilinear(cellSet,
                                coordinates,
                                bounds,
                                sample,
                                DeviceAdapter());
    }
  }

  template <typename DeviceAdapter>
  vtkm::cont::DataSet Run(const vtkm::cont::CellSetExplicit<>& cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &bounds,
                          const vtkm::cont::ArrayHandle<vtkm::IdComponent> &sample,
                          DeviceAdapter)
  {
    throw vtkm::cont::ErrorBadType("CellSetExplicit can't extract grid");
    return vtkm::cont::DataSet();
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ExtractStructured_h
