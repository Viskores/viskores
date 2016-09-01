//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/MapperRayTracer.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/RuntimeDeviceTracker.h>
#include <vtkm/cont/internal/SimplePolymorphicContainer.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/Camera.h>

namespace vtkm {
namespace rendering {

struct MapperRayTracer::InternalsType
{
  vtkm::rendering::CanvasRayTracer *Canvas;
  vtkm::cont::internal::RuntimeDeviceTracker DeviceTracker;
  boost::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase>
      RayTracerContainer;

  VTKM_CONT_EXPORT
  InternalsType()
    : Canvas(nullptr)
  {  }

  template<typename Device>
  VTKM_CONT_EXPORT
  vtkm::rendering::raytracing::RayTracer<Device> *GetRayTracer(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    typedef vtkm::rendering::raytracing::RayTracer<Device> RayTracerType;
    typedef vtkm::cont::internal::SimplePolymorphicContainer<RayTracerType>
        ContainerType;
    RayTracerType *tracer = NULL;
    if (this->RayTracerContainer)
    {
      ContainerType *container =
          dynamic_cast<ContainerType *>(this->RayTracerContainer.get());
      if (container)
      {
        tracer = &container->Item;
      }
    }

    if (tracer == NULL)
    {
      ContainerType *container
          = new vtkm::cont::internal::SimplePolymorphicContainer<RayTracerType>;
      tracer = &container->Item;
      this->RayTracerContainer.reset(container);
    }

    return tracer;
  }
};

MapperRayTracer::MapperRayTracer()
  : Internals(new InternalsType)
{  }

void MapperRayTracer::SetCanvas(vtkm::rendering::Canvas *canvas)
{
  if(canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if(this->Internals->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorControlBadValue(
        "Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

struct MapperRayTracer::RenderFunctor
{
  vtkm::rendering::MapperRayTracer *Self;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4> > TriangleIndices;
  vtkm::Id NumberOfTriangles;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::Field ScalarField;
  vtkm::rendering::Camera Camera;
  vtkm::Range ScalarRange;

  VTKM_CONT_EXPORT
  RenderFunctor(vtkm::rendering::MapperRayTracer *self,
                const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id,4> > &indices,
                vtkm::Id numberOfTriangles,
                const vtkm::cont::CoordinateSystem &coordinates,
                const vtkm::cont::Field &scalarField,
                const vtkm::rendering::Camera &camera,
                const vtkm::Range &scalarRange)
    : Self(self),
      TriangleIndices(indices),
      NumberOfTriangles(numberOfTriangles),
      Coordinates(coordinates),
      ScalarField(scalarField),
      Camera(camera),
      ScalarRange(scalarRange)
  {  }

  template<typename Device>
  bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::rendering::raytracing::RayTracer<Device> *tracer =
        this->Self->Internals->GetRayTracer(Device());

    tracer->GetCamera().SetParameters(this->Camera,
                                      *this->Self->Internals->Canvas);

    vtkm::Bounds dataBounds = this->Coordinates.GetBounds(Device());

    tracer->SetData(this->Coordinates.GetData(),
                    this->TriangleIndices,
                    this->ScalarField,
                    this->NumberOfTriangles,
                    this->ScalarRange,
                    dataBounds);
    tracer->SetColorMap(this->Self->ColorMap);
    tracer->SetBackgroundColor(
          this->Self->Internals->Canvas->GetBackgroundColor().Components);
    tracer->Render(this->Self->Internals->Canvas);

    return true;
  }
};

void MapperRayTracer::RenderCells(
    const vtkm::cont::DynamicCellSet &cellset,
    const vtkm::cont::CoordinateSystem &coords,
    const vtkm::cont::Field &scalarField,
    const vtkm::rendering::ColorTable &vtkmNotUsed(colorTable),
    const vtkm::rendering::Camera &camera,
    const vtkm::Range &scalarRange)
{
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> >  indices;
  vtkm::Id numberOfTriangles;
  vtkm::rendering::internal::RunTriangulator(
        cellset, indices, numberOfTriangles, this->Internals->DeviceTracker);

  RenderFunctor functor(this,
                        indices,
                        numberOfTriangles,
                        coords,
                        scalarField,
                        camera,
                        scalarRange);
  vtkm::cont::TryExecute(functor,
                         this->Internals->DeviceTracker,
                         VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
}

void MapperRayTracer::StartScene()
{
  // Nothing needs to be done.
}

void MapperRayTracer::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper *MapperRayTracer::NewCopy() const
{
  return new vtkm::rendering::MapperRayTracer(*this);
}


}
} // vtkm::rendering
