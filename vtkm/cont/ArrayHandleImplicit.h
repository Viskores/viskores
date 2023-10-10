//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleImplicit_h
#define vtk_m_cont_ArrayHandleImplicit_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkmstd/is_trivial.h>

namespace vtkm
{

namespace internal
{

/// \brief An array portal that returns the result of a functor
///
/// This array portal is similar to an implicit array i.e an array that is
/// defined functionally rather than actually stored in memory. The array
/// comprises a functor that is called for each index.
///
/// The \c ArrayPortalImplicit is used in an ArrayHandle with an
/// \c StorageImplicit container.
///
template <class FunctorType_>
class VTKM_ALWAYS_EXPORT ArrayPortalImplicit
{
public:
  using FunctorType = FunctorType_;
  using ValueType = decltype(FunctorType{}(vtkm::Id{}));

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalImplicit()
    : Functor()
    , NumberOfValues(0)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalImplicit(FunctorType f, vtkm::Id numValues)
    : Functor(f)
    , NumberOfValues(numValues)
  {
  }

  VTKM_EXEC_CONT
  const FunctorType& GetFunctor() const { return this->Functor; }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return this->Functor(index); }

private:
  FunctorType Functor;
  vtkm::Id NumberOfValues;
};

} // namespace internal

namespace cont
{

/// \brief An implementation for read-only implicit arrays.
///
/// It is sometimes the case that you want VTK-m to operate on an array of
/// implicit values. That is, rather than store the data in an actual array, it
/// is gerenated on the fly by a function. This is handled in VTK-m by creating
/// an ArrayHandle in VTK-m with a StorageTagImplicit type of \c Storage. This
/// tag itself is templated to specify an ArrayPortal that generates the
/// desired values. An ArrayHandle created with this tag will raise an error on
/// any operation that tries to modify it.
///
template <class ArrayPortalType>
struct VTKM_ALWAYS_EXPORT StorageTagImplicit
{
  using PortalType = ArrayPortalType;
};

namespace internal
{

/// Given an array portal, returns the buffers for the `ArrayHandle` with a storage that
/// is (or is compatible with) a storage tag of `StorageTagImplicit<PortalType>`.
template <typename PortalType>
VTKM_CONT inline std::vector<vtkm::cont::internal::Buffer> PortalToArrayHandleImplicitBuffers(
  const PortalType& portal)
{
  std::vector<vtkm::cont::internal::Buffer> buffers(1);
  buffers[0].SetMetaData(portal);
  return buffers;
}

/// Given a functor and the number of values, returns the buffers for the `ArrayHandleImplicit`
/// for the given functor.
template <typename FunctorType>
VTKM_CONT inline std::vector<vtkm::cont::internal::Buffer> FunctorToArrayHandleImplicitBuffers(
  const FunctorType& functor,
  vtkm::Id numValues)
{
  return PortalToArrayHandleImplicitBuffers(
    vtkm::internal::ArrayPortalImplicit<FunctorType>(functor, numValues));
}

template <class ArrayPortalType>
struct VTKM_ALWAYS_EXPORT
  Storage<typename ArrayPortalType::ValueType, StorageTagImplicit<ArrayPortalType>>
{
  VTKM_IS_TRIVIALLY_COPYABLE(ArrayPortalType);

  VTKM_STORAGE_NO_RESIZE;
  VTKM_STORAGE_NO_WRITE_PORTAL;

  using ReadPortalType = ArrayPortalType;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return vtkm::cont::internal::PortalToArrayHandleImplicitBuffers(ArrayPortalType{});
  }

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>&)
  {
    return vtkm::VecFlat<typename ArrayPortalType::ValueType>::NUM_COMPONENTS;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<ArrayPortalType>().GetNumberOfValues();
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId,
    vtkm::cont::Token&)
  {
    return buffers[0].GetMetaData<ArrayPortalType>();
  }
};

} // namespace internal

namespace detail
{

/// A convenience class that provides a typedef to the appropriate tag for
/// a implicit array container.
template <typename FunctorType>
struct ArrayHandleImplicitTraits
{
  using ValueType = decltype(FunctorType{}(vtkm::Id{}));
  using PortalType = vtkm::internal::ArrayPortalImplicit<FunctorType>;
  using StorageTag = vtkm::cont::StorageTagImplicit<PortalType>;
  using Superclass = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
};

} // namespace detail

/// \brief An \c ArrayHandle that computes values on the fly.
///
/// \c ArrayHandleImplicit is a specialization of ArrayHandle.
/// It takes a user defined functor which is called with a given index value.
/// The functor returns the result of the functor as the value of this
/// array at that position.
///
template <class FunctorType>
class VTKM_ALWAYS_EXPORT ArrayHandleImplicit
  : public detail::ArrayHandleImplicitTraits<FunctorType>::Superclass
{
private:
  using ArrayTraits = typename detail::ArrayHandleImplicitTraits<FunctorType>;
  using PortalType = typename ArrayTraits::PortalType;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleImplicit,
                             (ArrayHandleImplicit<FunctorType>),
                             (typename ArrayTraits::Superclass));

  VTKM_CONT
  ArrayHandleImplicit(FunctorType functor, vtkm::Id length)
    : Superclass(internal::PortalToArrayHandleImplicitBuffers(PortalType(functor, length)))
  {
  }
};

/// make_ArrayHandleImplicit is convenience function to generate an
/// ArrayHandleImplicit.  It takes a functor and the virtual length of the
/// arry.

template <typename FunctorType>
VTKM_CONT vtkm::cont::ArrayHandleImplicit<FunctorType> make_ArrayHandleImplicit(FunctorType functor,
                                                                                vtkm::Id length)
{
  return ArrayHandleImplicit<FunctorType>(functor, length);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename Functor>
struct SerializableTypeString<vtkm::cont::ArrayHandleImplicit<Functor>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Implicit<" + SerializableTypeString<Functor>::Get() + ">";
    return name;
  }
};

template <typename Functor>
struct SerializableTypeString<vtkm::cont::ArrayHandle<
  typename vtkm::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalImplicit<Functor>>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleImplicit<Functor>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename Functor>
struct Serialization<vtkm::cont::ArrayHandleImplicit<Functor>>
{
private:
  using Type = vtkm::cont::ArrayHandleImplicit<Functor>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetNumberOfValues());
    vtkmdiy::save(bb, obj.ReadPortal().GetFunctor());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Id count = 0;
    vtkmdiy::load(bb, count);

    Functor functor;
    vtkmdiy::load(bb, functor);

    obj = vtkm::cont::make_ArrayHandleImplicit(functor, count);
  }
};

template <typename Functor>
struct Serialization<vtkm::cont::ArrayHandle<
  typename vtkm::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalImplicit<Functor>>>>
  : Serialization<vtkm::cont::ArrayHandleImplicit<Functor>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleImplicit_h
