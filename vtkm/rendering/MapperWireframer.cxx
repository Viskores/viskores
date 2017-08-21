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
#include <vtkm/filter/ExternalFaces.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Wireframer.h>
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
    if (shape.Id == vtkm::CELL_SHAPE_LINE)
    {
      return 1;
    }
    else
    {
      return vtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, *this);
    }
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
    vtkm::Id p1, p2;
    if (shape.Id == vtkm::CELL_SHAPE_LINE)
    {
      p1 = pointIndices[0];
      p2 = pointIndices[1];
    }
    else
    {
      vtkm::Vec<vtkm::IdComponent, 2> localEdgeIndices = vtkm::exec::CellEdgeLocalIndices(
        pointIndices.GetNumberOfComponents(), visitIndex, shape, *this);
      p1 = pointIndices[localEdgeIndices[0]];
      p2 = pointIndices[localEdgeIndices[1]];
    }
    // These indices need to be arranged in a definite order, as they will later be sorted to
    // detect duplicates
    edgeIndices[0] = p1 < p2 ? p1 : p2;
    edgeIndices[1] = p1 < p2 ? p2 : p1;
  }

private:
  ScatterType Scatter;
}; // struct EdgesExtracter

struct ExtractUniqueEdges
{
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 2>> EdgeIndices;

  VTKM_CONT
  ExtractUniqueEdges(const vtkm::cont::DynamicCellSet& cellSet)
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
}; // struct ExtractUniqueEdges
} // namespace

struct MapperWireframer::InternalsType
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
}; // struct MapperWireframer::InternalsType

MapperWireframer::MapperWireframer()
  : Internals(new InternalsType(nullptr, false))
{
}

MapperWireframer::~MapperWireframer()
{
}

vtkm::rendering::Canvas* MapperWireframer::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperWireframer::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  this->Internals->Canvas = canvas;
}

bool MapperWireframer::GetShowInternalZones() const
{
  return this->Internals->ShowInternalZones;
}

void MapperWireframer::SetShowInternalZones(bool showInternalZones)
{
  this->Internals->ShowInternalZones = showInternalZones;
}

void MapperWireframer::StartScene()
{
  // Nothing needs to be done.
}

void MapperWireframer::EndScene()
{
  // Nothing needs to be done.
}

void MapperWireframer::RenderCells(const vtkm::cont::DynamicCellSet& inCellSet,
                                   const vtkm::cont::CoordinateSystem& coords,
                                   const vtkm::cont::Field& inScalarField,
                                   const vtkm::rendering::ColorTable& colorTable,
                                   const vtkm::rendering::Camera& camera,
                                   const vtkm::Range& scalarRange)
{
  vtkm::cont::DynamicCellSet cellSet = inCellSet;
  vtkm::cont::Field field = inScalarField;
  if (!(this->Internals->ShowInternalZones))
  {
    // If internal zones are to be hidden, the number of edges processed can be reduced by first
    // running the external faces filter on the input cell set and using the resulting cell set.
    vtkm::cont::DataSet dataSet;
    dataSet.AddCoordinateSystem(coords);
    dataSet.AddCellSet(inCellSet);
    vtkm::filter::ExternalFaces externalFaces;
    externalFaces.SetCompactPoints(false);
    externalFaces.SetPassPolyData(true);
    vtkm::filter::Result result = externalFaces.Execute(dataSet);
    externalFaces.MapFieldOntoOutput(result, inScalarField);
    cellSet = result.GetDataSet().GetCellSet();
    field = result.GetDataSet().GetField(0);
  }

  // Extract unique edges from the cell set.
  ExtractUniqueEdges extracter(cellSet);
  vtkm::cont::TryExecute(extracter);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 2>> edgeIndices = extracter.EdgeIndices;

  Wireframer renderer(this->Internals->Canvas, this->Internals->ShowInternalZones);
  // Render the cell set using a raytracer, on a separate canvas, and use the generated depth
  // buffer, which represents the solid mesh, to avoid drawing on the internal zones
  if (!(this->Internals->ShowInternalZones))
  {
    CanvasRayTracer canvas(this->Internals->Canvas->GetWidth(),
                           this->Internals->Canvas->GetHeight());
    canvas.SetBackgroundColor(vtkm::rendering::Color::white);
    canvas.Initialize();
    canvas.Activate();
    canvas.Clear();
    MapperRayTracer raytracer;
    raytracer.SetCanvas(&canvas);
    raytracer.SetActiveColorTable(colorTable);
    raytracer.RenderCells(cellSet, coords, field, colorTable, camera, scalarRange);
    renderer.SetSolidDepthBuffer(canvas.GetDepthBuffer());
  }

  renderer.SetCamera(camera);
  renderer.SetColorMap(this->ColorMap);
  renderer.SetData(coords, edgeIndices, field, scalarRange);
  renderer.Render();
}

vtkm::rendering::Mapper* MapperWireframer::NewCopy() const
{
  return new vtkm::rendering::MapperWireframer(*this);
}
}
} // vtkm::rendering
