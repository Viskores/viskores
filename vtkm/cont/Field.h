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
#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/UnknownArrayHandle.h>

namespace vtkm
{
namespace cont
{


/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a point logical dimension, or the whole mesh.
///
class VTKM_CONT_EXPORT Field
{
public:
  enum struct Association
  {
    Any,
    WholeMesh,
    Points,
    Cells,
    ANY VTKM_DEPRECATED(1.8, "Use vtkm::cont::Field::Association::Any.") = Any,
    WHOLE_MESH VTKM_DEPRECATED(1.8, "Use vtkm::cont::Field::Association::WholeMesh.") = WholeMesh,
    POINTS VTKM_DEPRECATED(1.8, "Use vtkm::cont::Field::Association::Points.") = Points,
    CELL_SET VTKM_DEPRECATED(1.8, "Use vtkm::cont::Field::Association::Cells.") = Cells
  };

  VTKM_CONT
  Field() = default;

  VTKM_CONT
  Field(std::string name, Association association, const vtkm::cont::UnknownArrayHandle& data);

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name,
                  Association association,
                  const vtkm::cont::ArrayHandle<T, Storage>& data)
    : Field(name, association, vtkm::cont::UnknownArrayHandle{ data })
  {
  }

  Field(const vtkm::cont::Field& src);
  Field(vtkm::cont::Field&& src) noexcept;

  VTKM_CONT virtual ~Field();

  VTKM_CONT Field& operator=(const vtkm::cont::Field& src);
  VTKM_CONT Field& operator=(vtkm::cont::Field&& src) noexcept;

  VTKM_CONT bool IsFieldCell() const { return this->FieldAssociation == Association::Cells; }
  VTKM_CONT bool IsFieldPoint() const { return this->FieldAssociation == Association::Points; }
  VTKM_CONT bool IsFieldGlobal() const { return this->FieldAssociation == Association::WholeMesh; }

  /// Returns true if the array of the field has a value type that matches something in
  /// `VTKM_FIELD_TYPE_LIST` and a storage that matches something in `VTKM_FIELD_STORAGE_LIST`.
  VTKM_CONT bool IsSupportedType() const;

  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfValues(); }

  VTKM_CONT const std::string& GetName() const { return this->Name; }
  VTKM_CONT Association GetAssociation() const { return this->FieldAssociation; }
  const vtkm::cont::UnknownArrayHandle& GetData() const;
  vtkm::cont::UnknownArrayHandle& GetData();

  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange() const;

  VTKM_CONT void GetRange(vtkm::Range* range) const;

  /// \brief Get the data as an array with `vtkm::FloatDefault` components.
  ///
  /// Returns a `vtkm::cont::UnknownArrayHandle` that contains an array that either contains
  /// values of type `vtkm::FloatDefault` or contains `Vec`s with components of type
  /// `vtkm::FloatDefault`. If the array has value types that do not match this type, then
  /// it will be copied into an array that does.
  ///
  /// Additionally, the returned array will have a storage that is compatible with
  /// something in `VTKM_FIELD_STORAGE_LIST`. If this condition is not met, then the
  /// array will be copied.
  ///
  /// If the array contained in the field already matches the required criteria, the array
  /// will be returned without copying.
  ///
  VTKM_CONT vtkm::cont::UnknownArrayHandle GetDataAsDefaultFloat() const;

  /// \brief Get the data as an array of an expected type.
  ///
  /// Returns a `vtkm::cont::UnknownArrayHandle` that contains an array that (probably) has
  /// a value type that matches something in `VTKM_FIELD_TYPE_LIST` and a storage that matches
  /// something in `VTKM_FIELD_STORAGE_LIST`. If the array has a value type and storage that
  /// match `VTKM_FIELD_TYPE_LIST` and `VTKM_FIELD_STORAGE_LIST` respectively, then the same
  /// array is returned. If something does not match, then the data are copied to a
  /// `vtkm::cont::ArrayHandleBasic` with a value type component of `vtkm::FloatDefault`.
  ///
  /// Note that the returned array is likely to be compatible with `VTKM_FIELD_TYPE_LIST`, but
  /// not guaranteed. In particular, if this field contains `Vec`s, the returned array will also
  /// contain `Vec`s of the same size. For example, if the field contains `vtkm::Vec2i_16` values,
  /// they will (likely) be converted to `vtkm::Vec2f`. Howver, `vtkm::Vec2f` may still not be
  /// in `VTKM_FIELD_TYPE_LIST`.
  ///
  VTKM_CONT vtkm::cont::UnknownArrayHandle GetDataWithExpectedTypes() const;

  /// \brief Convert this field to use an array of an expected type.
  ///
  /// Copies the internal data, as necessary, to an array that (probably) has a value type
  /// that matches something in `VTKM_FIELD_TYPE_LIST` and a storage that matches something
  /// in `VTKM_FIELD_STORAGE_LIST`. If the field already has a value type and storage that
  /// match `VTKM_FIELD_TYPE_LIST` and `VTKM_FIELD_STORAGE_LIST` respectively, then nothing
  /// in the field is changed. If something does not match, then the data are copied to a
  /// `vtkm::cont::ArrayHandleBasic` with a value type component of `vtkm::FloatDefault`.
  ///
  /// Note that the returned array is likely to be compatible with `VTKM_FIELD_TYPE_LIST`, but
  /// not guaranteed. In particular, if this field contains `Vec`s, the returned array will also
  /// contain `Vec`s of the same size. For example, if the field contains `vtkm::Vec2i_16` values,
  /// they will (likely) be converted to `vtkm::Vec2f`. Howver, `vtkm::Vec2f` may still not be
  /// in `VTKM_FIELD_TYPE_LIST`.
  ///
  VTKM_CONT void ConvertToExpected();

  template <typename TypeList>
  VTKM_DEPRECATED(1.6, "TypeList no longer supported in Field::GetRange.")
  VTKM_CONT void GetRange(vtkm::Range* range, TypeList) const
  {
    this->GetRange(range);
  }

  template <typename TypeList>
  VTKM_DEPRECATED(1.6, "TypeList no longer supported in Field::GetRange.")
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(TypeList) const
  {
    return this->GetRange();
  }

  VTKM_CONT void SetData(const vtkm::cont::UnknownArrayHandle& newdata);

  template <typename T, typename StorageTag>
  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<T, StorageTag>& newdata)
  {
    this->SetData(vtkm::cont::UnknownArrayHandle(newdata));
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

  Association FieldAssociation = Association::Any;
  vtkm::cont::UnknownArrayHandle Data;
  mutable vtkm::cont::ArrayHandle<vtkm::Range> Range;
  mutable bool ModifiedFlag = true;
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
                             vtkm::CopyFlag copy)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandle(data, size, copy));
}

template <typename T>
VTKM_DEPRECATED(1.6, "Specify a vtkm::CopyFlag or use a move version of make_Field.")
vtkm::cont::Field
  make_Field(std::string name, Field::Association association, const T* data, vtkm::Id size)
{
  return make_Field(name, association, data, size, vtkm::CopyFlag::Off);
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const std::vector<T>& data,
                             vtkm::CopyFlag copy)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandle(data, copy));
}

template <typename T>
VTKM_DEPRECATED(1.6, "Specify a vtkm::CopyFlag or use a move version of make_Field.")
vtkm::cont::Field
  make_Field(std::string name, Field::Association association, const std::vector<T>& data)
{
  return make_Field(name, association, data, vtkm::CopyFlag::Off);
}

template <typename T>
vtkm::cont::Field make_FieldMove(std::string name,
                                 Field::Association association,
                                 std::vector<T>&& data)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandleMove(data));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             std::vector<T>&& data,
                             vtkm::CopyFlag vtkmNotUsed(copy))
{
  return make_FieldMove(name, association, std::move(data));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             std::initializer_list<T>&& data)
{
  return make_FieldMove(name, association, vtkm::cont::make_ArrayHandle(std::move(data)));
}

//@}

/// Convenience function to build point fields from vtkm::cont::ArrayHandle
template <typename T, typename S>
vtkm::cont::Field make_FieldPoint(std::string name, const vtkm::cont::ArrayHandle<T, S>& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::Points, data);
}

/// Convenience function to build point fields from vtkm::cont::UnknownArrayHandle
inline vtkm::cont::Field make_FieldPoint(std::string name,
                                         const vtkm::cont::UnknownArrayHandle& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::Points, data);
}

/// Convenience function to build cell fields from vtkm::cont::ArrayHandle
template <typename T, typename S>
vtkm::cont::Field make_FieldCell(std::string name, const vtkm::cont::ArrayHandle<T, S>& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::Cells, data);
}


/// Convenience function to build cell fields from vtkm::cont::UnknownArrayHandle
inline vtkm::cont::Field make_FieldCell(std::string name,
                                        const vtkm::cont::UnknownArrayHandle& data)
{
  return vtkm::cont::Field(name, vtkm::cont::Field::Association::Cells, data);
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
struct VTKM_DEPRECATED(1.6, "You can now directly serialize Field.") SerializableField
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

template <>
struct VTKM_CONT_EXPORT Serialization<vtkm::cont::Field>
{
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::Field& field);
  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::Field& field);
};

// Implement deprecated code
VTKM_DEPRECATED_SUPPRESS_BEGIN
template <typename TypeList>
struct Serialization<vtkm::cont::SerializableField<TypeList>>
{
private:
  using Type = vtkm::cont::SerializableField<TypeList>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& serializable)
  {
    Serialization<vtkm::cont::Field>::save(bb, serializable.Field);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& serializable)
  {
    Serialization<vtkm::cont::Field>::load(bb, serializable.Field);
  }
};
VTKM_DEPRECATED_SUPPRESS_END

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_Field_h
