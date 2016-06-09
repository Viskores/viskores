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
#ifndef vtk_m_rendering_MapperRayTracer_h
#define vtk_m_rendering_MapperRayTracer_h
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/internal/DeviceAdapterTagSerial.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/Triangulator.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/Camera.h>
namespace vtkm {
namespace rendering {

//  static bool doOnce = true;
template<typename DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class MapperRayTracer : public Mapper
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorMap;
  vtkm::rendering::raytracing::RayTracer<DeviceAdapter> Tracer;
  CanvasRayTracer *Canvas;
public:
  VTKM_CONT_EXPORT
  MapperRayTracer()
  {
    this->Canvas = NULL;
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
          "Ray Tracer: bad canvas type. Must be CanvasRayTracer");
      }
    }
  }
  VTKM_CONT_EXPORT
  void SetActiveColorTable(const ColorTable &colorTable)
  {
    colorTable.Sample(1024, ColorMap);
  }

  VTKM_CONT_EXPORT
  void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                   const vtkm::cont::CoordinateSystem &coords,
                   const vtkm::cont::Field &scalarField,
                   const vtkm::rendering::ColorTable &vtkmNotUsed(colorTable),
                   const vtkm::rendering::Camera &camera,
                   const vtkm::Range &scalarRange)
  {

    const vtkm::cont::DynamicArrayHandleCoordinateSystem dynamicCoordsHandle = coords.GetData();
    vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> >  indices;
    this->Tracer.GetCamera().SetParameters(camera, *this->Canvas);
    vtkm::Id numberOfTriangles;

    vtkm::Bounds dataBounds = coords.GetBounds(DeviceAdapter());

    Triangulator<DeviceAdapter> triangulator;
    triangulator.run(cellset, indices, numberOfTriangles);//,dynamicCoordsHandle,dataBounds);

    this->Tracer.SetData(dynamicCoordsHandle,
                         indices,
                         scalarField,
                         numberOfTriangles,
                         scalarRange,
                         dataBounds);
    this->Tracer.SetColorMap(this->ColorMap);
    this->Tracer.SetBackgroundColor(this->BackgroundColor);
    this->Tracer.Render(this->Canvas);
  }
};
}} //namespace vtkm::rendering
#endif //vtk_m_rendering_MapperRayTracer_h
