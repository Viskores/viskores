//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>
#include <vtkm/exec/ConnectivityStructured.h>

namespace vtkm {
namespace cont {



template<vtkm::IdComponent DIMENSION>
class CellSetStructured : public CellSet
{
private:
  typedef vtkm::cont::CellSetStructured<DIMENSION> Thisclass;
  typedef vtkm::internal::ConnectivityStructuredInternals<DIMENSION>
      InternalsType;

public:
  static const vtkm::IdComponent Dimension=DIMENSION;

  typedef typename InternalsType::SchedulingRangeType SchedulingRangeType;

  VTKM_CONT_EXPORT
  CellSetStructured(const std::string &name = std::string())
    : CellSet(name,Dimension)
  {
  }

  VTKM_CONT_EXPORT
  CellSetStructured(const Thisclass &src)
    : CellSet(src), Structure(src.Structure)
  {  }

  VTKM_CONT_EXPORT
  Thisclass &operator=(const Thisclass &src)
  {
    this->CellSet::operator=(src);
    this->Structure = src.Structure;
    return *this;
  }

  virtual ~CellSetStructured() {  }

  virtual vtkm::Id GetNumberOfCells() const
  {
    return this->Structure.GetNumberOfCells();
  }

  virtual vtkm::Id GetNumberOfPoints() const
  {
    return this->Structure.GetNumberOfPoints();
  }

  void SetPointDimensions(SchedulingRangeType dimensions)
  {
    this->Structure.SetPointDimensions(dimensions);
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent
  GetNumberOfPointsInCell(vtkm::Id vtkmNotUsed(cellIndex)=0) const
  {
    return this->Structure.GetNumberOfPointsInCell();
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent GetCellShape() const
  {
    return this->Structure.GetCellShape();
  }

  template<typename TopologyElement>
  VTKM_CONT_EXPORT
  SchedulingRangeType GetSchedulingRange(TopologyElement) const {
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
    return this->Structure.GetSchedulingRange(TopologyElement());
  }

  template<typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);
    typedef vtkm::exec::ConnectivityStructured<FromTopology,ToTopology,Dimension> ExecObjectType;
  };

  template<typename DeviceAdapter, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<DeviceAdapter,FromTopology,ToTopology>::ExecObjectType
  PrepareForInput(DeviceAdapter, FromTopology, ToTopology) const
  {
    typedef typename
        ExecutionTypes<DeviceAdapter,FromTopology,ToTopology>::ExecObjectType
            ConnectivityType;
    return ConnectivityType(this->Structure);
  }

  virtual void PrintSummary(std::ostream &out) const
  {
      out << "  StructuredCellSet: " << this->GetName()
          << " dim= " << this->GetDimensionality() << std::endl;
      this->Structure.PrintSummary(out);
  }

private:
  InternalsType Structure;
};


}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetStructured_h
