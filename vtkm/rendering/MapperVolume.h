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
#ifndef vtk_m_rendering_MapperVolume_h
#define vtk_m_rendering_MapperVolume_h

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/Triangulator.h>
#include <vtkm/rendering/raytracing/VolumeRendererStructured.h>
#include <vtkm/rendering/raytracing/Camera.h>

#include <typeinfo>

namespace vtkm {
namespace rendering {
template<typename DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class MapperVolume : public Mapper
{
protected:
  vtkm::rendering::raytracing::VolumeRendererStructured<DeviceAdapter>  Tracer;
  CanvasRayTracer *Canvas;
public:
  VTKM_CONT_EXPORT
  MapperVolume()
  {
    this->Canvas = NULL;
  }

  VTKM_CONT_EXPORT
  void SetNumberOfSamples(const vtkm::Int32 &numSamples)
  {
    Tracer.SetNumberOfSamples(numSamples);
  }

  VTKM_CONT_EXPORT
  void SetCanvas(vtkm::rendering::Canvas *canvas)
  {
    if(canvas != NULL)
    {

      this->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
      if(this->Canvas == NULL)
      {
        throw vtkm::cont::ErrorControlBadValue(
          "Volume Render: bad canvas type. Must be CanvasRayTracer");
      }
    }
  }

  VTKM_CONT_EXPORT
  virtual void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                           const vtkm::cont::CoordinateSystem &coords,
                           const vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &, //colorTable
                           const vtkm::rendering::Camera &camera,
                           const vtkm::Range &scalarRange)
  {
//    vtkm::cont::DynamicArrayHandleCoordinateSystem dynamicCoordsHandle = coords.GetData();
    vtkm::Bounds coordsBounds = coords.GetBounds(DeviceAdapter());

    if(!cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      std::cerr<<"ERROR cell set type not currently supported\n";
      std::string theType = typeid(cellset).name();
      std::cerr<<"Type : "<<theType<<std::endl;
    }
    else
    {
      vtkm::cont::CellSetStructured<3> cellSetStructured3D = cellset.Cast<vtkm::cont::CellSetStructured<3> >();
      //vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
      //vertices = dynamicCoordsHandle.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      this->Tracer.GetCamera().SetParameters(camera);
      this->Tracer.SetData(coords,
                           scalarField,
                           coordsBounds,
                           cellSetStructured3D,
                           scalarRange);
      this->Tracer.SetColorMap(this->ColorMap);
      this->Tracer.SetBackgroundColor(this->BackgroundColor);
      this->Tracer.Render(this->Canvas);
    }

  }
};
}} //namespace vtkm::rendering
#endif //vtk_m_rendering_MapperVolume_h
