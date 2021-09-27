//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtk_m_cont_CellSetExplicit_cxx

#include <vtkm/cont/CellSetExplicit.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DefaultTypes.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/cont/internal/ReverseConnectivityBuilder.h>

namespace
{

template <typename ConnectStorage, typename OffsetStorage>
void DoBuildReverseConnectivity(
  const vtkm::cont::ArrayHandle<vtkm::Id, ConnectStorage>& connections,
  const vtkm::cont::ArrayHandle<vtkm::Id, OffsetStorage>& offsets,
  vtkm::Id numberOfPoints,
  vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
  vtkm::cont::DeviceAdapterId suggestedDevice)
{
  using CellsWithPointsConnectivity = vtkm::cont::internal::
    ConnectivityExplicitInternals<VTKM_DEFAULT_STORAGE_TAG, ConnectStorage, OffsetStorage>;

  // Make a fake visitCellsWithPoints to pass to ComputeRConnTable. This is a bit of a
  // patchwork from changing implementation.
  CellsWithPointsConnectivity visitCellsWithPoints;
  visitCellsWithPoints.ElementsValid = true;
  visitCellsWithPoints.Connectivity = connections;
  visitCellsWithPoints.Offsets = offsets;

  bool success =
    vtkm::cont::TryExecuteOnDevice(suggestedDevice, [&](vtkm::cont::DeviceAdapterId realDevice) {
      vtkm::cont::internal::ComputeRConnTable(
        visitPointsWithCells, visitCellsWithPoints, numberOfPoints, realDevice);
      return true;
    });

  if (!success)
  {
    throw vtkm::cont::ErrorExecution("Failed to run CellSetExplicit reverse "
                                     "connectivity builder.");
  }
}

struct BuildReverseConnectivityForCellSetType
{
  template <typename ShapeStorage, typename ConnectStorage, typename OffsetStorage>
  void operator()(
    const vtkm::cont::CellSetExplicit<ShapeStorage, ConnectStorage, OffsetStorage>&,
    const vtkm::cont::UnknownArrayHandle& connections,
    const vtkm::cont::UnknownArrayHandle& offsets,
    vtkm::Id numberOfPoints,
    vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
    vtkm::cont::DeviceAdapterId device)
  {
    if (visitPointsWithCells.ElementsValid)
    {
      return; // Already computed reverse
    }

    using ConnectArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectStorage>;
    using OffsetArrayType = vtkm::cont::ArrayHandle<vtkm::Id, OffsetStorage>;
    if (connections.CanConvert<ConnectArrayType>() && offsets.CanConvert<OffsetArrayType>())
    {
      DoBuildReverseConnectivity(connections.AsArrayHandle<ConnectArrayType>(),
                                 offsets.AsArrayHandle<OffsetArrayType>(),
                                 numberOfPoints,
                                 visitPointsWithCells,
                                 device);
    }
  }

  template <typename CellSetType>
  void operator()(const CellSetType&,
                  const vtkm::cont::UnknownArrayHandle&,
                  const vtkm::cont::UnknownArrayHandle&,
                  vtkm::Id,
                  vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit&,
                  vtkm::cont::DeviceAdapterId)
  {
    // Not an explicit cell set, so skip.
  }
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{

template class VTKM_CONT_EXPORT CellSetExplicit<>; // default
template class VTKM_CONT_EXPORT
  CellSetExplicit<typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
                  VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
                  typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>;

namespace detail
{

void BuildReverseConnectivity(
  const vtkm::cont::UnknownArrayHandle& connections,
  const vtkm::cont::UnknownArrayHandle& offsets,
  vtkm::Id numberOfPoints,
  vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
  vtkm::cont::DeviceAdapterId device)
{
  if (visitPointsWithCells.ElementsValid)
  {
    return; // Already computed
  }

  vtkm::ListForEach(BuildReverseConnectivityForCellSetType{},
                    VTKM_DEFAULT_CELL_SET_LIST{},
                    connections,
                    offsets,
                    numberOfPoints,
                    visitPointsWithCells,
                    device);

  if (!visitPointsWithCells.ElementsValid)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "BuildReverseConnectivity failed for all known cell set types. "
               "Attempting to copy connectivity arrays.");
    vtkm::cont::ArrayHandle<vtkm::Id> connectionsCopy;
    vtkm::cont::ArrayCopy(connections, connectionsCopy);
    vtkm::cont::ArrayHandle<vtkm::Id> offsetsCopy;
    vtkm::cont::ArrayCopy(offsets, offsetsCopy);
    DoBuildReverseConnectivity(
      connectionsCopy, offsetsCopy, numberOfPoints, visitPointsWithCells, device);
  }
}

} // namespace vtkm::cont::detail

} // namespace vtkm::cont
} // namespace vtkm
