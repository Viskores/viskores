//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

namespace vtkm
{
namespace cont
{

template <vtkm::IdComponent DIMENSION>
CellSetStructured<DIMENSION>::CellSetStructured(const CellSetStructured<DIMENSION>& src)
  : CellSet(src)
  , Structure(src.Structure)
{
}

template <vtkm::IdComponent DIMENSION>
CellSetStructured<DIMENSION>::CellSetStructured(CellSetStructured<DIMENSION>&& src) noexcept
  : CellSet(std::forward<CellSet>(src)),
    Structure(std::move(src.Structure))
{
}

template <vtkm::IdComponent DIMENSION>
CellSetStructured<DIMENSION>& CellSetStructured<DIMENSION>::operator=(
  const CellSetStructured<DIMENSION>& src)
{
  this->CellSet::operator=(src);
  this->Structure = src.Structure;
  return *this;
}

template <vtkm::IdComponent DIMENSION>
CellSetStructured<DIMENSION>& CellSetStructured<DIMENSION>::operator=(
  CellSetStructured<DIMENSION>&& src) noexcept
{
  this->CellSet::operator=(std::forward<CellSet>(src));
  this->Structure = std::move(src.Structure);
  return *this;
}

template <vtkm::IdComponent DIMENSION>
template <typename TopologyElement>
typename CellSetStructured<DIMENSION>::SchedulingRangeType
  CellSetStructured<DIMENSION>::GetSchedulingRange(TopologyElement) const
{
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
  return this->Structure.GetSchedulingRange(TopologyElement());
}

template <vtkm::IdComponent DIMENSION>
template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
typename CellSetStructured<
  DIMENSION>::template ExecutionTypes<DeviceAdapter, FromTopology, ToTopology>::ExecObjectType
  CellSetStructured<DIMENSION>::PrepareForInput(DeviceAdapter, FromTopology, ToTopology) const
{
  using ConnectivityType =
    typename ExecutionTypes<DeviceAdapter, FromTopology, ToTopology>::ExecObjectType;
  return ConnectivityType(this->Structure);
}

template <vtkm::IdComponent DIMENSION>
void CellSetStructured<DIMENSION>::PrintSummary(std::ostream& out) const
{
  out << "  StructuredCellSet: " << this->GetName() << std::endl;
  this->Structure.PrintSummary(out);
}
}
}
