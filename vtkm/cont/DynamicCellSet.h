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
#ifndef vtk_m_cont_DynamicCellSet_h
#define vtk_m_cont_DynamicCellSet_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetListTag.h>
#include <vtkm/cont/ErrorControlBadValue.h>

#include <vtkm/cont/internal/DynamicTransform.h>
#include <vtkm/cont/internal/SimplePolymorphicContainer.h>

VTKM_BOOST_PRE_INCLUDE
#include <boost/smart_ptr/shared_ptr.hpp>
VTKM_BOOST_POST_INCLUDE

namespace vtkm {
namespace cont {

// Forward declaration.
template<typename CellSetList>
class DynamicCellSetBase;

namespace detail {

// One instance of a template class cannot access the private members of
// another instance of a template class. However, I want to be able to copy
// construct a DynamicCellSet from another DynamicCellSet of any other type.
// Since you cannot partially specialize friendship, use this accessor class to
// get at the internals for the copy constructor.
struct DynamicCellSetCopyHelper {
  template<typename CellSetList>
  VTKM_CONT_EXPORT
  static
  boost::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase>
  GetCellSetContainer(const vtkm::cont::DynamicCellSetBase<CellSetList> &src)
  {
    return src.CellSetContainer;
  }
};

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
/// names \c DynamicCellSetBase, which is templated on the list of cell set
/// types. \c DynamicCellSet is really just a typedef of \c DynamicCellSetBase
/// with the default cell set list.
///
template<typename CellSetList>
class DynamicCellSetBase
{
  VTKM_IS_LIST_TAG(CellSetList);
public:
  VTKM_CONT_EXPORT
  DynamicCellSetBase() {  }

  template<typename CellSetType>
  VTKM_CONT_EXPORT
  DynamicCellSetBase(const CellSetType &cellSet)
    : CellSetContainer(
        new vtkm::cont::internal::SimplePolymorphicContainer<CellSetType>(
          cellSet))
  {
    VTKM_IS_CELL_SET(CellSetType);
  }

  VTKM_CONT_EXPORT
  DynamicCellSetBase(const DynamicCellSetBase<CellSetList> &src)
    : CellSetContainer(src.CellSetContainer) {  }

  friend class DynamicCellSetBase;

  template<typename OtherCellSetList>
  VTKM_CONT_EXPORT
  explicit
  DynamicCellSetBase(const DynamicCellSetBase<OtherCellSetList> &src)
    : CellSetContainer(
        detail::DynamicCellSetCopyHelper::GetCellSetContainer(src))
  {  }

  /// Returns true if this cell set is of the provided type.
  ///
  template<typename CellSetType>
  VTKM_CONT_EXPORT
  bool IsType(CellSetType = CellSetType()) const {
    return (this->TryCast<CellSetType>() != NULL);
  }

  /// Returns the contained cell set as the abstract \c CellSet type.
  ///
  VTKM_CONT_EXPORT
  const vtkm::cont::CellSet &GetCellSet() const {
    return *reinterpret_cast<const vtkm::cont::CellSet *>(
          this->CellSetContainer->GetVoidPointer());
  }

  /// Returns this cell set cast to a concrete \c CellSet object of the given
  /// type. Throws ErrorControlBadValue if the cast does not work. Use
  /// IsTypeAndStorage to check if the cast can happen.
  ///
  template<typename CellSetType>
  VTKM_CONT_EXPORT
  const CellSetType &CastTo(CellSetType = CellSetType()) const {
    const vtkm::cont::internal::SimplePolymorphicContainer<CellSetType> *
        container = this->TryCast<CellSetType>();
    if (container == NULL)
    {
      throw vtkm::cont::ErrorControlBadValue("Bad cast of dynamic cell set.");
    }
    return container->Item;
  }
  template<typename CellSetType>
  VTKM_CONT_EXPORT
  CellSetType &CastTo(CellSetType = CellSetType()) {
    vtkm::cont::internal::SimplePolymorphicContainer<CellSetType> *
        container = this->TryCast<CellSetType>();
    if (container == NULL)
    {
      throw vtkm::cont::ErrorControlBadValue("Bad cast of dynamic cell set.");
    }
    return container->Item;
  }

  /// Changes the cell set types to try casting to when resolving this dynamic
  /// cell set, which is specified with a list tag like those in
  /// CellSetListTag.h. Since C++ does not allow you to actually change the
  /// template arguments, this method returns a new dynamic cell setobject.
  /// This method is particularly useful to narrow down (or expand) the types
  /// when using a cell set of particular constraints.
  ///
  template<typename NewCellSetList>
  VTKM_CONT_EXPORT
  DynamicCellSetBase<NewCellSetList>
  ResetCellSetList(NewCellSetList = NewCellSetList()) const {
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
  template<typename Functor>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f) const;

  /// \brief Create a new cell set of the same type as this cell set.
  ///
  /// This method creates a new cell setthat is the same type as this one and
  /// returns a new dynamic cell set for it. This method is convenient when
  /// creating output data sets that should be the same type as some input cell
  /// set.
  ///
  VTKM_CONT_EXPORT
  DynamicCellSetBase<CellSetList> NewInstance() const
  {
    DynamicCellSetBase<CellSetList> newCellSet;
    newCellSet.CellSetContainer = this->CellSetContainer->NewInstance();
    return newCellSet;
  }

private:
  boost::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase>
      CellSetContainer;

  friend struct detail::DynamicCellSetCopyHelper;

  template<typename CellSetType>
  VTKM_CONT_EXPORT
  vtkm::cont::internal::SimplePolymorphicContainer<CellSetType> *
  TryCast() const {
    return
        dynamic_cast<
          vtkm::cont::internal::SimplePolymorphicContainer<CellSetType> *>(
            this->CellSetContainer.get());
  }
};

namespace detail {

template<typename Functor>
struct DynamicCellSetTryCellSet
{
  const vtkm::cont::CellSet *AbstractCellSet;
  const Functor &Function;
  bool FoundCast;

  VTKM_CONT_EXPORT
  DynamicCellSetTryCellSet(const vtkm::cont::CellSet &cellSet,
                           const Functor &f)
    : AbstractCellSet(&cellSet), Function(f), FoundCast(false) {  }

  template<typename CellSetType>
  VTKM_CONT_EXPORT
  void operator()(CellSetType) {
    if (!this->FoundCast)
    {
      const CellSetType *cellSet =
          dynamic_cast<const CellSetType *>(this->AbstractCellSet);
      if (cellSet != NULL)
      {
        this->Function(*cellSet);
        this->FoundCast = true;
      }
    }
  }
};

template<typename Functor, typename CellSetTypeList>
VTKM_CONT_EXPORT
void CastAndCallCellSet(
    const Functor &f,
    const vtkm::cont::DynamicCellSetBase<CellSetTypeList> &cellSet)
{
  typedef detail::DynamicCellSetTryCellSet<Functor> TryCellSetType;
  TryCellSetType tryCellSet = TryCellSetType(cellSet.GetCellSet(), f);
  vtkm::ListForEach(tryCellSet, CellSetTypeList());
  if (!tryCellSet.FoundCast)
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Could not find appropriate cast for cell set.");
  }
}

} // namespace detail

template<typename CellSetList>
template<typename Functor>
VTKM_CONT_EXPORT
void DynamicCellSetBase<CellSetList>::CastAndCall(const Functor &f) const
{
  detail::CastAndCallCellSet(f, *this);
}

typedef DynamicCellSetBase<vtkm::cont::CellSetListTagCommon> DynamicCellSet;

namespace internal {

template<typename CellSetList>
struct DynamicTransformTraits<
    vtkm::cont::DynamicCellSetBase<CellSetList> >
{
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_DynamicCellSet_h
