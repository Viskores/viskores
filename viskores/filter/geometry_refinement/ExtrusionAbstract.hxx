//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_geometry_refinement_ExtrusionAbstract_hxx
#define viskores_filter_geometry_refinement_ExtrusionAbstract_hxx

#include <viskores/filter/geometry_refinement/ExtrusionAbstract.h>

#include <viskores/BinaryOperators.h>
#include <viskores/CellShape.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/geometry_refinement/Triangulate.h>
#include <viskores/filter/geometry_refinement/worklet/Extrusion.h>

#include <limits>
#include <type_traits>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{
namespace internal
{

struct IsShapeTriangle
{
  VISKORES_EXEC_CONT
  bool operator()(viskores::UInt8 shape) const { return shape == viskores::CELL_SHAPE_TRIANGLE; }
};

struct IsThreeIndices
{
  template <typename ValueType>
  VISKORES_EXEC_CONT bool operator()(ValueType count) const
  {
    return count == 3;
  }
};

struct BinaryAnd
{
  VISKORES_EXEC_CONT
  bool operator()(bool u, bool v) const { return u && v; }
};

template <typename ConnectivityArrayType>
VISKORES_CONT void ValidateConnectivityRange(const ConnectivityArrayType& connectivity,
                                             viskores::Id numberOfPoints,
                                             const std::string& filterName)
{
  const viskores::Vec<viskores::Id, 2> range = viskores::cont::Algorithm::Reduce(
    connectivity,
    viskores::make_Vec(std::numeric_limits<viskores::Id>::max(),
                       std::numeric_limits<viskores::Id>::lowest()),
    viskores::MinAndMax<viskores::Id>{});
  if ((range[0] < 0) || (range[1] >= numberOfPoints))
  {
    throw viskores::cont::ErrorFilterExecution(
      filterName + ": triangle connectivity contains an invalid point id.");
  }
}

VISKORES_CONT inline viskores::cont::ArrayHandle<viskores::Id> MakeTriangleConnectivity(
  const viskores::cont::UnknownCellSet& unknownCellSet,
  const std::string& filterName)
{
  if (unknownCellSet.GetNumberOfCells() <= 0)
  {
    throw viskores::cont::ErrorFilterExecution(filterName + ": input has no cells.");
  }
  if (unknownCellSet.GetNumberOfPoints() <= 0)
  {
    throw viskores::cont::ErrorFilterExecution(filterName + ": input has no points.");
  }

  viskores::cont::ArrayHandle<viskores::Id> connectivity;

  if (unknownCellSet.CanConvert<viskores::cont::CellSetSingleType<>>())
  {
    const auto cellSet = unknownCellSet.AsCellSet<viskores::cont::CellSetSingleType<>>();
    if (cellSet.GetCellShapeAsId() != viskores::CellShapeTagTriangle::Id)
    {
      throw viskores::cont::ErrorFilterExecution(filterName + ": input cells must be triangles.");
    }
    connectivity = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                                viskores::TopologyElementTagPoint{});
  }
  else if (unknownCellSet.CanConvert<viskores::cont::CellSetExplicit<>>())
  {
    const auto cellSet = unknownCellSet.AsCellSet<viskores::cont::CellSetExplicit<>>();
    const auto shapes = cellSet.GetShapesArray(viskores::TopologyElementTagCell{},
                                               viskores::TopologyElementTagPoint{});
    const bool allTriangleShapes = viskores::cont::Algorithm::Reduce(
      viskores::cont::make_ArrayHandleTransform(shapes, IsShapeTriangle{}), true, BinaryAnd{});
    const auto numIndices = cellSet.GetNumIndicesArray(viskores::TopologyElementTagCell{},
                                                       viskores::TopologyElementTagPoint{});
    const bool allTriangleCounts = viskores::cont::Algorithm::Reduce(
      viskores::cont::make_ArrayHandleTransform(numIndices, IsThreeIndices{}), true, BinaryAnd{});
    if (!allTriangleShapes || !allTriangleCounts)
    {
      throw viskores::cont::ErrorFilterExecution(filterName + ": input cells must be triangles.");
    }
    connectivity = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                                viskores::TopologyElementTagPoint{});
  }
  else
  {
    throw viskores::cont::ErrorFilterExecution(
      filterName + ": input cell set must be explicit triangle topology.");
  }

  ValidateConnectivityRange(connectivity, unknownCellSet.GetNumberOfPoints(), filterName);
  return connectivity;
}

VISKORES_CONT inline void ValidateCellSetExtrudeLimits(viskores::Id numberOfPoints,
                                                       viskores::Id numberOfPlanes,
                                                       viskores::Id connectivitySize,
                                                       const std::string& filterName)
{
  const viskores::Id maxInt32 =
    static_cast<viskores::Id>(std::numeric_limits<viskores::Int32>::max());
  if ((numberOfPoints > maxInt32) || (numberOfPlanes > maxInt32) || (connectivitySize > maxInt32))
  {
    throw viskores::cont::ErrorFilterExecution(
      filterName + ": compact CellSetExtrude output exceeds Int32 topology limits.");
  }
}

VISKORES_CONT inline viskores::cont::ArrayHandle<viskores::Int32> MakeInt32Connectivity(
  const viskores::cont::ArrayHandle<viskores::Id>& connectivity)
{
  viskores::cont::ArrayHandle<viskores::Int32> connectivity32;
  viskores::cont::ArrayCopy(connectivity, connectivity32);
  return connectivity32;
}

VISKORES_CONT inline viskores::cont::ArrayHandle<viskores::Int32> MakeIdentityNextNode(
  viskores::Id numberOfPoints)
{
  viskores::cont::ArrayHandle<viskores::Int32> nextNode;
  viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleCounting(
                                    viskores::Int32{ 0 }, viskores::Int32{ 1 }, numberOfPoints),
                                  nextNode);
  return nextNode;
}

VISKORES_CONT inline bool HasPointFields(const viskores::cont::DataSet& input)
{
  const viskores::IdComponent numberOfFields = input.GetNumberOfFields();
  for (viskores::IdComponent fieldIndex = 0; fieldIndex < numberOfFields; ++fieldIndex)
  {
    if (input.GetField(fieldIndex).IsPointField())
    {
      return true;
    }
  }
  return false;
}

VISKORES_CONT inline bool HasCellFields(const viskores::cont::DataSet& input)
{
  const viskores::IdComponent numberOfFields = input.GetNumberOfFields();
  for (viskores::IdComponent fieldIndex = 0; fieldIndex < numberOfFields; ++fieldIndex)
  {
    if (input.GetField(fieldIndex).IsCellField())
    {
      return true;
    }
  }
  return false;
}

VISKORES_CONT inline bool MapExtrudedField(
  viskores::cont::DataSet& result,
  const viskores::cont::Field& field,
  const viskores::cont::ArrayHandle<viskores::Id>& pointPermutation,
  const viskores::cont::ArrayHandle<viskores::Id>& cellPermutation)
{
  if (field.IsPointField())
  {
    return viskores::filter::MapFieldPermutation(field, pointPermutation, result);
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, cellPermutation, result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}

} // namespace internal

template <typename MakeCoordinateWorklet,
          typename MakeOrientationWorklet,
          typename ValidateCoordinates>
VISKORES_CONT viskores::cont::DataSet ExtrusionAbstract::ExecuteTriangleExtrusion(
  const viskores::cont::DataSet& input,
  const std::string& filterName,
  bool closeSweep,
  MakeCoordinateWorklet makeCoordinateWorklet,
  MakeOrientationWorklet makeOrientationWorklet,
  ValidateCoordinates validateCoordinates)
{
  if (closeSweep)
  {
    if (this->NumberOfPlanes < 3)
    {
      throw viskores::cont::ErrorFilterExecution(filterName +
                                                 ": closed sweeps require at least 3 planes.");
    }
  }
  else if (this->NumberOfPlanes < 2)
  {
    throw viskores::cont::ErrorFilterExecution(filterName +
                                               ": open sweeps require at least 2 planes.");
  }

  viskores::cont::DataSet workingInput = input;
  if (this->TriangulateInput)
  {
    viskores::filter::geometry_refinement::Triangulate triangulate;
    workingInput = triangulate.Execute(input);
  }

  const auto& inputCoordinates =
    workingInput.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  const viskores::Id numberOfPoints = workingInput.GetCellSet().GetNumberOfPoints();
  if (inputCoordinates.GetData().GetNumberOfValues() != numberOfPoints)
  {
    throw viskores::cont::ErrorFilterExecution(
      filterName + ": coordinate count does not match cell set point count.");
  }

  viskores::cont::ArrayHandle<viskores::Id> connectivity =
    internal::MakeTriangleConnectivity(workingInput.GetCellSet(), filterName);
  const viskores::Id numberOfCellsPerPlane = connectivity.GetNumberOfValues() / 3;
  viskores::cont::ArrayHandle<viskores::Id> orientedConnectivity;
  const viskores::Id numberOfOutputPoints = this->NumberOfPlanes * numberOfPoints;
  const viskores::Id numberOfOutputCells =
    (closeSweep ? this->NumberOfPlanes : (this->NumberOfPlanes - 1)) * numberOfCellsPerPlane;

  viskores::cont::UnknownArrayHandle outputCoordinates;
  auto resolveCoordinates = [&](const auto& concreteCoordinates)
  {
    using CoordinateArrayType = std::decay_t<decltype(concreteCoordinates)>;
    using CoordinateType = typename CoordinateArrayType::ValueType;

    validateCoordinates(concreteCoordinates);

    viskores::cont::ArrayHandle<CoordinateType> generatedCoordinates;
    this->Invoke(makeCoordinateWorklet(numberOfPoints),
                 viskores::cont::ArrayHandleIndex(numberOfOutputPoints),
                 concreteCoordinates,
                 generatedCoordinates);
    outputCoordinates = generatedCoordinates;

    this->Invoke(makeOrientationWorklet(),
                 viskores::cont::make_ArrayHandleGroupVec<3>(connectivity),
                 concreteCoordinates,
                 viskores::cont::make_ArrayHandleGroupVec<3>(orientedConnectivity));
  };
  this->CastAndCallVecField<3>(inputCoordinates.GetData(), resolveCoordinates);

  // Match ExternalFaces: check field associations before creating maps that may not be used.
  const bool hasPointFields = internal::HasPointFields(workingInput);
  const bool hasCellFields = internal::HasCellFields(workingInput);

  viskores::cont::ArrayHandle<viskores::Id> pointPermutation;
  if (hasPointFields)
  {
    viskores::cont::Algorithm::Copy(
      viskores::cont::make_ArrayHandleTransform(
        viskores::cont::ArrayHandleIndex(numberOfOutputPoints),
        viskores::worklet::ExtrusionOutputToInputPlane{ numberOfPoints }),
      pointPermutation);
  }

  viskores::cont::ArrayHandle<viskores::Id> cellPermutation;
  if (hasCellFields)
  {
    viskores::cont::Algorithm::Copy(
      viskores::cont::make_ArrayHandleTransform(
        viskores::cont::ArrayHandleIndex(numberOfOutputCells),
        viskores::worklet::ExtrusionOutputToInputPlane{ numberOfCellsPerPlane }),
      cellPermutation);
  }

  auto mapper = [&](auto& result, const auto& field)
  { return internal::MapExtrudedField(result, field, pointPermutation, cellPermutation); };

  if (this->CompactOutput)
  {
    internal::ValidateCellSetExtrudeLimits(
      numberOfPoints, this->NumberOfPlanes, orientedConnectivity.GetNumberOfValues(), filterName);
    viskores::cont::ArrayHandle<viskores::Int32> connectivity32 =
      internal::MakeInt32Connectivity(orientedConnectivity);
    viskores::cont::ArrayHandle<viskores::Int32> nextNode =
      internal::MakeIdentityNextNode(numberOfPoints);
    viskores::cont::CellSetExtrude outputCells(connectivity32,
                                               static_cast<viskores::Int32>(numberOfPoints),
                                               static_cast<viskores::Int32>(this->NumberOfPlanes),
                                               nextNode,
                                               closeSweep);
    return this->CreateResultCoordinateSystem(
      workingInput, outputCells, inputCoordinates.GetName(), outputCoordinates, mapper);
  }
  else
  {
    viskores::cont::ArrayHandle<viskores::Id> explicitConnectivity;
    this->Invoke(
      viskores::worklet::ExtrudedCellSetToExplicitWedgeConnectivity{
        numberOfCellsPerPlane, numberOfPoints, this->NumberOfPlanes },
      viskores::cont::ArrayHandleIndex(numberOfOutputCells * 6),
      orientedConnectivity,
      explicitConnectivity);

    viskores::cont::CellSetSingleType<> explicitCells;
    explicitCells.Fill(numberOfOutputPoints, viskores::CELL_SHAPE_WEDGE, 6, explicitConnectivity);

    return this->CreateResultCoordinateSystem(
      workingInput, explicitCells, inputCoordinates.GetName(), outputCoordinates, mapper);
  }
}

} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_ExtrusionAbstract_hxx
