//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_Field_h
#define vtk_m_cont_Field_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Range.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/ArrayRangeCompute.hxx>
#include <vtkm/cont/VariantArrayHandle.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

struct ComputeRange
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& input, vtkm::cont::ArrayHandle<vtkm::Range>& range) const
  {
    range = vtkm::cont::ArrayRangeCompute(input);
  }
};

} // namespace internal


/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a point logical dimension, or the whole mesh.
///
class VTKM_CONT_EXPORT Field
{
public:
  enum struct Association
  {
    ANY,
    WHOLE_MESH,
    POINTS,
    CELL_SET
  };

  VTKM_CONT
  Field() = default;

  VTKM_CONT
  Field(std::string name, Association association, const vtkm::cont::VariantArrayHandle& data);

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name,
                  Association association,
                  const vtkm::cont::ArrayHandle<T, Storage>& data)
    : Field(name, association, vtkm::cont::VariantArrayHandle{ data })
  {
  }

  Field(const vtkm::cont::Field& src);
  Field(vtkm::cont::Field&& src) noexcept;

  VTKM_CONT virtual ~Field();

  VTKM_CONT Field& operator=(const vtkm::cont::Field& src);
  VTKM_CONT Field& operator=(vtkm::cont::Field&& src) noexcept;

  VTKM_CONT const std::string& GetName() const { return this->Name; }
  VTKM_CONT Association GetAssociation() const { return this->FieldAssociation; }
  const vtkm::cont::VariantArrayHandle& GetData() const;
  vtkm::cont::VariantArrayHandle& GetData();

  VTKM_CONT bool IsFieldCell() const { return this->FieldAssociation == Association::CELL_SET; }
  VTKM_CONT bool IsFieldPoint() const { return this->FieldAssociation == Association::POINTS; }

  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfValues(); }

  template <typename TypeList>
  VTKM_CONT void GetRange(vtkm::Range* range, TypeList) const
  {
    this->GetRangeImpl(TypeList());
    const vtkm::Id length = this->Range.GetNumberOfValues();
    for (vtkm::Id i = 0; i < length; ++i)
    {
      range[i] = this->Range.GetPortalConstControl().Get(i);
    }
  }

  template <typename TypeList>
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(TypeList) const
  {
    return this->GetRangeImpl(TypeList());
  }

  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange() const
  {
    return this->GetRangeImpl(VTKM_DEFAULT_TYPE_LIST());
  }

  VTKM_CONT void GetRange(vtkm::Range* range) const
  {
    return this->GetRange(range, VTKM_DEFAULT_TYPE_LIST());
  }

  template <typename T, typename StorageTag>
  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<T, StorageTag>& newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  VTKM_CONT
  void SetData(const vtkm::cont::VariantArrayHandle& newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  VTKM_CONT
  virtual void PrintSummary(std::ostream& out) const;

  VTKM_CONT
  virtual void ReleaseResourcesExecution()
  {
    this->Data.ReleaseResourcesExecution();
    this->Range.ReleaseResourcesExecution();
  }

private:
  std::string Name; ///< name of field

  Association FieldAssociation = Association::ANY;
  vtkm::cont::VariantArrayHandle Data;
  mutable vtkm::cont::ArrayHandle<vtkm::Range> Range;
  mutable bool ModifiedFlag = true;

  template <typename TypeList>
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRangeImpl(TypeList) const
  {
    VTKM_IS_LIST(TypeList);

    if (this->ModifiedFlag)
    {
      vtkm::cont::CastAndCall(
        this->Data.ResetTypes(TypeList()), internal::ComputeRange{}, this->Range);
      this->ModifiedFlag = false;
    }

    return this->Range;
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const vtkm::cont::Field& field, Functor&& f, Args&&... args)
{
  vtkm::cont::CastAndCall(field.GetData(), std::forward<Functor>(f), std::forward<Args>(args)...);
}


//@{
/// Convenience functions to build fields from C style arrays and std::vector
template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const T* data,
                             vtkm::Id size,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandle(data, size, copy));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const std::vector<T>& data,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandle(data, copy));
}

//@}

/// Convenience function to build point fields from vtkm::cont::ArrayHandle
template <typename T, typename S>
vtkm::cont::Field make_FieldPoint(std::string name, const vtkm::cont::ArrayHandle<T, S>& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::POINTS, data);
}

/// Convenience function to build point fields from vtkm::cont::VariantArrayHandle
inline vtkm::cont::Field make_FieldPoint(std::string name,
                                         const vtkm::cont::VariantArrayHandle& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::POINTS, data);
}

/// Convenience function to build cell fields from vtkm::cont::ArrayHandle
template <typename T, typename S>
vtkm::cont::Field make_FieldCell(std::string name, const vtkm::cont::ArrayHandle<T, S>& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::CELL_SET, data);
}


/// Convenience function to build cell fields from vtkm::cont::VariantArrayHandle
inline vtkm::cont::Field make_FieldCell(std::string name,
                                        const vtkm::cont::VariantArrayHandle& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::CELL_SET, data);
}

} // namespace cont
} // namespace vtkm


namespace vtkm
{
namespace cont
{
namespace internal
{
template <>
struct DynamicTransformTraits<vtkm::cont::Field>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};
} // namespace internal
} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{
template <typename TypeList = VTKM_DEFAULT_TYPE_LIST>
struct SerializableField
{
  SerializableField() = default;

  explicit SerializableField(const vtkm::cont::Field& field)
    : Field(field)
  {
  }

  vtkm::cont::Field Field;
};
} // namespace cont
} // namespace vtkm

namespace mangled_diy_namespace
{

template <typename TypeList>
struct Serialization<vtkm::cont::SerializableField<TypeList>>
{
private:
  using Type = vtkm::cont::SerializableField<TypeList>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& serializable)
  {
    const auto& field = serializable.Field;

    vtkmdiy::save(bb, field.GetName());
    vtkmdiy::save(bb, static_cast<int>(field.GetAssociation()));
    vtkmdiy::save(bb, field.GetData().ResetTypes(TypeList{}));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& serializable)
  {
    auto& field = serializable.Field;

    std::string name;
    vtkmdiy::load(bb, name);
    int assocVal = 0;
    vtkmdiy::load(bb, assocVal);

    auto assoc = static_cast<vtkm::cont::Field::Association>(assocVal);
    vtkm::cont::VariantArrayHandleBase<TypeList> data;
    vtkmdiy::load(bb, data);
    field = vtkm::cont::Field(name, assoc, vtkm::cont::VariantArrayHandle(data));
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_Field_h
