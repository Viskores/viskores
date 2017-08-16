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

#include <vtkm/Assert.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/exec/CellEdge.h>
#include <vtkm/rendering/CanvasLineRenderer.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperLineRenderer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/WireframeRenderer.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace rendering
{
namespace
{

struct EdgesCounter : public vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(CellSetIn cellSet, FieldOutCell<> numEdges);
  typedef _2 ExecutionSignature(CellShape shape, PointCount numPoints);
  using InputDomain = _1;

  template <typename CellShapeTag>
  VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape, vtkm::IdComponent numPoints) const
  {
    return vtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, *this);
  }
}; // struct EdgesCounter

struct EdgesExtracter : public vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(CellSetIn cellSet, FieldOutCell<> edgeIndices);
  typedef void ExecutionSignature(CellShape, PointIndices, VisitIndex, _2);
  using InputDomain = _1;
  using ScatterType = vtkm::worklet::ScatterCounting;

  VTKM_CONT
  template <typename CountArrayType, typename DeviceTag>
  EdgesExtracter(const CountArrayType& counts, DeviceTag device)
    : Scatter(counts, device)
  {
  }

  VTKM_CONT ScatterType GetScatter() const { return this->Scatter; }

  template <typename CellShapeTag, typename PointIndexVecType, typename EdgeIndexVecType>
  VTKM_EXEC void operator()(CellShapeTag shape,
                            const PointIndexVecType& pointIndices,
                            vtkm::IdComponent visitIndex,
                            EdgeIndexVecType& edgeIndices) const
  {
    vtkm::Vec<vtkm::IdComponent, 2> localEdgeIndices = vtkm::exec::CellEdgeLocalIndices(
      pointIndices.GetNumberOfComponents(), visitIndex, shape, *this);
    vtkm::Id p1 = pointIndices[localEdgeIndices[0]];
    vtkm::Id p2 = pointIndices[localEdgeIndices[1]];
    // These indices need to be arranged in a definite order, as they will later be sorted to
    // detect duplicates
    edgeIndices[0] = p1 < p2 ? p1 : p2;
    edgeIndices[1] = p1 < p2 ? p2 : p1;
  }

private:
  ScatterType Scatter;
}; // struct EdgesCounter

struct ExtractEdgesFunctor
{
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 2>> EdgeIndices;

  VTKM_CONT
  ExtractEdgesFunctor(const vtkm::cont::DynamicCellSet& cellSet)
    : CellSet(cellSet)
  {
  }

  template <typename DeviceTag>
  VTKM_CONT bool operator()(DeviceTag)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> counts;
    vtkm::worklet::DispatcherMapTopology<EdgesCounter, DeviceTag>().Invoke(CellSet, counts);
    EdgesExtracter extractWorklet(counts, DeviceTag());
    vtkm::worklet::DispatcherMapTopology<EdgesExtracter, DeviceTag> extractDispatcher(
      extractWorklet);
    extractDispatcher.Invoke(CellSet, EdgeIndices);
    vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::template Sort<vtkm::Id2>(EdgeIndices);
    vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::template Unique<vtkm::Id2>(EdgeIndices);
    return true;
  }
};
} // namespace

struct MapperLineRenderer::InternalsType
{
  InternalsType()
    : InternalsType(nullptr, false)
  {
  }

  InternalsType(vtkm::rendering::Canvas* canvas, bool showInternalZones)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
  {
  }

  vtkm::rendering::Canvas* Canvas;
  bool ShowInternalZones;
};

MapperLineRenderer::MapperLineRenderer()
  : Internals(new InternalsType())
{
}

MapperLineRenderer::~MapperLineRenderer()
{
}

vtkm::rendering::Canvas* MapperLineRenderer::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperLineRenderer::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  this->Internals->Canvas = canvas;
}

bool MapperLineRenderer::GetShowInternalZones() const
{
  return this->Internals->ShowInternalZones;
}

void MapperLineRenderer::SetShowInternalZones(bool showInternalZones)
{
  this->Internals->ShowInternalZones = showInternalZones;
}

void MapperLineRenderer::StartScene()
{
  // Nothing needs to be done.
}

void MapperLineRenderer::EndScene()
{
  // Nothing needs to be done.
}

void MapperLineRenderer::RenderCells(const vtkm::cont::DynamicCellSet& cellSet,
                                     const vtkm::cont::CoordinateSystem& coords,
                                     const vtkm::cont::Field& scalarField,
                                     const vtkm::rendering::ColorTable& colorTable,
                                     const vtkm::rendering::Camera& camera,
                                     const vtkm::Range& scalarRange)
{
  ExtractEdgesFunctor functor(cellSet);
  vtkm::cont::TryExecute(functor);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 2>> edgeIndices = functor.EdgeIndices;

  WireframeRenderer renderer(this->Internals->Canvas, this->Internals->ShowInternalZones);
  if (!(this->Internals->ShowInternalZones))
  {
    CanvasRayTracer canvas(this->Internals->Canvas->GetWidth(),
                           this->Internals->Canvas->GetHeight());
    MapperRayTracer raytracer;
    canvas.Initialize();
    canvas.Activate();
    canvas.Clear();
    canvas.SetBackgroundColor(vtkm::rendering::Color::white);
    raytracer.SetCanvas(&canvas);
    raytracer.SetActiveColorTable(colorTable);
    raytracer.RenderCells(cellSet, coords, scalarField, colorTable, camera, scalarRange);
    renderer.SetSolidDepthBuffer(canvas.GetDepthBuffer());
  }

  renderer.SetCamera(camera);
  renderer.SetColorMap(this->ColorMap);
  renderer.SetData(coords, edgeIndices, scalarField, scalarRange);
  renderer.Render();
}

vtkm::rendering::Mapper* MapperLineRenderer::NewCopy() const
{
  return new vtkm::rendering::MapperLineRenderer(*this);
}
}
} // vtkm::rendering
