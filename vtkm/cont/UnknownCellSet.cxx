//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/cont/UncertainCellSet.h>

#include <sstream>

namespace
{

// Could potentially precompile more cell sets to serialze if that is useful.
using UnknownSerializationCellSets = VTKM_DEFAULT_CELL_SET_LIST;

}

namespace vtkm
{
namespace cont
{

vtkm::cont::UnknownCellSet UnknownCellSet::NewInstance() const
{
  UnknownCellSet newCellSet;
  if (this->Container)
  {
    newCellSet.Container = this->Container->NewInstance();
  }
  return newCellSet;
}

std::string UnknownCellSet::GetCellSetName() const
{
  if (this->Container)
  {
    return vtkm::cont::TypeToString(typeid(this->Container.get()));
  }
  else
  {
    return "";
  }
}

void UnknownCellSet::PrintSummary(std::ostream& os) const
{
  if (this->Container)
  {
    this->Container->PrintSummary(os);
  }
  else
  {
    os << " UnknownCellSet = nullptr\n";
  }
}

namespace internal
{

void ThrowCastAndCallException(const vtkm::cont::UnknownCellSet& ref, const std::type_info& type)
{
  std::ostringstream out;
  out << "Could not find appropriate cast for cell set in CastAndCall.\n"
         "CellSet: ";
  ref.PrintSummary(out);
  out << "TypeList: " << vtkm::cont::TypeToString(type) << "\n";
  throw vtkm::cont::ErrorBadType(out.str());
}

} // namespace internal

} // namespace vtkm::cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes

namespace vtkm
{
namespace cont
{

std::string SerializableTypeString<vtkm::cont::UnknownCellSet>::Get()
{
  return "UnknownCS";
}
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

void Serialization<vtkm::cont::UnknownCellSet>::save(BinaryBuffer& bb,
                                                     const vtkm::cont::UnknownCellSet& obj)
{
  vtkmdiy::save(bb, obj.ResetCellSetList<UnknownSerializationCellSets>());
}

void Serialization<vtkm::cont::UnknownCellSet>::load(BinaryBuffer& bb,
                                                     vtkm::cont::UnknownCellSet& obj)
{
  vtkm::cont::UncertainCellSet<UnknownSerializationCellSets> uncertainCellSet;
  vtkmdiy::load(bb, uncertainCellSet);
  obj = uncertainCellSet;
}

} // namespace mangled_diy_namespace
