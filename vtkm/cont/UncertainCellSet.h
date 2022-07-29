//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_UncertainCellSet_h
#define vtk_m_cont_UncertainCellSet_h

#include <vtkm/cont/UnknownCellSet.h>

namespace vtkm
{
namespace cont
{

/// \brief A `CellSet` of an uncertain type.
///
/// `UncertainCellSet` holds a `CellSet` object using runtime polymorphism to
/// manage different types. It behaves like its superclass, `UnknownCellSet`,
/// except that it also contains a template parameter that provides a
/// `vtkm::List` of potential cell set types.
///
/// These potental types come into play when the `CastAndCall` method is called
/// (or the `UncertainCellSet` is used in the `vtkm::cont::CastAndCall` function).
/// In this case, the `CastAndCall` will search for `CellSet`s of types that match
/// this list.
///
/// Both `UncertainCellSet` and `UnknownCellSet` have a method named
/// `ResetCellSetList` that redefines the list of potential cell sets by returning
/// a new `UncertainCellSet` containing the same `CellSet` but with the new cell
/// set type list.
///
template <typename CellSetList>
class VTKM_ALWAYS_EXPORT UncertainCellSet : public vtkm::cont::UnknownCellSet
{
  VTKM_IS_LIST(CellSetList);

  VTKM_STATIC_ASSERT_MSG((!std::is_same<CellSetList, vtkm::ListUniversal>::value),
                         "Cannot use vtkm::ListUniversal with UncertainCellSet.");

  using Superclass = UnknownCellSet;
  using Thisclass = UncertainCellSet<CellSetList>;

public:
  VTKM_CONT UncertainCellSet() = default;

  template <typename CellSetType>
  VTKM_CONT UncertainCellSet(const CellSetType& cellSet)
    : Superclass(cellSet)
  {
  }

  explicit VTKM_CONT UncertainCellSet(const vtkm::cont::UnknownCellSet& src)
    : Superclass(src)
  {
  }

  template <typename OtherCellSetList>
  explicit VTKM_CONT UncertainCellSet(const UncertainCellSet<OtherCellSetList>& src)
    : Superclass(src)
  {
  }

  /// \brief Create a new cell set of the same type as this.
  ///
  /// This method creates a new cell set that is the same type as this one and
  /// returns a new `UncertainCellSet` for it.
  ///
  VTKM_CONT Thisclass NewInstance() const { return Thisclass(this->Superclass::NewInstance()); }

  /// \brief Call a functor using the underlying cell set type.
  ///
  /// `CastAndCall` attempts to cast the held cell set to a specific type,
  /// and then calls the given functor with the cast cell set.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCall(Functor&& functor, Args&&... args) const
  {
    this->CastAndCallForTypes<CellSetList>(std::forward<Functor>(functor),
                                           std::forward<Args>(args)...);
  }
};

// Defined here to avoid circular dependencies between UnknownCellSet and UncertainCellSet.
template <typename NewCellSetList>
VTKM_CONT vtkm::cont::UncertainCellSet<NewCellSetList> UnknownCellSet::ResetCellSetList(
  NewCellSetList) const
{
  return vtkm::cont::UncertainCellSet<NewCellSetList>(*this);
}
template <typename NewCellSetList>
VTKM_CONT vtkm::cont::UncertainCellSet<NewCellSetList> UnknownCellSet::ResetCellSetList() const
{
  return vtkm::cont::UncertainCellSet<NewCellSetList>(*this);
}

namespace internal
{

template <typename CellSetList>
struct DynamicTransformTraits<vtkm::cont::UncertainCellSet<CellSetList>>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

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

template <typename CellSetList>
struct SerializableTypeString<vtkm::cont::UncertainCellSet<CellSetList>>
{
  static VTKM_CONT std::string Get()
  {
    return SerializableTypeString<vtkm::cont::UnknownCellSet>::Get();
  }
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

namespace internal
{

struct UncertainCellSetSerializeFunctor
{
  template <typename CellSetType>
  void operator()(const CellSetType& cs, BinaryBuffer& bb) const
  {
    vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<CellSetType>::Get());
    vtkmdiy::save(bb, cs);
  }
};

struct UncertainCellSetDeserializeFunctor
{
  template <typename CellSetType>
  void operator()(CellSetType,
                  vtkm::cont::UnknownCellSet& unknownCellSet,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    if (!success && (typeString == vtkm::cont::SerializableTypeString<CellSetType>::Get()))
    {
      CellSetType knownCellSet;
      vtkmdiy::load(bb, knownCellSet);
      unknownCellSet = knownCellSet;
      success = true;
    }
  }
};

} // internal

template <typename CellSetList>
struct Serialization<vtkm::cont::UncertainCellSet<CellSetList>>
{
  using Type = vtkm::cont::UncertainCellSet<CellSetList>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    obj.CastAndCall(internal::UncertainCellSetSerializeFunctor{}, bb);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    vtkmdiy::load(bb, typeString);

    bool success = false;
    vtkm::ListForEach(
      internal::UncertainCellSetDeserializeFunctor{}, CellSetList{}, obj, typeString, success, bb);

    if (!success)
    {
      throw vtkm::cont::ErrorBadType(
        "Error deserializing Unknown/UncertainCellSet. Message TypeString: " + typeString);
    }
  }
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

// Implement the deprecated functionality of DynamicCellSetBase, which was replaced
// by UnknownCellSet/UncertainCellSet. Everything below this line (up to the #endif
// for the include guard) can be deleted once the deprecated functionality is removed.

namespace vtkm
{
namespace cont
{

// This is a deprecated class. Don't warn about deprecation while implementing
// deprecated functionality.
VTKM_DEPRECATED_SUPPRESS_BEGIN

template <typename CellSetList>
class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.8,
                                         "Use vtkm::cont::UncertainCellSet.") DynamicCellSetBase
  : public vtkm::cont::UncertainCellSet<CellSetList>
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

inline DynamicCellSet::operator vtkm::cont::DynamicCellSetBase<VTKM_DEFAULT_CELL_SET_LIST>() const
{
  return vtkm::cont::DynamicCellSetBase<VTKM_DEFAULT_CELL_SET_LIST>{ *this };
}

namespace internal
{

template <typename CellSetList>
struct DynamicTransformTraits<vtkm::cont::DynamicCellSetBase<CellSetList>>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

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

VTKM_DEPRECATED_SUPPRESS_END

#endif //vtk_m_cont_UncertainCellSet_h
