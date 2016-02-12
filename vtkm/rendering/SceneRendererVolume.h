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
#ifndef vtk_m_rendering_SceneRendererVolume_h
#define vtk_m_rendering_SceneRendererVolume_h
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/Triangulator.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/raytracing/VolumeRendererUniform.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/View.h>

#include <typeinfo>

namespace vtkm {
namespace rendering {
template<typename DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class SceneRendererVolume : public SceneRenderer
{ 
protected:
  vtkm::rendering::raytracing::VolumeRendererUniform<DeviceAdapter>  Tracer;
public:
  VTKM_CONT_EXPORT
  SceneRendererVolume()
  {}

  VTKM_CONT_EXPORT
  void SetNumberOfSamples(const vtkm::Int32 &numSamples)
  {
    Tracer.SetNumberOfSamples(numSamples);
  }

  VTKM_CONT_EXPORT
  virtual void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                           const vtkm::cont::CoordinateSystem &coords,
                           vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &, //colorTable
                           vtkm::Float64 *scalarBounds=NULL) //scalarBounds=NULL)
  {
    vtkm::cont::DynamicArrayHandleCoordinateSystem dynamicCoordsHandle = coords.GetData();
    vtkm::Float64 coordsBounds[6]; // Xmin,Xmax,Ymin..
    coords.GetBounds(coordsBounds,DeviceAdapter());

    bool isExplicit = false;
    bool isUniform = false;
    if(!cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      std::cerr<<"ERROR cell set type not currently supported\n";
      std::string theType = typeid(cellset).name();
      std::cerr<<"Type : "<<theType<<std::endl; 
    }
    else
    {
      vtkm::cont::CellSetStructured<3> cellSetStructured3D = cellset.Cast<vtkm::cont::CellSetStructured<3> >();
      std::cout<<"Is structured"<<std::endl;
      vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
      //if(dynamicCoordsHandle.IsArrayHandleType(vtkm::cont::ArrayHandleUniformPointCoordinates()))
      vertices = dynamicCoordsHandle.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      vtkm::rendering::raytracing::Camera<DeviceAdapter> &camera = Tracer.GetCamera();
      camera.SetParameters(View);
      Tracer.SetData(vertices, scalarField, coordsBounds, cellSetStructured3D, scalarBounds);
      Tracer.SetColorMap(ColorMap);
      std::cout<<"Structured Rendering"<<std::endl;
      Tracer.SetBackgroundColor(this->BackgroundColor);
      Tracer.Render();
    }
    
  }
};
}} //namespace vtkm::rendering
#endif //vtk_m_rendering_SceneRendererVolume_h
