//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/ConnectivityProxy.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/raytracing/ConnectivityTracer.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>


namespace vtkm
{
namespace rendering
{
struct ConnectivityProxy::InternalsType
{
protected:
  using ColorMapType = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using TracerType = vtkm::rendering::raytracing::ConnectivityTracer;

  TracerType Tracer;
  vtkm::cont::Field ScalarField;
  vtkm::cont::Field EmissionField;
  vtkm::cont::DynamicCellSet Cells;
  vtkm::cont::CoordinateSystem Coords;
  RenderMode Mode;
  vtkm::Bounds SpatialBounds;
  ColorMapType ColorMap;
  vtkm::cont::DataSet Dataset;
  vtkm::Range ScalarRange;
  bool CompositeBackground;

public:
  InternalsType(vtkm::cont::DataSet& dataSet)
  {
    Dataset = dataSet;
    Cells = dataSet.GetCellSet();
    Coords = dataSet.GetCoordinateSystem();
    Mode = VOLUME_MODE;
    CompositeBackground = true;
    //
    // Just grab a default scalar field
    //

    if (Dataset.GetNumberOfFields() > 0)
    {
      this->SetScalarField(Dataset.GetField(0).GetName());
    }
  }

  ~InternalsType() {}

  VTKM_CONT
  void SetUnitScalar(vtkm::Float32 unitScalar) { Tracer.SetUnitScalar(unitScalar); }

  void SetSampleDistance(const vtkm::Float32& distance)
  {
    if (Mode != VOLUME_MODE)
    {
      std::cout << "Volume Tracer Error: must set volume mode before setting sample dist\n";
      return;
    }
    Tracer.SetSampleDistance(distance);
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
  void SetColorMap(vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colormap)
  {
    Tracer.SetColorMap(colormap);
  }

  VTKM_CONT
  void SetCompositeBackground(bool on) { CompositeBackground = on; }

  VTKM_CONT
  void SetDebugPrints(bool on) { Tracer.SetDebugOn(on); }

  VTKM_CONT
  void SetEmissionField(const std::string& fieldName)
  {
    if (Mode != ENERGY_MODE)
    {
      std::cout << "Volume Tracer Error: must set energy mode before setting emission field\n";
      return;
    }
    EmissionField = Dataset.GetField(fieldName);
  }

  VTKM_CONT
  vtkm::Bounds GetSpatialBounds() const { return SpatialBounds; }

  VTKM_CONT
  vtkm::Range GetScalarFieldRange()
  {
    const vtkm::cont::ArrayHandle<vtkm::Range> range = this->ScalarField.GetRange();
    ScalarRange = range.GetPortalConstControl().Get(0);
    return ScalarRange;
  }

  VTKM_CONT
  void SetScalarRange(const vtkm::Range& range) { ScalarRange = range; }

  VTKM_CONT
  void Trace(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays)
  {

    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    Tracer.FullTrace(rays);
  }

  VTKM_CONT
  void Trace(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
  {
    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    Tracer.FullTrace(rays);
  }

  VTKM_CONT
  PartialVector64 PartialTrace(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays)
  {

    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    return Tracer.PartialTrace(rays);
  }

  VTKM_CONT
  PartialVector32 PartialTrace(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
  {
    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    return Tracer.PartialTrace(rays);
  }

  VTKM_CONT
  void Trace(const vtkm::rendering::Camera& camera, vtkm::rendering::CanvasRayTracer* canvas)
  {

    if (canvas == nullptr)
    {
      std::cout << "Conn proxy: canvas is NULL\n";
      throw vtkm::cont::ErrorBadValue("Conn Proxy: null canvas");
    }
    vtkm::rendering::raytracing::Camera rayCamera;
    rayCamera.SetParameters(camera, *canvas);
    vtkm::rendering::raytracing::Ray<vtkm::Float32> rays;
    rayCamera.CreateRays(rays, this->Coords.GetBounds());
    rays.Buffers.at(0).InitConst(0.f);
    raytracing::RayOperations::MapCanvasToRays(rays, camera, *canvas);

    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      throw vtkm::cont::ErrorBadValue("ENERGY MODE Not implemented for this use case\n");
    }

    Tracer.FullTrace(rays);

    canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);
    if (CompositeBackground)
    {
      canvas->BlendBackground();
    }
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

  dataset.SetCellSet(cellset);
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
void ConnectivityProxy::SetColorMap(vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colormap)
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
vtkm::Range ConnectivityProxy::GetScalarFieldRange()
{
  return Internals->GetScalarFieldRange();
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
PartialVector32 ConnectivityProxy::PartialTrace(
  vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
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

  PartialVector32 res = Internals->PartialTrace(rays);

  logger->CloseLogEntry(-1.0);
  return res;
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
PartialVector64 ConnectivityProxy::PartialTrace(
  vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays)
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

  PartialVector64 res = Internals->PartialTrace(rays);

  logger->CloseLogEntry(-1.0);
  return res;
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

VTKM_CONT
void ConnectivityProxy::SetDebugPrints(bool on)
{
  Internals->SetDebugPrints(on);
}

VTKM_CONT
void ConnectivityProxy::SetUnitScalar(vtkm::Float32 unitScalar)
{
  Internals->SetUnitScalar(unitScalar);
}
}
} // namespace vtkm::rendering
