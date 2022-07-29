//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_UnknownCellSet_h
#define vtk_m_cont_UnknownCellSet_h

#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/DefaultTypes.h>

#include <vtkm/Deprecated.h>

#include <vtkm/cont/vtkm_cont_export.h>

#include <memory>

namespace vtkm
{
namespace cont
{

// Forward declaration.
template <typename CellSetList>
class UncertainCellSet;

/// \brief A CellSet of an unknown type.
///
/// `UnknownCellSet` holds a `CellSet` object using runtime polymorphism to manage
/// the dynamic type rather than compile-time templates. This adds a programming
/// convenience that helps avoid a proliferation of templates.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in VTK-m, `UnknownCellSet` contains a method named `CastAndCallForTypes` that
/// determines the correct type from some known list of types. This mechanism is
/// used internally by VTK-m's worklet invocation mechanism to determine the type
/// when running algorithms.
///
/// If the `UnknownCellSet` is used in a context where the possible cell set types
/// can be whittled down to a finite list, you can specify lists of cell set types
/// using the `ResetCellSetList` method. This will convert this object to an
/// `UncertainCellSet` of the given types. In cases where a finite set of types
/// are needed but there is no subset, `VTKM_DEFAULT_CELL_SET_LIST`
///
class VTKM_CONT_EXPORT UnknownCellSet
{
  std::shared_ptr<vtkm::cont::CellSet> Container;

  void InitializeKnownOrUnknownCellSet(const UnknownCellSet& cellSet,
                                       std::true_type vtkmNotUsed(isUnknownCellSet))
  {
    *this = cellSet;
  }

  template <typename CellSetType>
  void InitializeKnownOrUnknownCellSet(const CellSetType& cellSet,
                                       std::false_type vtkmNotUsed(isUnknownCellSet))
  {
    VTKM_IS_CELL_SET(CellSetType);
    this->Container = std::shared_ptr<vtkm::cont::CellSet>(new CellSetType(cellSet));
  }

public:
  VTKM_CONT UnknownCellSet() = default;

  template <typename CellSetType>
  VTKM_CONT UnknownCellSet(const CellSetType& cellSet)
  {
    this->InitializeKnownOrUnknownCellSet(
      cellSet, typename std::is_base_of<UnknownCellSet, CellSetType>::type{});
  }

  /// \brief Returns whether a cell set is stored in this `UnknownCellSet`.
  ///
  /// If the `UnknownCellSet` is constructed without a `CellSet`, it will not
  /// have an underlying type, and therefore the operations will be invalid.
  ///
  VTKM_CONT bool IsValid() const { return static_cast<bool>(this->Container); }

  /// \brief Returns a pointer to the `CellSet` base class.
  ///
  VTKM_CONT vtkm::cont::CellSet* GetCellSetBase() { return this->Container.get(); }
  VTKM_CONT const vtkm::cont::CellSet* GetCellSetBase() const { return this->Container.get(); }

  /// \brief Create a new cell set of the same type as this cell set.
  ///
  /// This method creates a new cell set that is the same type as this one
  /// and returns a new `UnknownCellSet` for it. This method is convenient
  /// when creating output cell sets that should be the same type as the
  /// input cell set.
  ///
  VTKM_CONT UnknownCellSet NewInstance() const;

  /// \brief Returns the name of the cell set type stored in this class.
  ///
  /// Returns an empty string if no cell set is stored.
  ///
  VTKM_CONT std::string GetCellSetName() const;

  /// \brief Returns true if this cell set matches the `CellSetType` template argument.
  ///
  template <typename CellSetType>
  VTKM_CONT bool IsType() const
  {
    return (dynamic_cast<const CellSetType*>(this->Container.get()) != nullptr);
  }

  VTKM_CONT vtkm::Id GetNumberOfCells() const
  {
    return this->Container ? this->Container->GetNumberOfCells() : 0;
  }
  VTKM_CONT vtkm::Id GetNumberOfFaces() const
  {
    return this->Container ? this->Container->GetNumberOfFaces() : 0;
  }
  VTKM_CONT vtkm::Id GetNumberOfEdges() const
  {
    return this->Container ? this->Container->GetNumberOfEdges() : 0;
  }
  VTKM_CONT vtkm::Id GetNumberOfPoints() const
  {
    return this->Container ? this->Container->GetNumberOfPoints() : 0;
  }

  VTKM_CONT vtkm::UInt8 GetCellShape(vtkm::Id id) const
  {
    return this->GetCellSetBase()->GetCellShape(id);
  }
  VTKM_CONT vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id id) const
  {
    return this->GetCellSetBase()->GetNumberOfPointsInCell(id);
  }
  VTKM_CONT void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const
  {
    return this->GetCellSetBase()->GetCellPointIds(id, ptids);
  }

  VTKM_CONT void DeepCopyFrom(const CellSet* src) { this->GetCellSetBase()->DeepCopy(src); }

  VTKM_CONT void PrintSummary(std::ostream& os) const;

  VTKM_CONT void ReleaseResourcesExecution()
  {
    if (this->Container)
    {
      this->Container->ReleaseResourcesExecution();
    }
  }

  /// \brief Returns true if this cell set can be retrieved as the given type.
  ///
  /// This method will return true if calling `AsCellSet` of the given type will
  /// succeed. This result is similar to `IsType`, and if `IsType` returns true,
  /// then this will return true. However, this method will also return true for
  /// other types where automatic conversions are made.
  ///
  template <typename CellSetType>
  VTKM_CONT bool CanConvert() const
  {
    // TODO: Currently, these are the same. But in the future we expect to support
    // special CellSet types that can convert back and forth such as multiplexed
    // cell sets or a cell set that can hold structured grids of any dimension.
    return this->IsType<CellSetType>();
  }

  ///@{
  /// \brief Get the cell set as a known type.
  ///
  /// Returns this cell set cast appropriately and stored in the given `CellSet`
  /// type. Throws an `ErrorBadType` if the stored cell set cannot be stored in
  /// the given cell set type. Use the `CanConvert` method to determine if the
  /// cell set can be returned with the given type.
  ///
  template <typename CellSetType>
  VTKM_CONT void AsCellSet(CellSetType& cellSet) const
  {
    VTKM_IS_CELL_SET(CellSetType);
    CellSetType* cellSetPointer = dynamic_cast<CellSetType*>(this->Container.get());
    if (cellSetPointer == nullptr)
    {
      VTKM_LOG_CAST_FAIL(*this, CellSetType);
      throwFailedDynamicCast(this->GetCellSetName(), vtkm::cont::TypeToString(cellSet));
    }
    VTKM_LOG_CAST_SUCC(*this, *cellSetPointer);
    cellSet = *cellSetPointer;
  }

  template <typename CellSetType>
  VTKM_CONT CellSetType AsCellSet() const
  {
    CellSetType cellSet;
    this->AsCellSet(cellSet);
    return cellSet;
  }
  ///@}

  /// \brief Assigns potential cell set types.
  ///
  /// Calling this method will return an `UncertainCellSet` with the provided
  /// cell set list. The returned object will hold the same `CellSet`, but
  /// `CastAndCall`'s on the returned object will be constrained to the given
  /// types.
  ///
  // Defined in UncertainCellSet.h
  template <typename CellSetList>
  VTKM_CONT vtkm::cont::UncertainCellSet<CellSetList> ResetCellSetList(CellSetList) const;
  template <typename CellSetList>
  VTKM_CONT vtkm::cont::UncertainCellSet<CellSetList> ResetCellSetList() const;

  /// \brief Call a functor using the underlying cell set type.
  ///
  /// `CastAndCallForTypes` attemts to cast the held cell set to a specific type
  /// and then calls the given functor with the cast cell set. You must specify
  /// the `CellSetList` (in a `vtkm::List`) as a template argument.
  ///
  /// After the functor argument, you may add any number of arguments that will
  /// be passed to the functor after the converted cell set.
  ///
  template <typename CellSetList, typename Functor, typename... Args>
  VTKM_CONT void CastAndCallForTypes(Functor&& functor, Args&&... args) const;

  // Support for deprecated DynamicCellSet features

  template <typename CellSetType>
  VTKM_DEPRECATED(1.8, "Use CanConvert<decltype(cellset)>() (or IsType).")
  VTKM_CONT bool IsSameType(const CellSetType&) const
  {
    return this->IsType<CellSetType>();
  }

  template <typename CellSetType>
  VTKM_DEPRECATED(1.8, "Use AsCellSet<CellSetType>().")
  VTKM_CONT CellSetType& Cast() const
  {
    VTKM_IS_CELL_SET(CellSetType);
    CellSetType* cellSetPointer = dynamic_cast<CellSetType*>(this->Container.get());
    if (cellSetPointer == nullptr)
    {
      VTKM_LOG_CAST_FAIL(*this, CellSetType);
      throwFailedDynamicCast(this->GetCellSetName(), vtkm::cont::TypeToString<CellSetType>());
    }
    VTKM_LOG_CAST_SUCC(*this, *cellSetPointer);
    return *cellSetPointer;
  }

  template <typename CellSetType>
  VTKM_DEPRECATED(1.8, "Use AsCellSet(cellSet).")
  VTKM_CONT void CopyTo(CellSetType& cellSet) const
  {
    return this->AsCellSet(cellSet);
  }

  template <typename Functor, typename... Args>
  VTKM_DEPRECATED(1.8,
                  "Use the vtkm::cont::CastAndCall free function4 or use CastAndCallForTypes or "
                  "use ResetCellList and then CastAndCall.")
  VTKM_CONT void CastAndCall(Functor&& f, Args&&... args) const
  {
    this->CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>(std::forward<Functor>(f),
                                                          std::forward<Args>(args)...);
  }
};

//=============================================================================
// Free function casting helpers
// (Not sure if these should be deprecated.)

/// Returns true if `unknownCellSet` matches the type of `CellSetType`.
///
template <typename CellSetType>
VTKM_CONT inline bool IsType(const vtkm::cont::UnknownCellSet& unknownCellSet)
{
  return unknownCellSet.IsType<CellSetType>();
}

/// Returns `unknownCellSet` cast to the given `CellSet` type. Throws
/// `ErrorBadType` if the cast does not work. Use `IsType`
/// to check if the cast can happen.
///
template <typename CellSetType>
VTKM_CONT inline CellSetType Cast(const vtkm::cont::UnknownCellSet& unknownCellSet)
{
  return unknownCellSet.Cast<CellSetType>();
}

namespace internal
{

VTKM_CONT_EXPORT void ThrowCastAndCallException(const vtkm::cont::UnknownCellSet&,
                                                const std::type_info&);

template <>
struct DynamicTransformTraits<vtkm::cont::UnknownCellSet>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal

template <typename CellSetList, typename Functor, typename... Args>
VTKM_CONT void UnknownCellSet::CastAndCallForTypes(Functor&& functor, Args&&... args) const
{
  VTKM_IS_LIST(CellSetList);
  bool called = false;
  vtkm::ListForEach(
    [&](auto cellSet) {
      if (!called && this->CanConvert<decltype(cellSet)>())
      {
        called = true;
        this->AsCellSet(cellSet);
        VTKM_LOG_CAST_SUCC(*this, cellSet);

        // If you get a compile error here, it means that you have called CastAndCall for a
        // vtkm::cont::UnknownCellSet and the arguments of the functor do not match those
        // being passed. This is often because it is calling the functor with a CellSet
        // type that was not expected. Either add overloads to the functor to accept all
        // possible cell set types or constrain the types tried for the CastAndCall.
        functor(cellSet, args...);
      }
    },
    CellSetList{});

  if (!called)
  {
    VTKM_LOG_CAST_FAIL(*this, CellSetList);
    internal::ThrowCastAndCallException(*this, typeid(CellSetList));
  }
}

/// A specialization of `CastAndCall` for `UnknownCellSet`.
/// Since we have no hints on the types, use `VTKM_DEFAULT_CELL_SET_LIST`.
template <typename Functor, typename... Args>
void CastAndCall(const vtkm::cont::UnknownCellSet& cellSet, Functor&& f, Args&&... args)
{
  cellSet.CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>(std::forward<Functor>(f),
                                                          std::forward<Args>(args)...);
}

namespace internal
{

/// Checks to see if the given object is an unknown (or uncertain) cell set. It
/// resolves to either `std::true_type` or `std::false_type`.
///
template <typename T>
using UnknownCellSetCheck = typename std::is_base_of<vtkm::cont::UnknownCellSet, T>::type;

#define VTKM_IS_UNKNOWN_CELL_SET(T) \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::UnknownCellSetCheck<T>::value)

#define VTKM_IS_KNOWN_OR_UNKNOWN_CELL_SET(T)                                 \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::CellSetCheck<T>::type::value || \
                     ::vtkm::cont::internal::UnknownCellSetCheck<T>::value)

} // namespace internal

} // namespace vtkm::cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace vtkm
{
namespace cont
{

template <>
struct VTKM_CONT_EXPORT SerializableTypeString<vtkm::cont::UnknownCellSet>
{
  static VTKM_CONT std::string Get();
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

template <>
struct VTKM_CONT_EXPORT Serialization<vtkm::cont::UnknownCellSet>
{
public:
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::UnknownCellSet& obj);
  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::UnknownCellSet& obj);
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

// Implement the deprecated functionality of DynamicCellSet, which was replaced
// by UnknownCellSet/UncertainCellSet. Everything below this line (up to the #endif
// for the include guard) can be deleted once the deprecated functionality is removed.

// Headers originally included from DynamicCellSet.h but not UnknownCellSet.h
#include <vtkm/cont/CellSetList.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Logging.h>

namespace vtkm
{
namespace cont
{

// This is a deprecated class. Don't warn about deprecation while implementing
// deprecated functionality.
VTKM_DEPRECATED_SUPPRESS_BEGIN

// Forward declaration
template <typename CellSetList>
class DynamicCellSetBase;

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
struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.8, "Use vtkm::cont::UnknownCellSet.") DynamicCellSet
  : public vtkm::cont::UnknownCellSet
{
  using UnknownCellSet::UnknownCellSet;

  DynamicCellSet() = default;

  DynamicCellSet(const vtkm::cont::UnknownCellSet& src)
    : UnknownCellSet(src)
  {
  }

  operator vtkm::cont::DynamicCellSetBase<VTKM_DEFAULT_CELL_SET_LIST>() const;

  VTKM_CONT vtkm::cont::DynamicCellSet NewInstance() const
  {
    return vtkm::cont::DynamicCellSet(this->UnknownCellSet::NewInstance());
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

namespace internal
{

template <>
struct DynamicTransformTraits<vtkm::cont::DynamicCellSet>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

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

VTKM_DEPRECATED_SUPPRESS_END

// Include the implementation of UncertainCellSet. This should be included because there
// are methods in UnknownCellSet that produce objects of this type. It has to be included
// at the end to resolve the circular dependency.
#include <vtkm/cont/UncertainCellSet.h>

#endif //vtk_m_cont_UnknownCellSet_h
