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
  /// @brief Identifies what elements of a data set a field is associated with.
  ///
  /// The `Association` enum is used by `vtkm::cont::Field` to specify on what
  /// topological elements each item in the field is associated with.
  enum struct Association
  {
    // Documentation is below (for better layout in generated documents).
    Any,
    WholeDataSet,
    Points,
    Cells,
    Partitions,
    Global,
  };

  /// @var Association Any
  /// @brief Any field regardless of the association.
  ///
  /// This is used when choosing a `vtkm::cont::Field` that could be of any
  /// association. It is often used as the default if no association is given.

  /// @var Association WholeDataSet
  /// @brief A "global" field that applies to the entirety of a `vtkm::cont::DataSet`.
  ///
  /// Fields of this association often contain summary or annotation information.
  /// An example of a whole data set field could be the region that the mesh covers.

  /// @var Association Points
  /// @brief A field that applies to points.
  ///
  /// There is a separate field value attached to each point. Point fields usually represent
  /// samples of continuous data that can be reinterpolated through cells. Physical properties
  /// such as temperature, pressure, density, velocity, etc. are usually best represented in
  /// point fields. Data that deals with the points of the topology, such as displacement
  /// vectors, are also appropriate for point data.

  /// @var Association Cells
  /// @brief A field that applies to cells.
  ///
  /// There is a separate field value attached to each cell in a cell set. Cell fields
  /// usually represent values from an integration over the finite cells of the mesh.
  /// Integrated values like mass or volume are best represented in cell fields. Statistics
  /// about each cell like strain or cell quality are also appropriate for cell data.

  /// @var Association Partitions
  /// @brief A field that applies to partitions.
  ///
  /// This type of field is attached to a `vtkm::cont::PartitionedDataSet`. There is a
  /// separate field value attached to each partition. Identification or information
  /// about the arrangement of partitions such as hierarchy levels are usually best
  /// represented in partition fields.

  /// @var Association Global
  /// @brief A field that applies to all partitions.
  ///
  /// This type of field is attached to a `vtkm::cont::PartitionedDataSet`. It contains
  /// values that are "global" across all partitions and data therin.

  VTKM_CONT
  Field() = default;

  /// Create a field with the given name, association, and data.
  VTKM_CONT
  Field(std::string name, Association association, const vtkm::cont::UnknownArrayHandle& data);

  /// Create a field with the given name, association, and data.
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

  /// Return true if this field is associated with cells.
  VTKM_CONT bool IsCellField() const { return this->FieldAssociation == Association::Cells; }
  /// Return true if this field is associated with points.
  VTKM_CONT bool IsPointField() const { return this->FieldAssociation == Association::Points; }
  /// Return true if this field is associated with the whole data set.
  VTKM_CONT bool IsWholeDataSetField() const
  {
    return this->FieldAssociation == Association::WholeDataSet;
  }
  /// Return true if this field is associated with partitions in a partitioned data set.
  VTKM_CONT bool IsPartitionsField() const
  {
    return this->FieldAssociation == Association::Partitions;
  }
  /// Return true if this field is global.
  /// A global field is applied to a `vtkm::cont::PartitionedDataSet` to refer to data that
  /// applies across an entire collection of data.
  VTKM_CONT bool IsGlobalField() const { return this->FieldAssociation == Association::Global; }

  /// Returns true if the array of the field has a value type that matches something in
  /// `VTKM_FIELD_TYPE_LIST` and a storage that matches something in `VTKM_FIELD_STORAGE_LIST`.
  VTKM_CONT bool IsSupportedType() const;

  /// Return the number of values in the field array.
  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfValues(); }

  /// Return the name of the field.
  VTKM_CONT const std::string& GetName() const { return this->Name; }
  /// Return the association of the field.
  VTKM_CONT Association GetAssociation() const { return this->FieldAssociation; }
  /// Get the array of the data for the field.
  const vtkm::cont::UnknownArrayHandle& GetData() const;
  /// Get the array of the data for the field.
  vtkm::cont::UnknownArrayHandle& GetData();

  /// @brief Returns the range of each component in the field array.
  ///
  /// The ranges of each component are returned in an `ArrayHandle` containing `vtkm::Range`
  /// values.
  /// So, for example, calling `GetRange` on a scalar field will return an `ArrayHandle`
  /// with exactly 1 entry in it. Calling `GetRange` on a field of 3D vectors will return
  /// an `ArrayHandle` with exactly 3 entries corresponding to each of the components in
  /// the range.
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange() const;

  /// @brief Returns the range of each component in the field array.
  ///
  /// A C array of `vtkm::Range` objects is passed in as a place to store the result.
  /// It is imperative that the array be allocated to be large enough to hold an entry
  /// for each component.
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

  VTKM_CONT void SetData(const vtkm::cont::UnknownArrayHandle& newdata);

  template <typename T, typename StorageTag>
  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<T, StorageTag>& newdata)
  {
    this->SetData(vtkm::cont::UnknownArrayHandle(newdata));
  }

  /// Print a summary of the data in the field.
  VTKM_CONT
  virtual void PrintSummary(std::ostream& out, bool full = false) const;

  /// Remove the data from the device memory (but preserve the data on the host).
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
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const std::vector<T>& data,
                             vtkm::CopyFlag copy)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandle(data, copy));
}

template <typename T>
vtkm::cont::Field make_FieldMove(std::string name,
                                 Field::Association association,
                                 std::vector<T>&& data)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandleMove(std::move(data)));
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

namespace mangled_diy_namespace
{

template <>
struct VTKM_CONT_EXPORT Serialization<vtkm::cont::Field>
{
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::Field& field);
  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::Field& field);
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_Field_h
