//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetStructured_hxx
#define vtk_m_cont_CellSetStructured_hxx

namespace vtkm
{
namespace cont
{

template <vtkm::IdComponent DIMENSION>
template <typename TopologyElement>
typename CellSetStructured<DIMENSION>::SchedulingRangeType
  CellSetStructured<DIMENSION>::GetSchedulingRange(TopologyElement) const
{
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
  return this->Structure.GetSchedulingRange(TopologyElement());
}

template <vtkm::IdComponent DIMENSION>
template <typename VisitTopology, typename IncidentTopology>
typename CellSetStructured<DIMENSION>::template ExecConnectivityType<VisitTopology,
                                                                     IncidentTopology>
CellSetStructured<DIMENSION>::PrepareForInput(vtkm::cont::DeviceAdapterId,
                                              VisitTopology,
                                              IncidentTopology,
                                              vtkm::cont::Token&) const
{
  using ConnectivityType = ExecConnectivityType<VisitTopology, IncidentTopology>;
  return ConnectivityType(this->Structure);
}

template <vtkm::IdComponent DIMENSION>
void CellSetStructured<DIMENSION>::PrintSummary(std::ostream& out) const
{
  out << "  StructuredCellSet: " << std::endl;
  this->Structure.PrintSummary(out);
}
}
}
#endif
