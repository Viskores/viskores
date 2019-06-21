
#ifndef vtk_m_internal_ArrayPortalExtrude_h
#define vtk_m_internal_ArrayPortalExtrude_h

#include <vtkm/internal/IndicesExtrude.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorInternal.h>

#include <vtkm/BaseComponent.h>
#include <vtkm/cont/StorageExtrude.h>

namespace vtkm
{
namespace exec
{

template <typename PortalType>
struct VTKM_ALWAYS_EXPORT ArrayPortalExtrude
{
  using ValueType = vtkm::Vec<typename PortalType::ValueType, 3>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalExtrude()
    : Portal()
    , NumberOfValues(0)
    , NumberOfPlanes(0)
    , UseCylindrical(false){};

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalExtrude(const PortalType& p,
                     vtkm::Int32 numOfValues,
                     vtkm::Int32 numOfPlanes,
                     bool cylindrical = false)
    : Portal(p)
    , NumberOfValues(numOfValues)
    , NumberOfPlanes(numOfPlanes)
    , UseCylindrical(cylindrical)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return ((NumberOfValues / 2) * static_cast<vtkm::Id>(NumberOfPlanes));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id2 index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const {}

  PortalType Portal;
  vtkm::Int32 NumberOfValues;
  vtkm::Int32 NumberOfPlanes;
  bool UseCylindrical;
};
template <typename PortalType>
typename ArrayPortalExtrude<PortalType>::ValueType
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::Get(vtkm::Id index) const
{
  using CompType = typename ValueType::ComponentType;

  const vtkm::Id realIdx = (index * 2) % this->NumberOfValues;
  const vtkm::Id whichPlane = (index * 2) / this->NumberOfValues;
  const auto phi = static_cast<CompType>(whichPlane * (vtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
  }
}

template <typename PortalType>
typename ArrayPortalExtrude<PortalType>::ValueType
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::Get(vtkm::Id2 index) const
{
  using CompType = typename ValueType::ComponentType;

  const vtkm::Id realIdx = (index[0] * 2);
  const vtkm::Id whichPlane = index[1];
  const auto phi = static_cast<CompType>(whichPlane * (vtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
  }
}

template <typename PortalType>
vtkm::Vec<typename ArrayPortalExtrude<PortalType>::ValueType, 6>
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::GetWedge(const IndicesExtrude& index) const
{
  using CompType = typename ValueType::ComponentType;

  vtkm::Vec<ValueType, 6> result;
  for (int j = 0; j < 2; ++j)
  {
    const auto phi =
      static_cast<CompType>(index.Planes[j] * (vtkm::TwoPi() / this->NumberOfPlanes));
    for (int i = 0; i < 3; ++i)
    {
      const vtkm::Id realIdx = index.PointIds[j][i] * 2;
      auto r = this->Portal.Get(realIdx);
      auto z = this->Portal.Get(realIdx + 1);
      result[3 * j + i] = this->UseCylindrical
        ? ValueType(r, phi, z)
        : ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
    }
  }

  return result;
}
}
}


#endif
