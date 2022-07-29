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
  void BuildExplicit(
    const vtkm::cont::CellSetExplicit<ShapeStorage, ConnectStorage, OffsetStorage>*,
    const vtkm::cont::UnknownArrayHandle* connections,
    const vtkm::cont::UnknownArrayHandle* offsets,
    vtkm::Id numberOfPoints,
    vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit* visitPointsWithCells,
    vtkm::cont::DeviceAdapterId* device) const
  {
    if (visitPointsWithCells->ElementsValid)
    {
      return; // Already computed reverse
    }

    using ConnectArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectStorage>;
    using OffsetArrayType = vtkm::cont::ArrayHandle<vtkm::Id, OffsetStorage>;
    if (connections->CanConvert<ConnectArrayType>() && offsets->CanConvert<OffsetArrayType>())
    {
      DoBuildReverseConnectivity(connections->AsArrayHandle<ConnectArrayType>(),
                                 offsets->AsArrayHandle<OffsetArrayType>(),
                                 numberOfPoints,
                                 *visitPointsWithCells,
                                 *device);
    }
  }

  void BuildExplicit(...) const
  {
    // Not an explicit cell set, so skip.
  }

  // Note that we are doing something a little weird here. We are taking a method templated
  // on the cell set type, getting pointers of many of the arguments, and then calling
  // a different overloaded method to do the actual work. Here is why.
  //
  // Our ultimate goal is to call one method if operating on `CellSetExplict` or
  // _any subclass_. We also want to ignore any cell type that is not a `CellSetExplicit`
  // or one of its sublcasses. So we want one templated method that specializes on a
  // `CellSetExplicit` with its three template arguments and another that is templated on
  // any `CellSet`. That works for a class of `CellSetExplicit`, but not of a subclass.
  // A subclass will match the more generic class.
  //
  // We can get around that by use variadic arguments (e.g. `...` for the arguments), which
  // we can easily do since we just want to ignore the arguments. C++ will match the arguments
  // with templates before matching the C variadic arguments. However, these variadic
  // arguments only work for POD types. To convert to POD types, we make a new overloaded
  // method that accepts pointers instead. (Not sure why pointers work but references do not.)
  template <typename CellSetType>
  void operator()(
    const CellSetType& cellset,
    const vtkm::cont::UnknownArrayHandle& connections,
    const vtkm::cont::UnknownArrayHandle& offsets,
    vtkm::Id numberOfPoints,
    vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
    vtkm::cont::DeviceAdapterId device) const
  {
    this->BuildExplicit(
      &cellset, &connections, &offsets, numberOfPoints, &visitPointsWithCells, &device);
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
               "BuildReverseConnectivity attempted for all known cell set types; "
               "falling back to copy connectivity arrays.");
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
