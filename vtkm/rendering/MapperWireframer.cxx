//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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

class Convert1DCoordinates : public vtkm::worklet::WorkletMapField
{
private:
public:
  VTKM_CONT
  Convert1DCoordinates() {}

  typedef void ControlSignature(FieldIn<>,
                                FieldIn<vtkm::TypeListTagScalarAll>,
                                FieldOut<>,
                                FieldOut<>);

  typedef void ExecutionSignature(_1, _2, _3, _4);
  template <typename ScalarType>
  VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Float32, 3>& inCoord,
                            const ScalarType& scalar,
                            vtkm::Vec<vtkm::Float32, 3>& outCoord,
                            vtkm::Float32& fieldOut) const
  {
    //
    // Rendering supports lines based on a cellSetStructured<1>
    // where only the x coord matters. It creates a y based on
    // the scalar values and connects all the points with lines.
    // So, we need to convert it back to something that can
    // actuall be rendered.
    //
    outCoord[0] = inCoord[0];
    outCoord[1] = static_cast<vtkm::Float32>(scalar);
    outCoord[2] = 0.f;
    // all lines have the same color
    fieldOut = 1.f;
  }
}; // convert coords

#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
struct EdgesCounter : public vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(CellSetIn cellSet, FieldOutCell<> numEdges);
  typedef _2 ExecutionSignature(CellShape shape, PointCount numPoints);
  using InputDomain = _1;

  template <typename CellShapeTag>
  VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape, vtkm::IdComponent numPoints) const
  {
    //TODO: Remove the if/then with templates.
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
    //TODO: Remove the if/then with templates.
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

#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif

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
    : InternalsType(nullptr, false, false)
  {
  }

  InternalsType(vtkm::rendering::Canvas* canvas, bool showInternalZones, bool isOverlay)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
    , IsOverlay(isOverlay)
  {
  }

  vtkm::rendering::Canvas* Canvas;
  bool ShowInternalZones;
  bool IsOverlay;
}; // struct MapperWireframer::InternalsType

MapperWireframer::MapperWireframer()
  : Internals(new InternalsType(nullptr, false, false))
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

bool MapperWireframer::GetIsOverlay() const
{
  return this->Internals->IsOverlay;
}

void MapperWireframer::SetIsOverlay(bool isOverlay)
{
  this->Internals->IsOverlay = isOverlay;
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

  bool is1D = cellSet.IsSameType(vtkm::cont::CellSetStructured<1>());

  vtkm::cont::CoordinateSystem actualCoords = coords;
  vtkm::cont::Field actualField = inScalarField;

  if (is1D)
  {

    bool isSupportedField = inScalarField.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS;
    if (!isSupportedField)
    {
      throw vtkm::cont::ErrorBadValue(
        "WireFramer: field must be associated with points for 1D cell set");
    }
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> newCoords;
    vtkm::cont::ArrayHandle<vtkm::Float32> newScalars;
    //
    // Convert the cell set into something we can draw
    //
    vtkm::worklet::DispatcherMapField<Convert1DCoordinates, vtkm::cont::DeviceAdapterTagSerial>(
      Convert1DCoordinates())
      .Invoke(coords.GetData(), inScalarField.GetData(), newCoords, newScalars);

    actualCoords = vtkm::cont::CoordinateSystem("coords", newCoords);
    actualField =
      vtkm::cont::Field(inScalarField.GetName(), vtkm::cont::Field::ASSOC_POINTS, newScalars);
    vtkm::Id numCells = cellSet.GetNumberOfCells();
    vtkm::cont::ArrayHandle<vtkm::Id> conn;
    conn.Allocate(numCells * 2);
    auto connPortal = conn.GetPortalControl();
    for (int i = 0; i < numCells; ++i)
    {
      connPortal.Set(i * 2 + 0, i);
      connPortal.Set(i * 2 + 1, i + 1);
    }

    vtkm::cont::CellSetSingleType<> newCellSet("cells");
    newCellSet.Fill(newCoords.GetNumberOfValues(), vtkm::CELL_SHAPE_LINE, 2, conn);
    cellSet = vtkm::cont::DynamicCellSet(newCellSet);
  }
  bool isLines = false;
  // Check for a cell set that is already lines
  // Since there is no need to de external faces or
  // render the depth of the mesh to hide internal zones
  if (cellSet.IsSameType(vtkm::cont::CellSetSingleType<>()))
  {
    auto singleType = cellSet.Cast<vtkm::cont::CellSetSingleType<>>();
    isLines = singleType.GetCellShape(0) == vtkm::CELL_SHAPE_LINE;
  }

  bool doExternalFaces = !(this->Internals->ShowInternalZones) && !isLines && !is1D;
  if (doExternalFaces)
  {
    // If internal zones are to be hidden, the number of edges processed can be reduced by
    // running the external faces filter on the input cell set.
    vtkm::cont::DataSet dataSet;
    dataSet.AddCoordinateSystem(actualCoords);
    dataSet.AddCellSet(inCellSet);
    vtkm::filter::ExternalFaces externalFaces;
    externalFaces.SetCompactPoints(false);
    externalFaces.SetPassPolyData(true);
    vtkm::filter::Result result = externalFaces.Execute(dataSet);
    externalFaces.MapFieldOntoOutput(result, inScalarField);
    cellSet = result.GetDataSet().GetCellSet();
    actualField = result.GetDataSet().GetField(0);
  }

  // Extract unique edges from the cell set.
  ExtractUniqueEdges extracter(cellSet);
  vtkm::cont::TryExecute(extracter);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 2>> edgeIndices = extracter.EdgeIndices;

  Wireframer renderer(
    this->Internals->Canvas, this->Internals->ShowInternalZones, this->Internals->IsOverlay);
  // Render the cell set using a raytracer, on a separate canvas, and use the generated depth
  // buffer, which represents the solid mesh, to avoid drawing on the internal zones
  bool renderDepth =
    !(this->Internals->ShowInternalZones) && !(this->Internals->IsOverlay) && !isLines && !is1D;
  if (renderDepth)
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
    raytracer.RenderCells(cellSet, actualCoords, actualField, colorTable, camera, scalarRange);
    renderer.SetSolidDepthBuffer(canvas.GetDepthBuffer());
  }
  else
  {
    renderer.SetSolidDepthBuffer(this->Internals->Canvas->GetDepthBuffer());
  }

  renderer.SetCamera(camera);
  renderer.SetColorMap(this->ColorMap);
  renderer.SetData(actualCoords, edgeIndices, actualField, scalarRange);
  renderer.Render();
}

vtkm::rendering::Mapper* MapperWireframer::NewCopy() const
{
  return new vtkm::rendering::MapperWireframer(*this);
}
}
} // namespace vtkm::rendering
