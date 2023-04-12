//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleConcatenate_h
#define vtk_m_cont_ArrayHandleConcatenate_h

#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace internal
{

template <typename PortalType1, typename PortalType2>
class VTKM_ALWAYS_EXPORT ArrayPortalConcatenate
{
  using WritableP1 = vtkm::internal::PortalSupportsSets<PortalType1>;
  using WritableP2 = vtkm::internal::PortalSupportsSets<PortalType2>;
  using Writable = std::integral_constant<bool, WritableP1::value && WritableP2::value>;

public:
  using ValueType = typename PortalType1::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalConcatenate()
    : portal1()
    , portal2()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalConcatenate(const PortalType1& p1, const PortalType2& p2)
    : portal1(p1)
    , portal2(p2)
  {
  }

  // Copy constructor
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherP1, typename OtherP2>
  VTKM_EXEC_CONT ArrayPortalConcatenate(const ArrayPortalConcatenate<OtherP1, OtherP2>& src)
    : portal1(src.GetPortal1())
    , portal2(src.GetPortal2())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->portal1.GetNumberOfValues() + this->portal2.GetNumberOfValues();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      return this->portal1.Get(index);
    }
    else
    {
      return this->portal2.Get(index - this->portal1.GetNumberOfValues());
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      this->portal1.Set(index, value);
    }
    else
    {
      this->portal2.Set(index - this->portal1.GetNumberOfValues(), value);
    }
  }

  VTKM_EXEC_CONT
  const PortalType1& GetPortal1() const { return this->portal1; }

  VTKM_EXEC_CONT
  const PortalType2& GetPortal2() const { return this->portal2; }

private:
  PortalType1 portal1;
  PortalType2 portal2;
}; // class ArrayPortalConcatenate

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

template <typename StorageTag1, typename StorageTag2>
class VTKM_ALWAYS_EXPORT StorageTagConcatenate
{
};

namespace internal
{

template <typename T, typename ST1, typename ST2>
class Storage<T, StorageTagConcatenate<ST1, ST2>>
{
  using SourceStorage1 = vtkm::cont::internal::Storage<T, ST1>;
  using SourceStorage2 = vtkm::cont::internal::Storage<T, ST2>;

  using ArrayHandleType1 = vtkm::cont::ArrayHandle<T, ST1>;
  using ArrayHandleType2 = vtkm::cont::ArrayHandle<T, ST2>;

  struct Info
  {
    std::size_t NumBuffers1;
    std::size_t NumBuffers2;
  };

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> Buffers1(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + 1,
                                                     buffers.begin() + 1 + info.NumBuffers1);
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> Buffers2(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + 1 + info.NumBuffers1,
                                                     buffers.end());
  }

public:
  VTKM_STORAGE_NO_RESIZE;

  using ReadPortalType =
    vtkm::internal::ArrayPortalConcatenate<typename SourceStorage1::ReadPortalType,
                                           typename SourceStorage2::ReadPortalType>;
  using WritePortalType =
    vtkm::internal::ArrayPortalConcatenate<typename SourceStorage1::WritePortalType,
                                           typename SourceStorage2::WritePortalType>;

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return (SourceStorage1::GetNumberOfValues(Buffers1(buffers)) +
            SourceStorage2::GetNumberOfValues(Buffers2(buffers)));
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const T& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token)
  {
    vtkm::Id size1 = SourceStorage1::GetNumberOfValues(Buffers1(buffers));
    if ((startIndex < size1) && (endIndex <= size1))
    {
      SourceStorage1::Fill(Buffers1(buffers), fillValue, startIndex, endIndex, token);
    }
    else if (startIndex < size1) // && (endIndex > size1)
    {
      SourceStorage1::Fill(Buffers1(buffers), fillValue, startIndex, size1, token);
      SourceStorage2::Fill(Buffers2(buffers), fillValue, 0, endIndex - size1, token);
    }
    else // startIndex >= size1
    {
      SourceStorage2::Fill(
        Buffers2(buffers), fillValue, startIndex - size1, endIndex - size1, token);
    }
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(SourceStorage1::CreateReadPortal(Buffers1(buffers), device, token),
                          SourceStorage2::CreateReadPortal(Buffers2(buffers), device, token));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(SourceStorage1::CreateWritePortal(Buffers1(buffers), device, token),
                           SourceStorage2::CreateWritePortal(Buffers2(buffers), device, token));
  }

  VTKM_CONT static auto CreateBuffers(const ArrayHandleType1& array1 = ArrayHandleType1{},
                                      const ArrayHandleType2& array2 = ArrayHandleType2{})
    -> decltype(vtkm::cont::internal::CreateBuffers())
  {
    Info info;
    info.NumBuffers1 = array1.GetBuffers().size();
    info.NumBuffers2 = array2.GetBuffers().size();
    return vtkm::cont::internal::CreateBuffers(info, array1, array2);
  }

  VTKM_CONT static const ArrayHandleType1 GetArray1(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ArrayHandleType1(Buffers1(buffers));
  }

  VTKM_CONT static const ArrayHandleType2 GetArray2(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ArrayHandleType2(Buffers2(buffers));
  }
}; // class Storage

}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace cont
{

template <typename ArrayHandleType1, typename ArrayHandleType2>
class ArrayHandleConcatenate
  : public vtkm::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                                   StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                         typename ArrayHandleType2::StorageTag>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleConcatenate,
    (ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>),
    (vtkm::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                             StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                   typename ArrayHandleType2::StorageTag>>));

  VTKM_CONT
  ArrayHandleConcatenate(const ArrayHandleType1& array1, const ArrayHandleType2& array2)
    : Superclass(StorageType::CreateBuffers(array1, array2))
  {
  }
};

template <typename ArrayHandleType1, typename ArrayHandleType2>
VTKM_CONT ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2> make_ArrayHandleConcatenate(
  const ArrayHandleType1& array1,
  const ArrayHandleType2& array2)
{
  return ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>(array1, array2);
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

template <typename AH1, typename AH2>
struct SerializableTypeString<vtkm::cont::ArrayHandleConcatenate<AH1, AH2>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Concatenate<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConcatenate<ST1, ST2>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleConcatenate<vtkm::cont::ArrayHandle<T, ST1>,
                                                              vtkm::cont::ArrayHandle<T, ST2>>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2>
struct Serialization<vtkm::cont::ArrayHandleConcatenate<AH1, AH2>>
{
private:
  using Type = vtkm::cont::ArrayHandleConcatenate<AH1, AH2>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    vtkmdiy::save(bb, storage.GetArray1());
    vtkmdiy::save(bb, storage.GetArray2());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;

    vtkmdiy::load(bb, array1);
    vtkmdiy::load(bb, array2);

    obj = vtkm::cont::make_ArrayHandleConcatenate(array1, array2);
  }
};

template <typename T, typename ST1, typename ST2>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConcatenate<ST1, ST2>>>
  : Serialization<vtkm::cont::ArrayHandleConcatenate<vtkm::cont::ArrayHandle<T, ST1>,
                                                     vtkm::cont::ArrayHandle<T, ST2>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleConcatenate_h
