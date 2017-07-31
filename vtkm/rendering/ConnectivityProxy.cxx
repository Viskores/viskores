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
#include <cstdlib>
#include <typeinfo>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/ConnectivityProxy.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/raytracing/ConnectivityTracerFactory.h>
#include <vtkm/rendering/raytracing/Logger.h>

namespace vtkm
{
namespace rendering
{
struct ConnectivityProxy::InternalsType
{
protected:
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorMapType;
  typedef vtkm::rendering::raytracing::ConnectivityBase BaseType;

  BaseType* Tracer;
  vtkm::cont::Field ScalarField;
  vtkm::cont::Field EmissionField;
  vtkm::cont::DynamicCellSet Cells;
  vtkm::cont::CoordinateSystem Coords;
  RenderMode Mode;
  vtkm::Bounds SpatialBounds;
  ColorMapType ColorMap;
  vtkm::cont::DataSet Dataset;
  vtkm::Range ScalarRange;

  struct BoundsFunctor
  {
    vtkm::rendering::ConnectivityProxy::InternalsType* Internals;
    const vtkm::cont::CoordinateSystem& Coordinates;

    VTKM_CONT
    BoundsFunctor(vtkm::rendering::ConnectivityProxy::InternalsType* self,
                  const vtkm::cont::CoordinateSystem& coordinates)
      : Internals(self)
      , Coordinates(coordinates)
    {
    }

    template <typename Device>
    VTKM_CONT bool operator()(Device)
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(Device);

      Internals->SpatialBounds = Internals->Coords.GetBounds(Device());
      return true;
    }
  };

public:
  InternalsType(vtkm::cont::DataSet& dataSet)
  {
    Dataset = dataSet;
    Cells = dataSet.GetCellSet();
    Coords = dataSet.GetCoordinateSystem();
    Mode = VOLUME_MODE;

    //
    // Just grab a default scalar field
    //

    this->SetScalarField(Dataset.GetField(0).GetName());

    Tracer = raytracing::ConnectivityTracerFactory::CreateTracer(Cells, Coords);
  }

  ~InternalsType() { delete Tracer; }

  void SetSampleDistance(const vtkm::Float32& distance)
  {
    if (Mode != VOLUME_MODE)
    {
      std::cout << "Volume Tracer Error: must set volume mode before setting sample dist\n";
      return;
    }
    Tracer->SetSampleDistance(distance);
  }

  VTKM_CONT
  void SetRenderMode(RenderMode mode) { Mode = mode; }

  VTKM_CONT
  RenderMode GetRenderMode() { return Mode; }

  VTKM_CONT
  void SetScalarField(const std::string& fieldName)
  {
    ScalarField = Dataset.GetField(fieldName);
    const vtkm::cont::ArrayHandle<vtkm::Range> range = this->ScalarField.GetRange();
    ScalarRange = range.GetPortalConstControl().Get(0);
  }

  VTKM_CONT
  void SetColorMap(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colormap)
  {
    Tracer->SetColorMap(colormap);
  }

  VTKM_CONT
  void SetCompositeBackground(bool on) { Tracer->SetCompositeBackground(on); }

  VTKM_CONT
  void SetEmissionField(const std::string& fieldName)
  {
    if (Mode != ENERGY_MODE)
    {
      std::cout << "Volume Tracer Error: must set energy mode before setting emission field\n";
      return;
    }
    std::cout << "*************** Setting emission " << fieldName << "\n";
    EmissionField = Dataset.GetField(fieldName);
  }

  VTKM_CONT
  vtkm::Bounds GetSpatialBounds() const { return SpatialBounds; }

  VTKM_CONT
  vtkm::Range GetScalarRange() const { return ScalarRange; }

  VTKM_CONT
  void SetScalarRange(const vtkm::Range& range) { ScalarRange = range; }

  VTKM_CONT
  void Trace(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays)
  {

    if (Mode == VOLUME_MODE)
    {
      Tracer->SetVolumeData(this->ScalarField, this->ScalarRange);
    }
    else
    {
      Tracer->SetEnergyData(
        this->ScalarField, rays.Buffers.at(0).GetNumChannels(), this->EmissionField);
    }

    Tracer->Trace(rays);
  }

  VTKM_CONT
  void Trace(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
  {
    if (Mode == VOLUME_MODE)
    {
      Tracer->SetVolumeData(this->ScalarField, this->ScalarRange);
    }
    else
    {
      Tracer->SetEnergyData(
        this->ScalarField, rays.Buffers.at(0).GetNumChannels(), this->EmissionField);
    }
    Tracer->Trace(rays);
  }

  VTKM_CONT
  void Trace(const vtkm::rendering::Camera& camera, vtkm::rendering::CanvasRayTracer* canvas)
  {

    if (canvas == NULL)
    {
      std::cout << "Conn proxy: canvas is NULL\n";
      return;
    }
    vtkm::rendering::raytracing::Camera rayCamera;
    rayCamera.SetParameters(camera, *canvas);
    vtkm::rendering::raytracing::Ray<vtkm::Float32> rays;
    rayCamera.CreateRays(rays, this->Coords);


    if (Mode == VOLUME_MODE)
    {
      Tracer->SetVolumeData(this->ScalarField, this->ScalarRange);
    }
    else
    {
      std::cout << "ENGERY MODE Not implementedd yet\n";
    }

    Tracer->Trace(rays);

    canvas->WriteToCanvas(rays.PixelIdx, rays.Distance, rays.Buffers.at(0).Buffer, camera);
  }
};


VTKM_CONT
ConnectivityProxy::ConnectivityProxy(vtkm::cont::DataSet& dataSet)
  : Internals(new InternalsType(dataSet))
{
}

VTKM_CONT
ConnectivityProxy::ConnectivityProxy(const vtkm::cont::DynamicCellSet& cellset,
                                     const vtkm::cont::CoordinateSystem& coords,
                                     const vtkm::cont::Field& scalarField)
{
  vtkm::cont::DataSet dataset;

  dataset.AddCellSet(cellset);
  dataset.AddCoordinateSystem(coords);
  dataset.AddField(scalarField);

  Internals = std::shared_ptr<InternalsType>(new InternalsType(dataset));
}

VTKM_CONT
ConnectivityProxy::~ConnectivityProxy()
{
}

VTKM_CONT
ConnectivityProxy::ConnectivityProxy()
{
}

VTKM_CONT
void ConnectivityProxy::SetSampleDistance(const vtkm::Float32& distance)
{
  Internals->SetSampleDistance(distance);
}

VTKM_CONT
void ConnectivityProxy::SetRenderMode(RenderMode mode)
{
  Internals->SetRenderMode(mode);
}

VTKM_CONT
void ConnectivityProxy::SetScalarField(const std::string& fieldName)
{
  Internals->SetScalarField(fieldName);
}

VTKM_CONT
void ConnectivityProxy::SetColorMap(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colormap)
{
  Internals->SetColorMap(colormap);
}

VTKM_CONT
void ConnectivityProxy::SetEmissionField(const std::string& fieldName)
{
  Internals->SetEmissionField(fieldName);
}

VTKM_CONT
vtkm::Bounds ConnectivityProxy::GetSpatialBounds()
{
  return Internals->GetSpatialBounds();
}

VTKM_CONT
vtkm::Range ConnectivityProxy::GetScalarRange()
{
  return Internals->GetScalarRange();
}

VTKM_CONT
void ConnectivityProxy::SetCompositeBackground(bool on)
{
  return Internals->SetCompositeBackground(on);
}

VTKM_CONT
void ConnectivityProxy::SetScalarRange(const vtkm::Range& range)
{
  Internals->SetScalarRange(range);
}

VTKM_CONT
void ConnectivityProxy::Trace(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_64");
  if (Internals->GetRenderMode() == VOLUME_MODE)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  Internals->Trace(rays);
  logger->CloseLogEntry(-1.0);
}

VTKM_CONT
void ConnectivityProxy::Trace(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  if (Internals->GetRenderMode() == VOLUME_MODE)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  Internals->Trace(rays);

  logger->CloseLogEntry(-1.0);
}

VTKM_CONT
void ConnectivityProxy::Trace(const vtkm::rendering::Camera& camera,
                              vtkm::rendering::CanvasRayTracer* canvas)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  logger->AddLogData("volume_mode", "true");

  Internals->Trace(camera, canvas);

  logger->CloseLogEntry(-1.0);
}
}
} // namespace vtkm::rendering
