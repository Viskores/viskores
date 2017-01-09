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

#include <vtkm/rendering/MapperVolume.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/RuntimeDeviceTracker.h>
#include <vtkm/cont/internal/SimplePolymorphicContainer.h>

#include <vtkm/rendering/CanvasRayTracer.h>

#include <vtkm/rendering/internal/RunTriangulator.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/VolumeRendererStructured.h>

#include <typeinfo>

namespace vtkm {
namespace rendering {

struct MapperVolume::InternalsType
{
  vtkm::rendering::CanvasRayTracer *Canvas;
  vtkm::cont::internal::RuntimeDeviceTracker DeviceTracker;
  std::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase>
      RayTracerContainer;

  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
  {  }

  template<typename Device>
  VTKM_CONT
  vtkm::rendering::raytracing::VolumeRendererStructured<Device> *
  GetRayTracer(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    typedef vtkm::rendering::raytracing::VolumeRendererStructured<Device>
        RayTracerType;
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

MapperVolume::MapperVolume()
  : Internals(new InternalsType)
{  }

MapperVolume::~MapperVolume()
{  }

void MapperVolume::SetCanvas(vtkm::rendering::Canvas *canvas)
{
  if(canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if(this->Internals->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorBadValue(
        "Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

vtkm::rendering::Canvas *
MapperVolume::GetCanvas() const
{
  return this->Internals->Canvas;
}

struct MapperVolume::RenderFunctor
{
  vtkm::rendering::MapperVolume *Self;
  vtkm::cont::CellSetStructured<3> CellSet;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::Field ScalarField;
  vtkm::rendering::Camera Camera;
  vtkm::Range ScalarRange;

  VTKM_CONT
  RenderFunctor(vtkm::rendering::MapperVolume *self,
                const vtkm::cont::CellSetStructured<3> cellSet,
                const vtkm::cont::CoordinateSystem &coordinates,
                const vtkm::cont::Field &scalarField,
                const vtkm::rendering::Camera &camera,
                const vtkm::Range &scalarRange)
    : Self(self),
      CellSet(cellSet),
      Coordinates(coordinates),
      ScalarField(scalarField),
      Camera(camera),
      ScalarRange(scalarRange)
  {  }

  template<typename Device>
  bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::rendering::raytracing::VolumeRendererStructured<Device> *tracer =
        this->Self->Internals->GetRayTracer(Device());

    tracer->GetCamera().SetParameters(this->Camera,
                                      *this->Self->Internals->Canvas);

    vtkm::Bounds dataBounds = this->Coordinates.GetBounds(Device());

    tracer->SetData(this->Coordinates,
                    this->ScalarField,
                    dataBounds,
                    this->CellSet,
                    this->ScalarRange);
    tracer->SetColorMap(this->Self->ColorMap);
    tracer->SetBackgroundColor(
          this->Self->Internals->Canvas->GetBackgroundColor().Components);
    tracer->Render(this->Self->Internals->Canvas);

    return true;
  }
};

void MapperVolume::RenderCells(
    const vtkm::cont::DynamicCellSet &cellset,
    const vtkm::cont::CoordinateSystem &coords,
    const vtkm::cont::Field &scalarField,
    const vtkm::rendering::ColorTable &vtkmNotUsed(colorTable),
    const vtkm::rendering::Camera &camera,
    const vtkm::Range &scalarRange)
{
  if(!cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    std::cerr<<"ERROR cell set type not currently supported\n";
    std::string theType = typeid(cellset).name();
    std::cerr<<"Type : "<<theType<<std::endl;
  }
  else
  {
    RenderFunctor functor(this,
                          cellset.Cast<vtkm::cont::CellSetStructured<3> >(),
                          coords,
                          scalarField,
                          camera,
                          scalarRange);
    vtkm::cont::TryExecute(functor,
                           this->Internals->DeviceTracker,
                           VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
  }
}

void MapperVolume::StartScene()
{
  // Nothing needs to be done.
}

void MapperVolume::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper *MapperVolume::NewCopy() const
{
  return new vtkm::rendering::MapperVolume(*this);
}

}
} // namespace vtkm::rendering
