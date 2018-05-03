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
#ifndef vtk_m_cont_DynamicCellSet_h
#define vtk_m_cont_DynamicCellSet_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetListTag.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/cont/internal/DynamicTransform.h>
#include <vtkm/cont/internal/SimplePolymorphicContainer.h>

namespace vtkm
{
namespace cont
{

// Forward declaration.
template <typename CellSetList>
class DynamicCellSetBase;

namespace detail
{

// One instance of a template class cannot access the private members of
// another instance of a template class. However, I want to be able to copy
// construct a DynamicCellSet from another DynamicCellSet of any other type.
// Since you cannot partially specialize friendship, use this accessor class to
// get at the internals for the copy constructor.
struct DynamicCellSetCopyHelper
{
  template <typename CellSetList>
  VTKM_CONT static const std::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase>&
  GetCellSetContainer(const vtkm::cont::DynamicCellSetBase<CellSetList>& src)
  {
    return src.CellSetContainer;
  }
};

// A simple function to downcast a CellSet encapsulated in a
// SimplePolymorphicContainerBase to the given subclass of CellSet. If the
// conversion cannot be done, nullptr is returned.
template <typename CellSetType>
VTKM_CONT CellSetType* DynamicCellSetTryCast(
  vtkm::cont::internal::SimplePolymorphicContainerBase* cellSetContainer)
{
  vtkm::cont::internal::SimplePolymorphicContainer<CellSetType>* downcastContainer =
    dynamic_cast<vtkm::cont::internal::SimplePolymorphicContainer<CellSetType>*>(cellSetContainer);
  if (downcastContainer != nullptr)
  {
    return &downcastContainer->Item;
  }
  else
  {
    return nullptr;
  }
}

template <typename CellSetType>
VTKM_CONT CellSetType* DynamicCellSetTryCast(
  const std::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase>& cellSetContainer)
{
  return detail::DynamicCellSetTryCast<CellSetType>(cellSetContainer.get());
}

} // namespace detail

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
/// matches one of the types specified by \c VTKM_DEFAULT_CELL_SET_LIST_TAG.
/// This list can be changed by using the \c ResetTypeList method. It is
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
  VTKM_IS_LIST_TAG(CellSetList);

public:
  VTKM_CONT
  DynamicCellSetBase() {}

  template <typename CellSetType>
  VTKM_CONT DynamicCellSetBase(const CellSetType& cellSet)
    : CellSetContainer(new vtkm::cont::internal::SimplePolymorphicContainer<CellSetType>(cellSet))
  {
    VTKM_IS_CELL_SET(CellSetType);
  }

  VTKM_CONT
  DynamicCellSetBase(const DynamicCellSetBase<CellSetList>& src)
    : CellSetContainer(src.CellSetContainer)
  {
  }

  template <typename OtherCellSetList>
  VTKM_CONT explicit DynamicCellSetBase(const DynamicCellSetBase<OtherCellSetList>& src)
    : CellSetContainer(detail::DynamicCellSetCopyHelper::GetCellSetContainer(src))
  {
  }

  VTKM_CONT
  ~DynamicCellSetBase() {}

  VTKM_CONT
  vtkm::cont::DynamicCellSetBase<CellSetList>& operator=(
    const vtkm::cont::DynamicCellSetBase<CellSetList>& src)
  {
    this->CellSetContainer = src.CellSetContainer;
    return *this;
  }

  /// Returns true if this cell set is of the provided type.
  ///
  template <typename CellSetType>
  VTKM_CONT bool IsType() const
  {
    return (detail::DynamicCellSetTryCast<CellSetType>(this->CellSetContainer) != nullptr);
  }

  /// Returns true if this cell set is the same (or equivalent) type as the
  /// object provided.
  ///
  template <typename CellSetType>
  VTKM_CONT bool IsSameType(const CellSetType&) const
  {
    return this->IsType<CellSetType>();
  }

  /// Returns the contained cell set as the abstract \c CellSet type.
  ///
  VTKM_CONT
  const vtkm::cont::CellSet& CastToBase() const
  {
    return *reinterpret_cast<const vtkm::cont::CellSet*>(this->CellSetContainer->GetVoidPointer());
  }

  /// Returns this cell set cast to the given \c CellSet type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType to check if
  /// the cast can happen.
  ///
  template <typename CellSetType>
  VTKM_CONT CellSetType& Cast() const
  {
    CellSetType* cellSetPointer =
      detail::DynamicCellSetTryCast<CellSetType>(this->CellSetContainer);
    if (cellSetPointer == nullptr)
    {
      throw vtkm::cont::ErrorBadType("Bad cast of dynamic cell set.");
    }
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
  /// CellSetListTag.h. Since C++ does not allow you to actually change the
  /// template arguments, this method returns a new dynamic cell setobject.
  /// This method is particularly useful to narrow down (or expand) the types
  /// when using a cell set of particular constraints.
  ///
  template <typename NewCellSetList>
  VTKM_CONT DynamicCellSetBase<NewCellSetList> ResetCellSetList(
    NewCellSetList = NewCellSetList()) const
  {
    VTKM_IS_LIST_TAG(NewCellSetList);
    return DynamicCellSetBase<NewCellSetList>(*this);
  }

  /// Attempts to cast the held cell set to a specific concrete type, then call
  /// the given functor with the cast cell set. The cell sets tried in the cast
  /// are those in the \c CellSetList template argument of the \c
  /// DynamicCellSetBase class (or \c VTKM_DEFAULT_CELL_SET_LIST_TAG for \c
  /// DynamicCellSet). You can use \c ResetCellSetList to get different
  /// behavior from \c CastAndCall.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCall(Functor&& f, Args&&...) const;

  /// \brief Create a new cell set of the same type as this cell set.
  ///
  /// This method creates a new cell setthat is the same type as this one and
  /// returns a new dynamic cell set for it. This method is convenient when
  /// creating output data sets that should be the same type as some input cell
  /// set.
  ///
  VTKM_CONT
  DynamicCellSetBase<CellSetList> NewInstance() const
  {
    DynamicCellSetBase<CellSetList> newCellSet;
    newCellSet.CellSetContainer = this->CellSetContainer->NewInstance();
    return newCellSet;
  }

  VTKM_CONT
  std::string GetName() const { return this->CastToBase().GetName(); }

  VTKM_CONT
  vtkm::Id GetNumberOfCells() const { return this->CastToBase().GetNumberOfCells(); }

  VTKM_CONT
  vtkm::Id GetNumberOfFaces() const { return this->CastToBase().GetNumberOfFaces(); }

  VTKM_CONT
  vtkm::Id GetNumberOfEdges() const { return this->CastToBase().GetNumberOfEdges(); }

  VTKM_CONT
  vtkm::Id GetNumberOfPoints() const { return this->CastToBase().GetNumberOfPoints(); }

  VTKM_CONT
  void PrintSummary(std::ostream& stream) const { return this->CastToBase().PrintSummary(stream); }

private:
  std::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase> CellSetContainer;

  friend struct detail::DynamicCellSetCopyHelper;
};

namespace detail
{

struct DynamicCellSetTry
{
  DynamicCellSetTry(
    const vtkm::cont::internal::SimplePolymorphicContainerBase* const cellSetContainer)
    : Container(cellSetContainer)
  {
  }

  template <typename CellSetType, typename Functor, typename... Args>
  void operator()(CellSetType, Functor&& f, bool& called, Args&&... args) const
  {
    using downcastType = const vtkm::cont::internal::SimplePolymorphicContainer<CellSetType>* const;
    if (!called)
    {
      downcastType downcastContainer = dynamic_cast<downcastType>(this->Container);
      if (downcastContainer)
      {
        f(downcastContainer->Item, std::forward<Args>(args)...);
        called = true;
      }
    }
  }

  const vtkm::cont::internal::SimplePolymorphicContainerBase* const Container;
};

} // namespace detail

template <typename CellSetList>
template <typename Functor, typename... Args>
VTKM_CONT void DynamicCellSetBase<CellSetList>::CastAndCall(Functor&& f, Args&&... args) const
{
  bool called = false;
  detail::DynamicCellSetTry tryCellSet(this->CellSetContainer.get());
  vtkm::ListForEach(
    tryCellSet, CellSetList{}, std::forward<Functor>(f), called, std::forward<Args>(args)...);
  if (!called)
  {
    throw vtkm::cont::ErrorBadValue("Could not find appropriate cast for cell set.");
  }
}

using DynamicCellSet = DynamicCellSetBase<VTKM_DEFAULT_CELL_SET_LIST_TAG>;

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

#endif //vtk_m_cont_DynamicCellSet_h
