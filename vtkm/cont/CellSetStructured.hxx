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
CellSetStructured<DIMENSION>& CellSetStructured<DIMENSION>::operator=(
  const CellSetStructured<DIMENSION>& src)
{
  this->CellSet::operator=(src);
  this->Structure = src.Structure;
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
