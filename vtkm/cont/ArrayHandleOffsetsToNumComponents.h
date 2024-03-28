//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleOffsetsToNumComponents_h
#define vtk_m_cont_ArrayHandleOffsetsToNumComponents_h

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace internal
{

// Note that `ArrayPortalOffsetsToNumComponents` requires a source portal with +1 entry
// to avoid branching. See `ArrayHandleOffsetsToNumComponents` for details.
template <typename OffsetsPortal>
class VTKM_ALWAYS_EXPORT ArrayPortalOffsetsToNumComponents
{
  OffsetsPortal Portal;

public:
  ArrayPortalOffsetsToNumComponents() = default;

  ArrayPortalOffsetsToNumComponents(const OffsetsPortal& portal)
    : Portal(portal)
  {
  }

  using ValueType = vtkm::IdComponent;

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues() - 1; }

  VTKM_EXEC_CONT vtkm::IdComponent Get(vtkm::Id index) const
  {
    return static_cast<vtkm::IdComponent>(this->Portal.Get(index + 1) - this->Portal.Get(index));
  }
};

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

template <typename OffsetsStorageTag>
struct VTKM_ALWAYS_EXPORT StorageTagOffsetsToNumComponents
{
};

namespace internal
{

template <typename OffsetsStorageTag>
class VTKM_ALWAYS_EXPORT
  Storage<vtkm::IdComponent, vtkm::cont::StorageTagOffsetsToNumComponents<OffsetsStorageTag>>
{
  using OffsetsStorage = vtkm::cont::internal::Storage<vtkm::Id, OffsetsStorageTag>;

public:
  VTKM_STORAGE_NO_RESIZE;
  VTKM_STORAGE_NO_WRITE_PORTAL;

  using ReadPortalType =
    vtkm::internal::ArrayPortalOffsetsToNumComponents<typename OffsetsStorage::ReadPortalType>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return OffsetsStorage::CreateBuffers();
  }

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>&)
  {
    return 1;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    vtkm::Id numOffsets = OffsetsStorage::GetNumberOfValues(buffers);
    if (numOffsets < 1)
    {
      throw vtkm::cont::ErrorBadValue(
        "ArrayHandleOffsetsToNumComponents requires an offsets array with at least one value.");
    }
    return numOffsets - 1;
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    VTKM_ASSERT(OffsetsStorage::GetNumberOfValues(buffers) > 0);
    return ReadPortalType(OffsetsStorage::CreateReadPortal(buffers, device, token));
  }
};

} // namespace internal

/// \brief An `ArrayHandle` that converts an array of offsets to an array of `Vec` sizes.
///
/// It is common in VTK-m to pack small vectors of variable sizes into a single contiguous
/// array. For example, cells in an explicit cell set can each have a different amount of
/// vertices (triangles = 3, quads = 4, tetra = 4, hexa = 8, etc.). Generally, to access
/// items in this list, you need an array of components in each entry and the offset for
/// each entry. However, if you have just the array of offsets in sorted order, you can
/// easily derive the number of components for each entry by subtracting adjacent entries.
/// This works best if the offsets array has a size that is one more than the number of
/// packed vectors with the first entry set to 0 and the last entry set to the total size
/// of the packed array (the offset to the end).
///
/// `ArrayHandleOffsetsToNumComponents` decorates an array in exactly this manner. It
/// takes an offsets array and makes it behave like an array of counts. Note that the
/// offsets array must conform to the conditions described above: the offsets are in
/// sorted order and there is one additional entry in the offsets (ending in an offset
/// pointing past the end of the array).
///
/// When packing data of this nature, it is common to start with an array that is the
/// number of components. You can convert that to an offsets array using the
/// `vtkm::cont::ConvertNumComponentsToOffsets` function. This will create an offsets array
/// with one extra entry as previously described. You can then throw out the original
/// number of components array and use the offsets with `ArrayHandleOffsetsToNumComponents`
/// to represent both the offsets and num components while storing only one array.
///
template <class OffsetsArray>
class VTKM_ALWAYS_EXPORT ArrayHandleOffsetsToNumComponents
  : public vtkm::cont::ArrayHandle<
      vtkm::IdComponent,
      vtkm::cont::StorageTagOffsetsToNumComponents<typename OffsetsArray::StorageTag>>
{
  VTKM_IS_ARRAY_HANDLE(OffsetsArray);
  VTKM_STATIC_ASSERT_MSG((std::is_same<typename OffsetsArray::ValueType, vtkm::Id>::value),
                         "Offsets array must have a value type of vtkm::Id.");

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleOffsetsToNumComponents,
    (ArrayHandleOffsetsToNumComponents<OffsetsArray>),
    (vtkm::cont::ArrayHandle<
      vtkm::IdComponent,
      vtkm::cont::StorageTagOffsetsToNumComponents<typename OffsetsArray::StorageTag>>));

  VTKM_CONT ArrayHandleOffsetsToNumComponents(const OffsetsArray& array)
    : Superclass(array.GetBuffers())
  {
  }
};

template <typename OffsetsStorageTag>
VTKM_CONT vtkm::cont::ArrayHandleOffsetsToNumComponents<
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>>
make_ArrayHandleOffsetsToNumComponents(
  const vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>& array)
{
  // Converts to correct type.
  return array;
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleOffsetsToNumComponents_h
