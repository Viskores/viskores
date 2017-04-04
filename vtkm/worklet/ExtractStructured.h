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
    typedef typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;

    // Rectilinear Structured
    typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> DefaultHandle;
    typedef vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle,DefaultHandle,DefaultHandle> CartesianArrayHandle;
    typedef typename DefaultHandle::ExecutionTypes<DeviceAdapter>::PortalConst DefaultConstHandle;
    typedef typename CartesianArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst CartesianConstPortal;

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

    // Output data set and empty cell set of the same type
    vtkm::cont::DataSet output;
    CellSetType outCellSet(cellSet.GetName());

    // Data in the Field attached to CoordinateSystem is dynamic
    vtkm::cont::DynamicArrayHandleCoordinateSystem coordinateData = coordinates.GetData();

    if (IsUniformDataSet)
    {
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
      vtkm::Id3 OutDimensions;
      vtkm::Vec<vtkm::FloatDefault,3> OutOrigin = Origin;
      vtkm::Vec<vtkm::FloatDefault,3> OutSpacing = Spacing;
      OutDimensions[0] = (bounds.GetPortalConstControl().Get(1) - bounds.GetPortalConstControl().Get(0));
      OutDimensions[1] = (bounds.GetPortalConstControl().Get(3) - bounds.GetPortalConstControl().Get(2));
      OutDimensions[2] = (bounds.GetPortalConstControl().Get(5) - bounds.GetPortalConstControl().Get(4));
      std::cout << "UNIFORM OUT DIMENSIONS " << OutDimensions << std::endl;
      std::cout << "UNIFORM OUT ORIGIN " << OutOrigin << std::endl;
      std::cout << "UNIFORM OUT SPACING " << OutSpacing << std::endl;

      VTKM_ASSERT((OutDimensions[0]>=1 && OutDimensions[1]>=1 && OutDimensions[2]>=1) ||
                  (OutDimensions[0]>=1 && OutDimensions[1]>=1 && OutDimensions[2]>=1) ||
                  (OutDimensions[0]>=1 && OutDimensions[1]>=1 && OutDimensions[2]>=1));
      VTKM_ASSERT(OutSpacing[0]>0 && OutSpacing[1]>0 && OutSpacing[2]>0);

      // Set the output CoordinateSystem for Uniform
      UniformArrayHandle outCoordinateData(OutDimensions, OutOrigin, OutSpacing);
      vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
      output.AddCoordinateSystem(outCoordinates);

      std::cout << "CoordinateSystem for output:" << std::endl;
      outCoordinates.PrintSummary(std::cout);

      // Set the size of the cell set for Uniform
      outCellSet.SetPointDimensions(OutDimensions);
      output.AddCellSet(outCellSet);

      // At this point I should have a complete Uniform Structured dataset with only geometry
      // Need to calculate the ArrayPermutation for mapping point data
      // Need to calculate the ArrayPermutation for mapping cell data
      // This has to be kept in the worklet so that the calling filter can apply it repeatedly

      // After that need to do subsampling as well as subsetting which changes the geometry
      // and the maps
    }
    else
    {
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

      vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                         vtkm::TopologyElementTagCell,3> Conn;
      Conn = cellSet.PrepareForInput(DeviceAdapter(),
                                     vtkm::TopologyElementTagPoint(),
                                     vtkm::TopologyElementTagCell());
      vtkm::Id3 Dimensions = Conn.GetPointDimensions();
      std::cout << "RECTILINEAR DIMENSIONS " << Dimensions << std::endl;

      for (vtkm::Id x = 0; x < Dimensions[0]; x++)
        std::cout << "X " << x << " = " << X.Get(x) << std::endl;
      for (vtkm::Id y = 0; y < Dimensions[1]; y++)
        std::cout << "Y " << y << " = " << Y.Get(y) << std::endl;
      for (vtkm::Id z = 0; z < Dimensions[2]; z++)
        std::cout << "Z " << z << " = " << Z.Get(z) << std::endl;

      // Sizes and values of output Rectilinear Structured
      vtkm::Id3 OutDimensions;
      OutDimensions[0] = (bounds.GetPortalConstControl().Get(1) - bounds.GetPortalConstControl().Get(0));
      OutDimensions[1] = (bounds.GetPortalConstControl().Get(3) - bounds.GetPortalConstControl().Get(2));
      OutDimensions[2] = (bounds.GetPortalConstControl().Get(5) - bounds.GetPortalConstControl().Get(4));
      std::cout << "RECTILINEAR OUT DIMENSIONS " << OutDimensions << std::endl;

      VTKM_ASSERT((OutDimensions[0]>=1 && OutDimensions[1]>=1 && OutDimensions[2]>=1) ||
                  (OutDimensions[0]>=1 && OutDimensions[1]>=1 && OutDimensions[2]>=1) ||
                  (OutDimensions[0]>=1 && OutDimensions[1]>=1 && OutDimensions[2]>=1));

      // Set output coordinate system
      DefaultHandle Xc, Yc, Zc;
      Xc.Allocate(OutDimensions[0]);
      Yc.Allocate(OutDimensions[1]);
      Zc.Allocate(OutDimensions[2]);

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

      for (vtkm::Id x = 0; x < OutDimensions[0]; x++)
        std::cout << "Xc " << x << " = " << Xc.GetPortalControl().Get(x) << std::endl;
      for (vtkm::Id y = 0; y < OutDimensions[1]; y++)
        std::cout << "Yc " << y << " = " << Yc.GetPortalControl().Get(y) << std::endl;
      for (vtkm::Id z = 0; z < OutDimensions[2]; z++)
        std::cout << "Zc " << z << " = " << Zc.GetPortalControl().Get(z) << std::endl;

      CartesianArrayHandle outCoordinateData(Xc, Yc, Zc);
      vtkm::cont::CoordinateSystem outCoordinates(coordinates.GetName(), outCoordinateData);
      output.AddCoordinateSystem(outCoordinates);

      std::cout << "CoordinateSystem for output:" << std::endl;
      outCoordinates.PrintSummary(std::cout);

      // Set the size of the cell set for Uniform
      outCellSet.SetPointDimensions(OutDimensions);
      output.AddCellSet(outCellSet);
    }

    return output;
  }

  template <typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> Run(const vtkm::cont::CellSetExplicit<>& cellSet,
                                    const DeviceAdapter&)
  {
    throw vtkm::cont::ErrorBadType("CellSetExplicit can't extract grid");
    return cellSet;
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ExtractStructured_h
