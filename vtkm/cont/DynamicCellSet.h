//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DynamicCellSet_h
#define vtk_m_cont_DynamicCellSet_h

#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetList.h>
#include <vtkm/cont/DefaultTypes.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/UncertainCellSet.h>

namespace vtkm
{
namespace cont
{

/// \brief Holds a cell set without having to specify concrete type.
///
/// \c DynamicCellSet holds a \c CellSet object using runtime polymorphism to
/// manage different subclass types and template parameters of the subclasses
/// rather than compile-time templates. This adds a programming convenience
/// that helps avoid a proliferation of templates. It also provides the
/// management necessary to interface VTK-m with data sources where types will
/// not be known until runtime.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in VTK-m, \c DynamicCellSet contains a method named \c CastAndCall that
/// will determine the correct type from some known list of cell set types.
/// This mechanism is used internally by VTK-m's worklet invocation mechanism
/// to determine the type when running algorithms.
///
/// By default, \c DynamicCellSet will assume that the value type in the array
/// matches one of the types specified by \c VTKM_DEFAULT_CELL_SET_LIST.
/// This list can be changed by using the \c ResetCellSetList method. It is
/// worthwhile to match these lists closely to the possible types that might be
/// used. If a type is missing you will get a runtime error. If there are more
/// types than necessary, then the template mechanism will create a lot of
/// object code that is never used, and keep in mind that the number of
/// combinations grows exponentially when using multiple \c Dynamic* objects.
///
/// The actual implementation of \c DynamicCellSet is in a templated class
/// named \c DynamicCellSetBase, which is templated on the list of cell set
/// types. \c DynamicCellSet is really just a typedef of \c DynamicCellSetBase
/// with the default cell set list.
///
template <typename CellSetList>
class VTKM_ALWAYS_EXPORT DynamicCellSetBase : public vtkm::cont::UncertainCellSet<CellSetList>
{
  using Superclass = vtkm::cont::UncertainCellSet<CellSetList>;

public:
  using Superclass::Superclass;

  VTKM_CONT DynamicCellSetBase<CellSetList> NewInstance() const
  {
    return DynamicCellSetBase<CellSetList>(this->Superclass::NewInstance());
  }

  template <typename NewCellSetList>
  VTKM_CONT vtkm::cont::DynamicCellSetBase<NewCellSetList> ResetCellSetList(NewCellSetList) const
  {
    return vtkm::cont::DynamicCellSetBase<NewCellSetList>(*this);
  }
  template <typename NewCellSetList>
  VTKM_CONT vtkm::cont::DynamicCellSetBase<NewCellSetList> ResetCellSetList() const
  {
    return vtkm::cont::DynamicCellSetBase<NewCellSetList>(*this);
  }
};

using DynamicCellSet = DynamicCellSetBase<VTKM_DEFAULT_CELL_SET_LIST>;

namespace internal
{

template <typename CellSetList>
struct DynamicTransformTraits<vtkm::cont::DynamicCellSetBase<CellSetList>>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal

namespace internal
{

/// Checks to see if the given object is a dynamic cell set. It contains a
/// typedef named \c type that is either std::true_type or std::false_type.
/// Both of these have a typedef named value with the respective boolean value.
///
template <typename T>
struct DynamicCellSetCheck
{
  using type = vtkm::cont::internal::UnknownCellSetCheck<T>;
};

#define VTKM_IS_DYNAMIC_CELL_SET(T) \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::DynamicCellSetCheck<T>::type::value)

#define VTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(T)                                \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::CellSetCheck<T>::type::value || \
                     ::vtkm::cont::internal::DynamicCellSetCheck<T>::type::value)

} // namespace internal
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

namespace internal
{

struct DynamicCellSetSerializeFunctor
{
  template <typename CellSetType>
  void operator()(const CellSetType& cs, BinaryBuffer& bb) const
  {
    vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<CellSetType>::Get());
    vtkmdiy::save(bb, cs);
  }
};

template <typename CellSetTypes>
struct DynamicCellSetDeserializeFunctor
{
  template <typename CellSetType>
  void operator()(CellSetType,
                  vtkm::cont::DynamicCellSetBase<CellSetTypes>& dh,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    if (!success && (typeString == vtkm::cont::SerializableTypeString<CellSetType>::Get()))
    {
      CellSetType cs;
      vtkmdiy::load(bb, cs);
      dh = vtkm::cont::DynamicCellSetBase<CellSetTypes>(cs);
      success = true;
    }
  }
};

} // internal

template <typename CellSetTypes>
struct Serialization<vtkm::cont::DynamicCellSetBase<CellSetTypes>>
{
private:
  using Type = vtkm::cont::DynamicCellSetBase<CellSetTypes>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    obj.CastAndCall(internal::DynamicCellSetSerializeFunctor{}, bb);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    vtkmdiy::load(bb, typeString);

    bool success = false;
    vtkm::ListForEach(internal::DynamicCellSetDeserializeFunctor<CellSetTypes>{},
                      CellSetTypes{},
                      obj,
                      typeString,
                      success,
                      bb);

    if (!success)
    {
      throw vtkm::cont::ErrorBadType("Error deserializing DynamicCellSet. Message TypeString: " +
                                     typeString);
    }
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_DynamicCellSet_h
