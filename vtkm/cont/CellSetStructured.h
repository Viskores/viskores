//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/exec/ConnectivityStructured.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>

namespace vtkm
{
namespace cont
{

template <vtkm::IdComponent DIMENSION>
class VTKM_ALWAYS_EXPORT CellSetStructured final : public CellSet
{
private:
  using Thisclass = vtkm::cont::CellSetStructured<DIMENSION>;
  using InternalsType = vtkm::internal::ConnectivityStructuredInternals<DIMENSION>;

public:
  static const vtkm::IdComponent Dimension = DIMENSION;

  using SchedulingRangeType = typename InternalsType::SchedulingRangeType;

  vtkm::Id GetNumberOfCells() const override { return this->Structure.GetNumberOfCells(); }

  vtkm::Id GetNumberOfPoints() const override { return this->Structure.GetNumberOfPoints(); }

  vtkm::Id GetNumberOfFaces() const override { return -1; }

  vtkm::Id GetNumberOfEdges() const override { return -1; }

  // Since the entire topology is defined by by three integers, nothing to do here.
  void ReleaseResourcesExecution() override {}

  void SetPointDimensions(SchedulingRangeType dimensions)
  {
    this->Structure.SetPointDimensions(dimensions);
  }

  void SetGlobalPointIndexStart(SchedulingRangeType start)
  {
    this->Structure.SetGlobalPointIndexStart(start);
  }

  SchedulingRangeType GetPointDimensions() const { return this->Structure.GetPointDimensions(); }

  SchedulingRangeType GetCellDimensions() const { return this->Structure.GetCellDimensions(); }

  SchedulingRangeType GetGlobalPointIndexStart() const
  {
    return this->Structure.GetGlobalPointIndexStart();
  }

  vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id vtkmNotUsed(cellIndex) = 0) const override
  {
    return this->Structure.GetNumberOfPointsInCell();
  }

  vtkm::UInt8 GetCellShape(vtkm::Id vtkmNotUsed(cellIndex) = 0) const override
  {
    return static_cast<vtkm::UInt8>(this->Structure.GetCellShape());
  }

  void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const override
  {
    auto asVec = this->Structure.GetPointsOfCell(id);
    for (vtkm::IdComponent i = 0; i < InternalsType::NUM_POINTS_IN_CELL; ++i)
    {
      ptids[i] = asVec[i];
    }
  }

  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetStructured>();
  }

  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetStructured*>(src);
    if (!other)
    {
      throw vtkm::cont::ErrorBadType("CellSetStructured::DeepCopy types don't match");
    }

    this->Structure = other->Structure;
  }

  template <typename TopologyElement>
  SchedulingRangeType GetSchedulingRange(TopologyElement) const;

  template <typename DeviceAdapter, typename VisitTopology, typename IncidentTopology>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(VisitTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(IncidentTopology);
    using ExecObjectType =
      vtkm::exec::ConnectivityStructured<VisitTopology, IncidentTopology, Dimension>;
  };

  template <typename DeviceAdapter, typename VisitTopology, typename IncidentTopology>
  typename ExecutionTypes<DeviceAdapter, VisitTopology, IncidentTopology>::ExecObjectType
    PrepareForInput(DeviceAdapter, VisitTopology, IncidentTopology) const;

  void PrintSummary(std::ostream& out) const override;

  // Cannot use the default implementation of the destructor because the CUDA compiler
  // will attempt to create it for device, and then it will fail when it tries to call
  // the destructor of the superclass.
  ~CellSetStructured() override {}

  CellSetStructured() = default;

  CellSetStructured(const CellSetStructured& src)
    : CellSet()
    , Structure(src.Structure)
  {
  }

  CellSetStructured& operator=(const CellSetStructured& src)
  {
    this->Structure = src.Structure;
    return *this;
  }

  CellSetStructured(CellSetStructured&& src) noexcept : CellSet(),
                                                        Structure(std::move(src.Structure))
  {
  }

  CellSetStructured& operator=(CellSetStructured&& src) noexcept
  {
    this->Structure = std::move(src.Structure);
    return *this;
  }

private:
  InternalsType Structure;
};

#ifndef vtkm_cont_CellSetStructured_cxx
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetStructured<1>;
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetStructured<2>;
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetStructured<3>;
#endif
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <vtkm::IdComponent DIMENSION>
struct SerializableTypeString<vtkm::cont::CellSetStructured<DIMENSION>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Structured<" + std::to_string(DIMENSION) + ">";
    return name;
  }
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <vtkm::IdComponent DIMENSION>
struct Serialization<vtkm::cont::CellSetStructured<DIMENSION>>
{
private:
  using Type = vtkm::cont::CellSetStructured<DIMENSION>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    vtkmdiy::save(bb, cs.GetPointDimensions());
    vtkmdiy::save(bb, cs.GetGlobalPointIndexStart());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    typename Type::SchedulingRangeType dims, start;
    vtkmdiy::load(bb, dims);
    vtkmdiy::load(bb, start);

    cs = Type{};
    cs.SetPointDimensions(dims);
    cs.SetGlobalPointIndexStart(start);
  }
};

} // diy
/// @endcond SERIALIZATION

#include <vtkm/cont/CellSetStructured.hxx>

#endif //vtk_m_cont_CellSetStructured_h
