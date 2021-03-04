//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleXGCCoordinates_h
#define vtk_m_cont_ArrayHandleXGCCoordinates_h

#include <vtkm/Math.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/internal/IndicesExtrude.h>


namespace vtkm
{
namespace internal
{

template <typename PortalType>
struct VTKM_ALWAYS_EXPORT ArrayPortalXGCCoordinates
{
  using ValueType = vtkm::Vec<typename PortalType::ValueType, 3>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalXGCCoordinates()
    : Portal()
    , NumberOfPointsPerPlane(0)
    , NumberOfPlanes(0)
    , NumberOfPlanesOwned(0)
    , PlaneStartId(0)
    , UseCylindrical(false){};

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalXGCCoordinates(const PortalType& p,
                            vtkm::Id numOfPlanes,
                            vtkm::Id numOfPlanesOwned,
                            vtkm::Id planeStartId,
                            bool cylindrical = false)
    : Portal(p)
    , NumberOfPlanes(numOfPlanes)
    , NumberOfPlanesOwned(numOfPlanesOwned)
    , PlaneStartId(planeStartId)
    , UseCylindrical(cylindrical)
  {
    this->NumberOfPointsPerPlane = this->Portal.GetNumberOfValues() / 2;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return (this->NumberOfPointsPerPlane * static_cast<vtkm::Id>(NumberOfPlanesOwned));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    const vtkm::Id realIdx = ((index * 2) % this->Portal.GetNumberOfValues()) / 2;
    const vtkm::Id whichPlane = (index * 2) / this->Portal.GetNumberOfValues() + this->PlaneStartId;
    return this->Get(vtkm::Id2(realIdx, whichPlane));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id2 index) const
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

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Vec<ValueType, 6> GetWedge(const exec::IndicesExtrude& index) const
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

private:
  PortalType Portal;
  vtkm::Id NumberOfPointsPerPlane;
  vtkm::Id NumberOfPlanes;
  vtkm::Id NumberOfPlanesOwned;
  vtkm::Id PlaneStartId;
  bool UseCylindrical;
};

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{
struct VTKM_ALWAYS_EXPORT StorageTagXGCCoordinates
{
};

namespace internal
{

struct XGCCoordinatesMetaData
{
  vtkm::Id NumberOfPlanes = 0;
  vtkm::Id NumberOfPlanesOwned = 0;
  vtkm::Id PlaneStartId = -1;
  bool UseCylindrical = false;

  XGCCoordinatesMetaData() = default;

  XGCCoordinatesMetaData(vtkm::Id numberOfPlanes,
                         vtkm::Id numberOfPlanesOwned,
                         vtkm::Id planeStartId,
                         bool useCylindrical)
    : NumberOfPlanes(numberOfPlanes)
    , NumberOfPlanesOwned(numberOfPlanesOwned)
    , PlaneStartId(planeStartId)
    , UseCylindrical(useCylindrical)
  {
  }
};

namespace detail
{

template <typename T>
class XGCCoordinatesStorageImpl
{
  using SourceStorage = Storage<T, StorageTagBasic>; // only allow input AH to use StorageTagBasic
  using MetaData = XGCCoordinatesMetaData;

  static MetaData& GetMetaData(const vtkm::cont::internal::Buffer* buffers)
  {
    return buffers[0].GetMetaData<MetaData>();
  }

  // Used to skip the metadata buffer and return only actual data buffers
  template <typename Buffs>
  VTKM_CONT constexpr static Buffs* SourceBuffers(Buffs* buffers)
  {
    return buffers + 1;
  }

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalXGCCoordinates<typename SourceStorage::ReadPortalType>;

  VTKM_CONT constexpr static vtkm::IdComponent GetNumberOfBuffers()
  {
    return SourceStorage::GetNumberOfBuffers() + 1; // To account for metadata
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    return GetNumberOfValuesPerPlane(buffers) * GetNumberOfPlanesOwned(buffers);
  }

  VTKM_CONT static vtkm::Id GetNumberOfValuesPerPlane(const vtkm::cont::internal::Buffer* buffers)
  {
    return SourceStorage::GetNumberOfValues(SourceBuffers(buffers)) / 2;
  }

  VTKM_CONT static vtkm::Id GetNumberOfPlanes(const vtkm::cont::internal::Buffer* buffers)
  {
    return GetMetaData(buffers).NumberOfPlanes;
  }

  VTKM_CONT static vtkm::Id GetNumberOfPlanesOwned(const vtkm::cont::internal::Buffer* buffers)
  {
    return GetMetaData(buffers).NumberOfPlanesOwned;
  }

  VTKM_CONT static vtkm::Id GetPlaneStartId(const vtkm::cont::internal::Buffer* buffers)
  {
    return GetMetaData(buffers).PlaneStartId;
  }

  VTKM_CONT static bool GetUseCylindrical(const vtkm::cont::internal::Buffer* buffers)
  {
    return GetMetaData(buffers).UseCylindrical;
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                                   vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token)
  {
    return ReadPortalType(SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
                          GetNumberOfPlanes(buffers),
                          GetNumberOfPlanesOwned(buffers),
                          GetPlaneStartId(buffers),
                          GetUseCylindrical(buffers));
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    vtkm::cont::ArrayHandle<T> array,
    vtkm::Id numberOfPlanes,
    vtkm::Id numberOfPlanesOwned,
    vtkm::Id planeStartId,
    bool useCylindrical)
  {
    return vtkm::cont::internal::CreateBuffers(
      MetaData(numberOfPlanes, numberOfPlanesOwned, planeStartId, useCylindrical), array);
  }

  VTKM_CONT static vtkm::cont::ArrayHandle<T> GetArrayHandle(
    const vtkm::cont::internal::Buffer* buffers)
  {
    return vtkm::cont::ArrayHandle<T>(SourceBuffers(buffers));
  }
};

} // namespace detail

template <>
class Storage<vtkm::Vec3f_32, vtkm::cont::StorageTagXGCCoordinates>
  : public detail::XGCCoordinatesStorageImpl<vtkm::Float32>
{
public:
  VTKM_STORAGE_NO_RESIZE;
  VTKM_STORAGE_NO_WRITE_PORTAL;
};

template <>
class Storage<vtkm::Vec3f_64, vtkm::cont::StorageTagXGCCoordinates>
  : public detail::XGCCoordinatesStorageImpl<vtkm::Float64>
{
public:
  VTKM_STORAGE_NO_RESIZE;
  VTKM_STORAGE_NO_WRITE_PORTAL;
};

} // namespace internal

template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleXGCCoordinates
  : public vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagXGCCoordinates>
{
  using AHandleType = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>;
  using OriginalType = vtkm::cont::ArrayHandle<T>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleXGCCoordinates,
    (ArrayHandleXGCCoordinates<T>),
    (vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagXGCCoordinates>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleXGCCoordinates(const OriginalType& array,
                            vtkm::Id numberOfPlanes,
                            vtkm::Id numberOfPlanesOwned,
                            vtkm::Id planeStartId,
                            bool cylindrical)
    : Superclass(StorageType::CreateBuffers(array,
                                            numberOfPlanes,
                                            numberOfPlanesOwned,
                                            planeStartId,
                                            cylindrical))
  {
  }

  ~ArrayHandleXGCCoordinates() {}

  VTKM_CONT vtkm::Id GetNumberOfPlanes() const
  {
    return StorageType::GetNumberOfPlanes(this->GetBuffers());
  }

  VTKM_CONT vtkm::Id GetNumberOfPlanesOwned() const
  {
    return StorageType::GetNumberOfPlanesOwned(this->GetBuffers());
  }

  VTKM_CONT vtkm::Id GetPlaneStartId() const
  {
    return StorageType::GetPlaneStartId(this->GetBuffers());
  }

  VTKM_CONT bool GetUseCylindrical() const
  {
    return StorageType::GetUseCylindrical(this->GetBuffers());
  }

  VTKM_CONT vtkm::Id GetNumberOfPointsPerPlane() const
  {
    return StorageType::GetNumberOfValuesPerPlane(this->GetBuffers());
  }

  VTKM_CONT OriginalType GetArray() const
  {
    return StorageType::GetArrayHandle(this->GetBuffers());
  }
};

template <typename T>
vtkm::cont::ArrayHandleXGCCoordinates<T> make_ArrayHandleXGCCoordinates(
  const vtkm::cont::ArrayHandle<T>& arrHandle,
  vtkm::Id numberOfPlanesOwned,
  bool cylindrical,
  vtkm::Id numberOfPlanes = -1,
  vtkm::Id planeStartId = 0)
{
  if (numberOfPlanes == -1)
  {
    numberOfPlanes = numberOfPlanesOwned;
  }
  return ArrayHandleXGCCoordinates<T>(
    arrHandle, numberOfPlanes, numberOfPlanesOwned, planeStartId, cylindrical);
}

template <typename T>
vtkm::cont::ArrayHandleXGCCoordinates<T> make_ArrayHandleXGCCoordinates(
  const T* array,
  vtkm::Id length,
  vtkm::Id numberOfPlanesOwned,
  bool cylindrical,
  vtkm::Id numberOfPlanes = -1,
  vtkm::Id planeStartId = 0,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  if (numberOfPlanes == -1)
  {
    numberOfPlanes = numberOfPlanesOwned;
  }
  return ArrayHandleXGCCoordinates<T>(vtkm::cont::make_ArrayHandle(array, length, copy),
                                      numberOfPlanes,
                                      numberOfPlanesOwned,
                                      planeStartId,
                                      cylindrical);
}

// if all planes belong to a single partition, then numberOfPlanes and planeStartId not needed
template <typename T>
vtkm::cont::ArrayHandleXGCCoordinates<T> make_ArrayHandleXGCCoordinates(
  const std::vector<T>& array,
  vtkm::Id numberOfPlanesOwned,
  bool cylindrical,
  vtkm::Id numberOfPlanes = -1,
  vtkm::Id planeStartId = 0,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    if (numberOfPlanes == -1)
    {
      numberOfPlanes = numberOfPlanesOwned;
    }
    return make_ArrayHandleXGCCoordinates<T>(&array.front(),
                                             static_cast<vtkm::Id>(array.size()),
                                             numberOfPlanesOwned,
                                             cylindrical,
                                             numberOfPlanes,
                                             planeStartId,
                                             copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleXGCCoordinates<T>();
  }
}

}
} // end namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleXGCCoordinates<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_XGCCoordinates<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagXGCCoordinates>>
  : SerializableTypeString<vtkm::cont::ArrayHandleXGCCoordinates<T>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleXGCCoordinates<T>>
{
private:
  using Type = vtkm::cont::ArrayHandleXGCCoordinates<T>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    Type ah = obj;
    vtkmdiy::save(bb, ah.GetNumberOfPlanes());
    vtkmdiy::save(bb, ah.GetNumberOfPlanesOwned());
    vtkmdiy::save(bb, ah.GetPlaneStartId());
    vtkmdiy::save(bb, ah.GetUseCylindrical());
    vtkmdiy::save(bb, ah.GetArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& ah)
  {
    vtkm::Id numberOfPlanes;
    vtkm::Id numberOfPlanesOwned;
    vtkm::Id planeStartId;
    bool isCylindrical;
    vtkm::cont::ArrayHandle<T> array;

    vtkmdiy::load(bb, numberOfPlanes);
    vtkmdiy::load(bb, numberOfPlanesOwned);
    vtkmdiy::load(bb, planeStartId);
    vtkmdiy::load(bb, isCylindrical);
    vtkmdiy::load(bb, array);

    ah = vtkm::cont::make_ArrayHandleXGCCoordinates(
      array, numberOfPlanes, numberOfPlanesOwned, planeStartId, isCylindrical);
  }
};

template <typename T>
struct Serialization<vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagXGCCoordinates>>
  : Serialization<vtkm::cont::ArrayHandleXGCCoordinates<T>>
{
};
} // diy
/// @endcond SERIALIZATION
#endif
