//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#ifndef viskores_interop_anari_ANARIMapper_h
#define viskores_interop_anari_ANARIMapper_h

// viskores
#include <viskores/Assert.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/Token.h>
#include <viskores/interop/anari/ANARIActor.h>

#include <viskores/interop/anari/viskores_anari_export.h>

// std
#include <cstdint>
#include <memory>

namespace viskores
{
namespace interop
{
namespace anari
{

inline void NoopANARIDeleter(const void*, const void*) {}

namespace detail
{

/// Keeps a Viskores array's host memory alive and read-locked for as long as
/// the ANARI array that shares it.
template <typename T>
struct ANARIArrayHostMemory
{
  explicit ANARIArrayHostMemory(const viskores::cont::ArrayHandle<T>& array)
    : Array(array)
  {
  }

  viskores::cont::ArrayHandle<T> Array;
  viskores::cont::Token Token;
};

template <typename T>
void DeleteANARIArrayHostMemory(const void* userData, const void*)
{
  delete static_cast<const ANARIArrayHostMemory<T>*>(userData);
}

/// Pin the host memory of `array` and pass it to `newArray(appMemory, deleter, userData)`.
/// The device owns the pin from then on and drops it when it destroys the array.
template <typename T, typename NewArrayFunctor>
auto MakeANARIArrayWithHostMemory(const viskores::cont::ArrayHandle<T>& array,
                                  anari_cpp::DataType elementType,
                                  std::uint64_t numItems,
                                  NewArrayFunctor&& newArray)
  -> decltype(newArray(nullptr, nullptr, nullptr))
{
  if (numItems == 0)
    return nullptr;

  (void)elementType; // only read by the assert
  VISKORES_ASSERT(numItems * anari_cpp::sizeOf(elementType) <=
                  static_cast<std::uint64_t>(array.GetNumberOfValues()) * sizeof(T));

  auto holder = std::make_unique<ANARIArrayHostMemory<T>>(array);
  const void* memory = holder->Array.GetBuffers()[0].ReadPointerHost(holder->Token);
  if (!memory)
    return nullptr;

  auto anariArray = newArray(memory, &DeleteANARIArrayHostMemory<T>, holder.get());
  if (anariArray)
    holder.release(); // the device calls the deleter when it destroys the array
  return anariArray;
}

} // namespace detail

/// @brief Create an ANARI 1D array that shares the host memory of a Viskores array.
///
/// The ANARI array keeps the Viskores array's memory alive, and blocks writes to it,
/// until the device destroys the ANARI array. Handles that hold the array stay valid
/// after the `ANARIMapper` that made them is gone. Returns null if there is nothing
/// to share.
///
/// @param elementType The ANARI type of each element.
/// @param numItems The number of `elementType` elements to share, which must fit in
/// `array`.
template <typename T>
anari_cpp::Array1D MakeANARIArray1D(anari_cpp::Device device,
                                    const viskores::cont::ArrayHandle<T>& array,
                                    anari_cpp::DataType elementType,
                                    std::uint64_t numItems)
{
  return detail::MakeANARIArrayWithHostMemory(
    array,
    elementType,
    numItems,
    [&](const void* memory, ANARIMemoryDeleter deleter, const void* userData)
    { return anariNewArray1D(device, memory, deleter, userData, elementType, numItems); });
}

/// @brief Create an ANARI 1D array with one element per value of a Viskores array.
///
/// See the overload above for how the memory is shared.
template <typename T>
anari_cpp::Array1D MakeANARIArray1D(anari_cpp::Device device,
                                    const viskores::cont::ArrayHandle<T>& array)
{
  return MakeANARIArray1D(device,
                          array,
                          ::anari::ANARITypeFor<T>::value,
                          static_cast<std::uint64_t>(array.GetNumberOfValues()));
}

/// @brief Create an ANARI 3D array of `numItems1 * numItems2 * numItems3` values that
/// shares the host memory of a Viskores array.
///
/// See `MakeANARIArray1D` for how the memory is shared.
template <typename T>
anari_cpp::Array3D MakeANARIArray3D(anari_cpp::Device device,
                                    const viskores::cont::ArrayHandle<T>& array,
                                    std::uint64_t numItems1,
                                    std::uint64_t numItems2,
                                    std::uint64_t numItems3)
{
  const auto elementType = ::anari::ANARITypeFor<T>::value;
  return detail::MakeANARIArrayWithHostMemory(
    array,
    elementType,
    numItems1 * numItems2 * numItems3,
    [&](const void* memory, ANARIMemoryDeleter deleter, const void* userData)
    {
      return anariNewArray3D(
        device, memory, deleter, userData, elementType, numItems1, numItems2, numItems3);
    });
}

/// @brief This is the base class used for all ANARI mappers.
///
/// This class implements shared functionality of all ANARI mappers. All ANARI
/// object handle lifetimes are tied to the lifetime of the mapper, including
/// the device.  Applications are not intended to ever release handles received
/// from the mapper, unless they manually retain the handle. A retained handle
/// stays valid after the mapper is destroyed, because the ANARI arrays it uses
/// own the Viskores memory they share (see `MakeANARIArray1D`). Additionally,
/// ANARIMappers will update surface or volume objects if changes occur, such as
/// changes to the color map or ANARIActor.
struct VISKORES_ANARI_EXPORT ANARIMapper
{
  ANARIMapper(
    anari_cpp::Device device,
    const ANARIActor& actor = {},
    const std::string& name = "<noname>",
    const viskores::cont::ColorTable& colorTable = viskores::cont::ColorTable::Preset::Default);
  virtual ~ANARIMapper() = default;

  anari_cpp::Device GetDevice() const;
  const ANARIActor& GetActor() const;
  const char* GetName() const;
  const viskores::cont::ColorTable& GetColorTable() const;

  void SetName(const char* name);
  void SetColorTable(const viskores::cont::ColorTable& colorTable);

  /// @brief Set the current actor on this mapper.
  ///
  /// This sets the actor used to create the geometry. When the actor is changed
  /// the mapper will update all the corresponding ANARI objects accordingly.
  /// This will not cause new ANARI geometry handles to be made, rather the
  /// existing handles will be updated to reflect the new actor's data.
  virtual void SetActor(const ANARIActor& actor);

  /// @brief Set whether fields from `ANARIActor` should end up as geometry attributes.
  ///
  /// When this is disabled, the mapper will skip creating the data arrays
  /// associated with fields for when applications only want the raw geometry.
  /// This defaults to being enabled.
  virtual void SetMapFieldAsAttribute(bool enabled);
  bool GetMapFieldAsAttribute() const;

  /// @brief Set color map arrays using raw ANARI array handles.
  /// @param color Color array used for color mapping.
  /// @param opacity (unused/deprecated, will remove on future ANARI version)
  /// @param releaseArrays If true this function will release the handles passed in.
  virtual void SetANARIColorMap(anari_cpp::Array1D color,
                                anari_cpp::Array1D opacity,
                                bool releaseArrays = true);

  /// @brief Set the value range (domain) for the color map.
  ///
  virtual void SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange);

  /// @brief Set a scale factor for opacity (typically used for volumes).
  ///
  virtual void SetANARIColorMapOpacityScale(viskores::Float32 opacityScale);

  /// @brief Get the corresponding ANARIGeometry handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::Geometry GetANARIGeometry();

  /// @brief Get the corresponding ANARISpatialField handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::SpatialField GetANARISpatialField();

  /// @brief Get the corresponding ANARISurface handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::Surface GetANARISurface();

  /// @brief Get the corresponding ANARIVolume handle from this mapper.
  ///
  /// NOTE: This handle is not retained, so applications should not release it.
  virtual anari_cpp::Volume GetANARIVolume();

  anari_cpp::Group GetANARIGroup();
  anari_cpp::Instance GetANARIInstance();

  bool GroupIsEmpty() const;

protected:
  std::string MakeObjectName(const char* suffix) const;

  /// @brief The image sampler `inTransform` that maps `valueRange` onto [0, 1].
  ///
  /// A range too narrow to invert, such as min equal to max, is widened
  /// symmetrically about its center so the transform stays finite and the
  /// center value maps to 0.5.
  static anari_cpp::mat4 MakeColorMapInTransform(const viskores::Vec2f_32& valueRange);
  void RefreshGroup();

  viskores::cont::ColorTable& GetColorTable();

  bool Valid{ false };
  bool Current{ false };

private:
  struct ANARIHandles
  {
    anari_cpp::Device Device{ nullptr };
    anari_cpp::Group Group{ nullptr };
    anari_cpp::Instance Instance{ nullptr };
    ~ANARIHandles();
  };

  std::shared_ptr<ANARIHandles> Handles;
  ANARIActor Actor;
  viskores::cont::ColorTable ColorTable;
  std::string Name;
  bool MapFieldAsAttribute{ true };
};

} // namespace anari
} // namespace interop
} // namespace viskores

#endif
