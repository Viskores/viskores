//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

class CreateConnectivity : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CreateConnectivity() {}

  using ControlSignature = void(FieldIn, WholeArrayOut);

  using ExecutionSignature = void(_1, _2);

  template <typename ConnPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& i, ConnPortalType& connPortal) const
  {
    connPortal.Set(i * 2 + 0, i);
    connPortal.Set(i * 2 + 1, i + 1);
  }
}; // conn

class Convert1DCoordinates : public vtkm::worklet::WorkletMapField
{
private:
  bool LogY;
  bool LogX;

public:
  VTKM_CONT
  Convert1DCoordinates(bool logY, bool logX)
    : LogY(logY)
    , LogX(logX)
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldOut, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename ScalarType>
  VTKM_EXEC void operator()(const vtkm::Vec3f_32& inCoord,
                            const ScalarType& scalar,
                            vtkm::Vec3f_32& outCoord,
                            vtkm::Float32& fieldOut) const
  {
    //
    // Rendering supports lines based on a cellSetStructured<1>
    // where only the x coord matters. It creates a y based on
    // the scalar values and connects all the points with lines.
    // So, we need to convert it back to something that can
    // actually be rendered.
    //
    outCoord[0] = inCoord[0];
    outCoord[1] = static_cast<vtkm::Float32>(scalar);
    outCoord[2] = 0.f;
    if (LogY)
    {
      outCoord[1] = vtkm::Log10(outCoord[1]);
    }
    if (LogX)
    {
      outCoord[0] = vtkm::Log10(outCoord[0]);
    }
    // all lines have the same color
    fieldOut = 1.f;
  }
}; // convert coords

#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
struct EdgesCounter : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell numEdges);
  using ExecutionSignature = _2(CellShape shape, PointCount numPoints);
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

struct EdgesExtracter : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell edgeIndices);
  using ExecutionSignature = void(CellShape, PointIndices, VisitIndex, _2);
  using InputDomain = _1;
  using ScatterType = vtkm::worklet::ScatterCounting;

  VTKM_CONT
  template <typename CountArrayType>
  static ScatterType MakeScatter(const CountArrayType& counts)
  {
    return ScatterType(counts);
  }

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
      p1 = pointIndices[vtkm::exec::CellEdgeLocalIndex(
        pointIndices.GetNumberOfComponents(), 0, visitIndex, shape, *this)];
      p2 = pointIndices[vtkm::exec::CellEdgeLocalIndex(
        pointIndices.GetNumberOfComponents(), 1, visitIndex, shape, *this)];
    }
    // These indices need to be arranged in a definite order, as they will later be sorted to
    // detect duplicates
    edgeIndices[0] = p1 < p2 ? p1 : p2;
    edgeIndices[1] = p1 < p2 ? p2 : p1;
  }
}; // struct EdgesExtracter

#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
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
    , CompositeBackground(true)
  {
  }

  vtkm::rendering::Canvas* Canvas;
  bool ShowInternalZones;
  bool IsOverlay;
  bool CompositeBackground;
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
                                   const vtkm::cont::ColorTable& colorTable,
                                   const vtkm::rendering::Camera& camera,
                                   const vtkm::Range& scalarRange)
{
  vtkm::cont::DynamicCellSet cellSet = inCellSet;

  bool is1D = cellSet.IsSameType(vtkm::cont::CellSetStructured<1>());

  vtkm::cont::CoordinateSystem actualCoords = coords;
  vtkm::cont::Field actualField = inScalarField;

  if (is1D)
  {

    const bool isSupportedField = inScalarField.IsFieldPoint();
    if (!isSupportedField)
    {
      throw vtkm::cont::ErrorBadValue(
        "WireFramer: field must be associated with points for 1D cell set");
    }
    vtkm::cont::ArrayHandle<vtkm::Vec3f_32> newCoords;
    vtkm::cont::ArrayHandle<vtkm::Float32> newScalars;
    //
    // Convert the cell set into something we can draw
    //
    vtkm::worklet::DispatcherMapField<Convert1DCoordinates>(
      Convert1DCoordinates(this->LogarithmY, this->LogarithmX))
      .Invoke(coords.GetData(),
              inScalarField.GetData().ResetTypes(vtkm::TypeListFieldScalar()),
              newCoords,
              newScalars);

    actualCoords = vtkm::cont::CoordinateSystem("coords", newCoords);
    actualField = vtkm::cont::Field(
      inScalarField.GetName(), vtkm::cont::Field::Association::POINTS, newScalars);

    vtkm::Id numCells = cellSet.GetNumberOfCells();
    vtkm::cont::ArrayHandle<vtkm::Id> conn;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> iter =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), numCells);
    conn.Allocate(numCells * 2);
    vtkm::worklet::DispatcherMapField<CreateConnectivity>(CreateConnectivity()).Invoke(iter, conn);

    vtkm::cont::CellSetSingleType<> newCellSet;
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
    dataSet.SetCellSet(inCellSet);
    dataSet.AddField(inScalarField);
    vtkm::filter::ExternalFaces externalFaces;
    externalFaces.SetCompactPoints(false);
    externalFaces.SetPassPolyData(true);
    vtkm::cont::DataSet output = externalFaces.Execute(dataSet);
    cellSet = output.GetCellSet();
    actualField = output.GetField(0);
  }

  // Extract unique edges from the cell set.
  vtkm::cont::ArrayHandle<vtkm::IdComponent> counts;
  vtkm::cont::ArrayHandle<vtkm::Id2> edgeIndices;
  vtkm::worklet::DispatcherMapTopology<EdgesCounter>().Invoke(cellSet, counts);
  vtkm::worklet::DispatcherMapTopology<EdgesExtracter> extractDispatcher(
    EdgesExtracter::MakeScatter(counts));
  extractDispatcher.Invoke(cellSet, edgeIndices);
  vtkm::cont::Algorithm::template Sort<vtkm::Id2>(edgeIndices);
  vtkm::cont::Algorithm::template Unique<vtkm::Id2>(edgeIndices);

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

  if (this->Internals->CompositeBackground)
  {
    this->Internals->Canvas->BlendBackground();
  }
}

void MapperWireframer::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

vtkm::rendering::Mapper* MapperWireframer::NewCopy() const
{
  return new vtkm::rendering::MapperWireframer(*this);
}
}
} // namespace vtkm::rendering
