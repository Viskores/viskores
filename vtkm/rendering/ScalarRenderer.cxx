//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/ScalarRenderer.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/ScalarRenderer.h>
#include <vtkm/rendering/raytracing/SphereExtractor.h>
#include <vtkm/rendering/raytracing/SphereIntersector.h>
#include <vtkm/rendering/raytracing/TriangleExtractor.h>

namespace vtkm
{
namespace rendering
{

struct ScalarRenderer::InternalsType
{
  bool ValidDataSet = false;
  vtkm::Int32 Width = 1024;
  vtkm::Int32 Height = 1024;
  vtkm::Float32 DefaultValue = vtkm::Nan32();
  vtkm::cont::DataSet DataSet;
  vtkm::rendering::raytracing::ScalarRenderer Tracer;
  vtkm::Bounds ShapeBounds;
};

ScalarRenderer::ScalarRenderer()
  : Internals(std::make_unique<InternalsType>())
{
}

ScalarRenderer::ScalarRenderer(ScalarRenderer&&) noexcept = default;
ScalarRenderer& ScalarRenderer::operator=(ScalarRenderer&&) noexcept = default;
ScalarRenderer::~ScalarRenderer() = default;

void ScalarRenderer::SetWidth(vtkm::Int32 width)
{
  if (width < 1)
  {
    throw vtkm::cont::ErrorBadValue("ScalarRenderer: width must be greater than 0");
  }
  Internals->Width = width;
}

void ScalarRenderer::SetDefaultValue(vtkm::Float32 value)
{
  Internals->DefaultValue = value;
}

void ScalarRenderer::SetHeight(vtkm::Int32 height)
{
  if (height < 1)
  {
    throw vtkm::cont::ErrorBadValue("ScalarRenderer: height must be greater than 0");
  }
  Internals->Height = height;
}

void ScalarRenderer::SetInput(vtkm::cont::DataSet& dataSet)
{
  this->Internals->DataSet = dataSet;
  this->Internals->ValidDataSet = true;

  raytracing::TriangleExtractor triExtractor;
  vtkm::cont::UnknownCellSet cellSet = this->Internals->DataSet.GetCellSet();
  vtkm::cont::CoordinateSystem coords = this->Internals->DataSet.GetCoordinateSystem();
  triExtractor.ExtractCells(cellSet);

  if (triExtractor.GetNumberOfTriangles() > 0)
  {
    auto triIntersector = std::make_unique<raytracing::TriangleIntersector>();
    triIntersector->SetData(coords, triExtractor.GetTriangles());
    this->Internals->ShapeBounds = triIntersector->GetShapeBounds();
    this->Internals->Tracer.SetShapeIntersector(std::move(triIntersector));
  }
}

ScalarRenderer::Result ScalarRenderer::Render(const vtkm::rendering::Camera& camera)
{
  if (!Internals->ValidDataSet)
  {
    throw vtkm::cont::ErrorBadValue("ScalarRenderer: input never set");
  }

  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("scalar_render");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;
  timer.Start();

  // Create rays
  vtkm::rendering::raytracing::Camera cam;
  cam.SetParameters(camera, this->Internals->Width, this->Internals->Height);

  // FIXME: rays are created with an unused Buffers.at(0), that ChannelBuffer
  //  also has wrong number of channels, thus allocates memory that is wasted.
  vtkm::rendering::raytracing::Ray<vtkm::Float32> rays;
  cam.CreateRays(rays, this->Internals->ShapeBounds);
  rays.Buffers.at(0).InitConst(0.f);

  // add fields
  const vtkm::Id numFields = this->Internals->DataSet.GetNumberOfFields();
  std::map<std::string, vtkm::Range> rangeMap;
  for (vtkm::Id i = 0; i < numFields; ++i)
  {
    const auto& field = this->Internals->DataSet.GetField(i);
    if (field.GetData().GetNumberOfComponents() == 1)
    {
      auto ranges = field.GetRange();
      rangeMap[field.GetName()] = ranges.ReadPortal().Get(0);
      this->Internals->Tracer.AddField(field);
    }
  }

  this->Internals->Tracer.Render(rays, Internals->DefaultValue, cam);

  using ArrayF32 = vtkm::cont::ArrayHandle<vtkm::Float32>;
  std::vector<ArrayF32> res;
  std::vector<std::string> names;
  const size_t numBuffers = rays.Buffers.size();
  vtkm::Id expandSize = Internals->Width * Internals->Height;

  for (size_t i = 0; i < numBuffers; ++i)
  {
    const std::string name = rays.Buffers[i].GetName();
    if (name == "default")
      continue;
    raytracing::ChannelBuffer<vtkm::Float32> buffer = rays.Buffers[i];
    raytracing::ChannelBuffer<vtkm::Float32> expanded =
      buffer.ExpandBuffer(rays.PixelIdx, expandSize, Internals->DefaultValue);
    res.push_back(expanded.Buffer);
    names.push_back(name);
  }

  raytracing::ChannelBuffer<vtkm::Float32> depthChannel(1, rays.NumRays);
  depthChannel.Buffer = rays.Distance;
  raytracing::ChannelBuffer<vtkm::Float32> depthExpanded =
    depthChannel.ExpandBuffer(rays.PixelIdx, expandSize, Internals->DefaultValue);

  Result result;
  result.Width = Internals->Width;
  result.Height = Internals->Height;
  result.Scalars = res;
  result.ScalarNames = names;
  result.Ranges = rangeMap;
  result.Depths = depthExpanded.Buffer;

  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);

  return result;
}

vtkm::cont::DataSet ScalarRenderer::Result::ToDataSet()
{
  if (Scalars.empty())
  {
    throw vtkm::cont::ErrorBadValue("ScalarRenderer: result empty");
  }

  VTKM_ASSERT(Width > 0);
  VTKM_ASSERT(Height > 0);

  vtkm::cont::DataSet result;
  vtkm::Vec<vtkm::Float32, 3> origin(0.f, 0.f, 0.f);
  vtkm::Vec<vtkm::Float32, 3> spacing(1.f, 1.f, 1.f);
  vtkm::Id3 dims(Width + 1, Height + 1, 1);
  result.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", dims, origin, spacing));
  vtkm::Id2 dims2(dims[0], dims[1]);
  vtkm::cont::CellSetStructured<2> resCellSet;
  resCellSet.SetPointDimensions(dims2);
  result.SetCellSet(resCellSet);

  const size_t fieldSize = Scalars.size();
  for (size_t i = 0; i < fieldSize; ++i)
  {
    result.AddField(
      vtkm::cont::Field(ScalarNames[i], vtkm::cont::Field::Association::Cells, Scalars[i]));
  }

  result.AddField(vtkm::cont::Field("depth", vtkm::cont::Field::Association::Cells, Depths));

  return result;
}
}
} // vtkm::rendering
