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
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Logging.h>

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
class VTKM_ALWAYS_EXPORT DynamicCellSetBase
{
  VTKM_IS_LIST(CellSetList);

public:
  VTKM_CONT
  DynamicCellSetBase() = default;

  template <typename CellSetType>
  VTKM_CONT DynamicCellSetBase(const CellSetType& cellSet)
    : CellSet(std::make_shared<CellSetType>(cellSet))
  {
    VTKM_IS_CELL_SET(CellSetType);
  }

  template <typename OtherCellSetList>
  VTKM_CONT explicit DynamicCellSetBase(const DynamicCellSetBase<OtherCellSetList>& src)
    : CellSet(src.CellSet)
  {
  }

  VTKM_CONT explicit DynamicCellSetBase(const std::shared_ptr<vtkm::cont::CellSet>& cs)
    : CellSet(cs)
  {
  }

  /// Returns true if this cell set is of the provided type.
  ///
  template <typename CellSetType>
  VTKM_CONT bool IsType() const
  {
    return (dynamic_cast<CellSetType*>(this->CellSet.get()) != nullptr);
  }

  /// Returns true if this cell set is the same (or equivalent) type as the
  /// object provided.
  ///
  template <typename CellSetType>
  VTKM_CONT bool IsSameType(const CellSetType&) const
  {
    return this->IsType<CellSetType>();
  }

  /// Returns this cell set cast to the given \c CellSet type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType to check if
  /// the cast can happen.
  ///
  template <typename CellSetType>
  VTKM_CONT CellSetType& Cast() const
  {
    auto cellSetPointer = dynamic_cast<CellSetType*>(this->CellSet.get());
    if (cellSetPointer == nullptr)
    {
      VTKM_LOG_CAST_FAIL(*this, CellSetType);
      throw vtkm::cont::ErrorBadType("Bad cast of dynamic cell set.");
    }
    VTKM_LOG_CAST_SUCC(*this, *cellSetPointer);
    return *cellSetPointer;
  }

  /// Given a reference to a concrete \c CellSet object, attempt to downcast
  /// the contain cell set to the provided type and copy into the given \c
  /// CellSet object. Throws \c ErrorBadType if the cast does not work.
  /// Use \c IsType to check if the cast can happen.
  ///
  /// Note that this is a shallow copy. Any data in associated arrays are not
  /// copied.
  ///
  template <typename CellSetType>
  VTKM_CONT void CopyTo(CellSetType& cellSet) const
  {
    cellSet = this->Cast<CellSetType>();
  }

  /// Changes the cell set types to try casting to when resolving this dynamic
  /// cell set, which is specified with a list tag like those in
  /// CellSetList.h. Since C++ does not allow you to actually change the
  /// template arguments, this method returns a new dynamic cell setobject.
  /// This method is particularly useful to narrow down (or expand) the types
  /// when using a cell set of particular constraints.
  ///
  template <typename NewCellSetList>
  VTKM_CONT DynamicCellSetBase<NewCellSetList> ResetCellSetList(
    NewCellSetList = NewCellSetList()) const
  {
    VTKM_IS_LIST(NewCellSetList);
    return DynamicCellSetBase<NewCellSetList>(*this);
  }

  /// Attempts to cast the held cell set to a specific concrete type, then call
  /// the given functor with the cast cell set. The cell sets tried in the cast
  /// are those in the \c CellSetList template argument of the \c
  /// DynamicCellSetBase class (or \c VTKM_DEFAULT_CELL_SET_LIST for \c
  /// DynamicCellSet). You can use \c ResetCellSetList to get different
  /// behavior from \c CastAndCall.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCall(Functor&& f, Args&&...) const;

  /// \brief Create a new cell set of the same type as this cell set.
  ///
  /// This method creates a new cell set that is the same type as this one and
  /// returns a new dynamic cell set for it. This method is convenient when
  /// creating output data sets that should be the same type as some input cell
  /// set.
  ///
  VTKM_CONT
  DynamicCellSetBase<CellSetList> NewInstance() const
  {
    DynamicCellSetBase<CellSetList> newCellSet;
    if (this->CellSet)
    {
      newCellSet.CellSet = this->CellSet->NewInstance();
    }
    return newCellSet;
  }

  VTKM_CONT
  vtkm::cont::CellSet* GetCellSetBase() { return this->CellSet.get(); }

  VTKM_CONT
  const vtkm::cont::CellSet* GetCellSetBase() const { return this->CellSet.get(); }

  VTKM_CONT
  vtkm::Id GetNumberOfCells() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfCells() : 0;
  }

  VTKM_CONT
  vtkm::Id GetNumberOfFaces() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfFaces() : 0;
  }

  VTKM_CONT
  vtkm::Id GetNumberOfEdges() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfEdges() : 0;
  }

  VTKM_CONT
  vtkm::Id GetNumberOfPoints() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfPoints() : 0;
  }

  VTKM_CONT
  void ReleaseResourcesExecution()
  {
    if (this->CellSet)
    {
      this->CellSet->ReleaseResourcesExecution();
    }
  }

  VTKM_CONT
  void PrintSummary(std::ostream& stream) const
  {
    if (this->CellSet)
    {
      this->CellSet->PrintSummary(stream);
    }
    else
    {
      stream << " DynamicCellSet = nullptr" << std::endl;
    }
  }

private:
  std::shared_ptr<vtkm::cont::CellSet> CellSet;

  template <typename>
  friend class DynamicCellSetBase;
};

//=============================================================================
// Free function casting helpers

/// Returns true if \c dynamicCellSet matches the type of CellSetType.
///
template <typename CellSetType, typename Ts>
VTKM_CONT inline bool IsType(const vtkm::cont::DynamicCellSetBase<Ts>& dynamicCellSet)
{
  return dynamicCellSet.template IsType<CellSetType>();
}

/// Returns \c dynamicCellSet cast to the given \c CellSet type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename CellSetType, typename Ts>
VTKM_CONT inline CellSetType Cast(const vtkm::cont::DynamicCellSetBase<Ts>& dynamicCellSet)
{
  return dynamicCellSet.template Cast<CellSetType>();
}

namespace detail
{

struct DynamicCellSetTry
{
  DynamicCellSetTry(const vtkm::cont::CellSet* const cellSetBase)
    : CellSetBase(cellSetBase)
  {
  }

  template <typename CellSetType, typename Functor, typename... Args>
  void operator()(CellSetType, Functor&& f, bool& called, Args&&... args) const
  {
    if (!called)
    {
      auto* cellset = dynamic_cast<const CellSetType*>(this->CellSetBase);
      if (cellset)
      {
        VTKM_LOG_CAST_SUCC(*this->CellSetBase, *cellset);
        f(*cellset, std::forward<Args>(args)...);
        called = true;
      }
    }
  }

  const vtkm::cont::CellSet* const CellSetBase;
};

} // namespace detail

template <typename CellSetList>
template <typename Functor, typename... Args>
VTKM_CONT void DynamicCellSetBase<CellSetList>::CastAndCall(Functor&& f, Args&&... args) const
{
  bool called = false;
  detail::DynamicCellSetTry tryCellSet(this->CellSet.get());
  vtkm::ListForEach(
    tryCellSet, CellSetList{}, std::forward<Functor>(f), called, std::forward<Args>(args)...);
  if (!called)
  {
    VTKM_LOG_CAST_FAIL(*this, CellSetList);
    throw vtkm::cont::ErrorBadValue("Could not find appropriate cast for cell set.");
  }
}

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
  using type = std::false_type;
};

template <typename CellSetList>
struct DynamicCellSetCheck<vtkm::cont::DynamicCellSetBase<CellSetList>>
{
  using type = std::true_type;
};

#define VTKM_IS_DYNAMIC_CELL_SET(T)                                                                \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::DynamicCellSetCheck<T>::type::value)

#define VTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(T)                                                      \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::CellSetCheck<T>::type::value ||                       \
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
