//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_ConnectivityExplicitInternals_h
#define vtk_m_cont_internal_ConnectivityExplicitInternals_h

#include <vtkm/CellShape.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename NumIndicesArrayType, typename IndexOffsetArrayType, typename DeviceAdapterTag>
void buildIndexOffsets(const NumIndicesArrayType& numIndices,
                       IndexOffsetArrayType& offsets,
                       DeviceAdapterTag,
                       std::true_type)
{
  //We first need to make sure that NumIndices and IndexOffsetArrayType
  //have the same type so we can call scane exclusive
  using CastedNumIndicesType = vtkm::cont::ArrayHandleCast<vtkm::Id, NumIndicesArrayType>;

  // Although technically we are making changes to this object, the changes
  // are logically consistent with the previous state, so we consider it
  // valid under const.
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  Algorithm::ScanExclusive(CastedNumIndicesType(numIndices), offsets);
}

template <typename NumIndicesArrayType, typename IndexOffsetArrayType, typename DeviceAdapterTag>
void buildIndexOffsets(const NumIndicesArrayType&,
                       IndexOffsetArrayType&,
                       DeviceAdapterTag,
                       std::false_type)
{
  //this is a no-op as the storage for the offsets is an implicit handle
  //and should already be built. This signature exists so that
  //the compiler doesn't try to generate un-used code that will
  //try and run Algorithm::ScanExclusive on an implicit array which will
  //cause a compile time failure.
}

template <typename ArrayHandleIndices, typename ArrayHandleOffsets, typename DeviceAdapterTag>
void buildIndexOffsets(const ArrayHandleIndices& numIndices,
                       ArrayHandleOffsets offsets,
                       DeviceAdapterTag tag)
{
  using IsWriteable =
    vtkm::cont::internal::IsWriteableArrayHandle<ArrayHandleOffsets, DeviceAdapterTag>;
  buildIndexOffsets(numIndices, offsets, tag, typename IsWriteable::type());
}

template <typename ShapeStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename NumIndicesStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename IndexOffsetStorageTag = VTKM_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  using ShapeArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorageTag>;
  using NumIndicesArrayType = vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorageTag>;
  using ConnectivityArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>;
  using IndexOffsetArrayType = vtkm::cont::ArrayHandle<vtkm::Id, IndexOffsetStorageTag>;

  ShapeArrayType Shapes;
  NumIndicesArrayType NumIndices;
  ConnectivityArrayType Connectivity;
  mutable IndexOffsetArrayType IndexOffsets;

  bool ElementsValid;
  mutable bool IndexOffsetsValid;

  VTKM_CONT
  ConnectivityExplicitInternals()
    : ElementsValid(false)
    , IndexOffsetsValid(false)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfElements() const
  {
    VTKM_ASSERT(this->ElementsValid);

    return this->Shapes.GetNumberOfValues();
  }

  VTKM_CONT
  void ReleaseResourcesExecution()
  {
    this->Shapes.ReleaseResourcesExecution();
    this->NumIndices.ReleaseResourcesExecution();
    this->Connectivity.ReleaseResourcesExecution();
    this->IndexOffsets.ReleaseResourcesExecution();
  }

  template <typename Device>
  VTKM_CONT void BuildIndexOffsets(Device) const
  {
    VTKM_ASSERT(this->ElementsValid);

    if (!this->IndexOffsetsValid)
    {
      buildIndexOffsets(this->NumIndices, this->IndexOffsets, Device());
      this->IndexOffsetsValid = true;
    }
  }

  VTKM_CONT
  void BuildIndexOffsets(vtkm::cont::DeviceAdapterTagError) const
  {
    if (!this->IndexOffsetsValid)
    {
      throw vtkm::cont::ErrorBadType(
        "Cannot build indices using the error device. Must be created previously.");
    }
  }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    if (this->ElementsValid)
    {
      out << "     Shapes: ";
      vtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
      out << "     NumIndices: ";
      vtkm::cont::printSummary_ArrayHandle(this->NumIndices, out);
      out << "     Connectivity: ";
      vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
      if (this->IndexOffsetsValid)
      {
        out << "     IndexOffsets: ";
        vtkm::cont::printSummary_ArrayHandle(this->IndexOffsets, out);
      }
      else
      {
        out << "     IndexOffsets: Not Allocated" << std::endl;
      }
    }
    else
    {
      out << "     Not Allocated" << std::endl;
    }
  }
};


// Worklet to expand the PointToCell numIndices array by repeating cell index
class ExpandIndices : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<> cellIndex,
                                FieldIn<> offset,
                                FieldIn<> numIndices,
                                WholeArrayOut<> cellIndices);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_CONT
  ExpandIndices() {}

  template <typename PortalType>
  VTKM_EXEC void operator()(const vtkm::Id& cellIndex,
                            const vtkm::Id& offset,
                            const vtkm::Id& numIndices,
                            const PortalType& cellIndices) const
  {
    VTKM_ASSERT(cellIndices.GetNumberOfValues() >= offset + numIndices);
    vtkm::Id startIndex = offset;
    for (vtkm::Id i = 0; i < numIndices; i++)
    {
      cellIndices.Set(startIndex++, cellIndex);
    }
  }
};

class ScatterValues : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<> index, FieldIn<> value, WholeArrayOut<> output);
  typedef void ExecutionSignature(_1, _2, _3);
  using InputDomain = _1;

  template <typename T, typename PortalType>
  VTKM_EXEC void operator()(const vtkm::Id& index, const T& value, const PortalType& output) const
  {
    output.Set(index, value);
  }
};

template <typename PointToCell, typename C2PShapeStorageTag, typename Device>
void ComputeCellToPointConnectivity(ConnectivityExplicitInternals<C2PShapeStorageTag>& cell2Point,
                                    const PointToCell& point2Cell,
                                    vtkm::Id numberOfPoints,
                                    Device)
{
  // PointToCell connectivity array (point indices) will be
  // transformed into the CellToPoint numIndices array using reduction
  //
  // PointToCell numIndices array using expansion will be
  // transformed into the CellToPoint connectivity array

  if (cell2Point.ElementsValid)
  {
    return;
  }

  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

  // Sizes of the PointToCell information
  vtkm::Id numberOfCells = point2Cell.NumIndices.GetNumberOfValues();
  vtkm::Id connectivityLength = point2Cell.Connectivity.GetNumberOfValues();

  // PointToCell connectivity will be basis of CellToPoint numIndices
  vtkm::cont::ArrayHandle<vtkm::Id> pointIndices;
  Algorithm::Copy(point2Cell.Connectivity, pointIndices);

  // PointToCell numIndices will be basis of CellToPoint connectivity

  cell2Point.Connectivity.Allocate(connectivityLength);
  vtkm::cont::ArrayHandleCounting<vtkm::Id> index(0, 1, numberOfCells);

  vtkm::worklet::DispatcherMapField<ExpandIndices, Device> expandDispatcher;
  expandDispatcher.Invoke(
    index, point2Cell.IndexOffsets, point2Cell.NumIndices, cell2Point.Connectivity);

  // SortByKey where key is PointToCell connectivity and value is the expanded cellIndex
  Algorithm::SortByKey(pointIndices, cell2Point.Connectivity);

  // CellToPoint numIndices from the now sorted PointToCell connectivity
  vtkm::cont::ArrayHandleConstant<vtkm::IdComponent> numArray(1, connectivityLength);
  vtkm::cont::ArrayHandle<vtkm::Id> uniquePoints;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  Algorithm::ReduceByKey(pointIndices, numArray, uniquePoints, numIndices, vtkm::Add());

  // if not all the points have a cell
  if (uniquePoints.GetNumberOfValues() < numberOfPoints)
  {
    vtkm::cont::ArrayHandle<vtkm::IdComponent> fullNumIndices;
    Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, numberOfPoints),
                    fullNumIndices);
    vtkm::worklet::DispatcherMapField<ScatterValues, Device>().Invoke(
      uniquePoints, numIndices, fullNumIndices);
    numIndices = fullNumIndices;
  }

  // Set the CellToPoint information
  cell2Point.Shapes = vtkm::cont::make_ArrayHandleConstant(
    static_cast<vtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  cell2Point.NumIndices = numIndices;

  cell2Point.ElementsValid = true;
  cell2Point.IndexOffsetsValid = false;
}
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ConnectivityExplicitInternals_h
