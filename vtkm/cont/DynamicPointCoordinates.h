//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DynamicPointCoordinates_h
#define vtk_m_cont_DynamicPointCoordinates_h

#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/PointCoordinatesArray.h>
#include <vtkm/cont/PointCoordinatesListTag.h>

#include <vtkm/cont/internal/DynamicTransform.h>

VTKM_BOOST_PRE_INCLUDE
#include <boost/shared_ptr.hpp>
VTKM_BOOST_POST_INCLUDE

namespace vtkm {
namespace cont {

namespace internal {

/// Behaves like (and is interchangable with) a \c DynamicPointCoordinates. The
/// difference is that the lists of point coordinates, base types, and
/// storage to try when calling \c CastAndCall is set to the class template
/// arguments.
///
template<typename PointCoordinatesList,
         typename TypeList,
         typename StorageList>
class DynamicPointCoordinatesCast;

} // namespace internal

/// \brief Holds point coordinates polymorphically.
///
/// The \c DynamicPointCoordinates holds a point coordinate field for a mesh.
/// Like a \c DynamicArrayHandle it contains a \c CastAndCall method that
/// allows it to interface with templated functions and will automatically be
/// converted on a worklet invoke.
///
/// \c DynamicPointCoordinates differes from \c DynamicArrayHandle in the type
/// of arrays it will check. Point coordinates are often defined as implicit
/// (uniform), semi-implicit (structured), unstructured, or some combination
/// thereof. Methods for defining point coordinates are captured in \c
/// PointCoordinates classes, and \c DynamicPointCoordinates polymorphically
/// stores one of these \c PointCoordinates objects.
///
/// By default, \c DynamicPointCoordinates will assume that the stored point
/// coordinates are of a type specified by \c
/// VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG. This can be overriden by using the
/// \c ResetPointCoordinatesList method.
///
/// Internally, some \c PointCoordinates objects will reference dynamic arrays.
/// Thus, \c DynamicPointCoordinates also maintains lists of types and
/// storage that these subarrays might use. These default to \c
/// VTKM_DEFAULT_TYPE_LIST_TAG and \c VTKM_DEFAULT_STORAGE_LIST_TAG and can
/// be changed with the \c ResetTypeList and \c ResetStorageList methods.
///
class DynamicPointCoordinates
{
public:
  VTKM_CONT_EXPORT
  DynamicPointCoordinates() {  }

  /// Special constructor for the common case of using a basic array to store
  /// point coordinates.
  ///
  VTKM_CONT_EXPORT
  DynamicPointCoordinates(const vtkm::cont::DynamicArrayHandle &array)
    : PointCoordinatesContainer(new vtkm::cont::PointCoordinatesArray(array))
  {  }

  /// Special constructor for the common case of using a basic array to store
  /// point coordinates.
  ///
  template<typename Storage>
  VTKM_CONT_EXPORT
  DynamicPointCoordinates(
      const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,3>,Storage> &array)
    : PointCoordinatesContainer(new vtkm::cont::PointCoordinatesArray(array))
  {  }

  /// Special constructor for the common case of using a basic array to store
  /// point coordinates.
  ///
  template<typename Storage>
  VTKM_CONT_EXPORT
  DynamicPointCoordinates(
      const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64,3>,Storage> &array)
    : PointCoordinatesContainer(new vtkm::cont::PointCoordinatesArray(array))
  {  }

  /// Takes a concrete point coordinates class and stores it polymorphically.
  /// Although the template will match any possible type, there is a check
  /// to make sure that the type is a valid point coordinates class. If you get
  /// a compile error in the check, follow the instantiation list to where the
  /// constructor is called.
  ///
  template<typename PointCoordinatesType>
  VTKM_CONT_EXPORT
  DynamicPointCoordinates(const PointCoordinatesType &pointCoordinates)
    : PointCoordinatesContainer(new PointCoordinatesType(pointCoordinates))
  {
    VTKM_IS_POINT_COORDINATES(PointCoordinatesType);
  }

  /// Returns true if these point coordinates are stored in a \c
  /// PointCoordinates class of the given type.
  ///
  template<typename PointCoordinatesType>
  VTKM_CONT_EXPORT
  bool IsPointCoordinateType(PointCoordinatesType = PointCoordinatesType()) const
  {
    VTKM_IS_POINT_COORDINATES(PointCoordinatesType);
    return (this->TryCastPointCoordinatesType<PointCoordinatesType>() != NULL);
  }

  /// Returns these point coordinates in a \c PointCoordinates class of the
  /// given type. Throws \c ErrorControlBadValue if the cast does not work. Use
  /// \c IsPointCoordinateType to check if the cast can happen.
  ///
  template<typename PointCoordinatesType>
  VTKM_CONT_EXPORT
  const PointCoordinatesType &
  CastToPointCoordinates(PointCoordinatesType = PointCoordinatesType()) const {
    VTKM_IS_POINT_COORDINATES(PointCoordinatesType);
    PointCoordinatesType *pointCoordinates =
        this->TryCastPointCoordinatesType<PointCoordinatesType>();
    if (pointCoordinates == NULL)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Bad cast of dynamic point coordinates.");
    }
    return *pointCoordinates;
  }

  /// Changes the point coordinates objects to try casting to when resolving
  /// dynamic arrays within the point coordinates container, which is specified
  /// with a list tag like those in PointCoordinatesListTag.h. Since C++ does
  /// not allow you to actually change the template arguments, this method
  /// returns a new dynamic array object. This method is particularly useful to
  /// narrow down (or expand) the types when using an array of particular
  /// constraints.
  ///
  template<typename NewPointCoordinatesList>
  VTKM_CONT_EXPORT
  internal::DynamicPointCoordinatesCast<
    NewPointCoordinatesList,
    VTKM_DEFAULT_TYPE_LIST_TAG,
    VTKM_DEFAULT_STORAGE_LIST_TAG>
  ResetPointCoordinatesList(
      NewPointCoordinatesList = NewPointCoordinatesList()) const {
    VTKM_IS_LIST_TAG(NewPointCoordinatesList);
    return internal::DynamicPointCoordinatesCast<
        NewPointCoordinatesList,
        VTKM_DEFAULT_TYPE_LIST_TAG,
        VTKM_DEFAULT_STORAGE_LIST_TAG>(*this);
  }

  /// Changes the types to try casting to when resolving dynamic arrays within
  /// the point coordinates container, which is specified with a list tag like
  /// those in TypeListTag.h. Since C++ does not allow you to actually change
  /// the template arguments, this method returns a new dynamic array object.
  /// This method is particularly useful to narrow down (or expand) the types
  /// when using an array of particular constraints.
  ///
  template<typename NewTypeList>
  VTKM_CONT_EXPORT
  internal::DynamicPointCoordinatesCast<
    VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG,
    NewTypeList,
    VTKM_DEFAULT_STORAGE_LIST_TAG>
  ResetTypeList(NewTypeList = NewTypeList()) const {
    VTKM_IS_LIST_TAG(NewTypeList);
    return internal::DynamicPointCoordinatesCast<
        VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG,
        NewTypeList,
        VTKM_DEFAULT_STORAGE_LIST_TAG>(*this);
  }

  /// Changes the array storage to try casting to when resolving dynamic
  /// arrays within the point coordinates container, which is specified with a
  /// list tag like those in StorageListTag.h. Since C++ does not allow you
  /// to actually change the template arguments, this method returns a new
  /// dynamic array object. This method is particularly useful to narrow down
  /// (or expand) the types when using an array of particular constraints.
  ///
  template<typename NewStorageList>
  VTKM_CONT_EXPORT
  internal::DynamicPointCoordinatesCast<
    VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG,
    VTKM_DEFAULT_TYPE_LIST_TAG,
    NewStorageList>
  ResetStorageList(NewStorageList = NewStorageList()) const {
    VTKM_IS_LIST_TAG(NewStorageList);
    return internal::DynamicPointCoordinatesCast<
        VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG,
        VTKM_DEFAULT_TYPE_LIST_TAG,
        NewStorageList>(*this);
  }

  /// Attempts to cast the held point coordinates to a specific array
  /// representation and then call the given functor with the cast array. This
  /// is generally done in two parts. The first part finds the concrete type of
  /// \c PointCoordinates object by trying all those in \c
  /// VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG.
  ///
  /// The second part then asks the concrete \c PointCoordinates object to cast
  /// and call to a concrete array. This second cast might rely on \c
  /// VTKM_DEFAULT_TYPE_LIST_TAG and \c VTKM_DEFAULT_STORAGE_LIST_TAG.
  ///
  template<typename Functor>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f) const
  {
    this->CastAndCall(f,
                      VTKM_DEFAULT_POINT_COORDINATES_LIST_TAG(),
                      VTKM_DEFAULT_TYPE_LIST_TAG(),
                      VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  /// A version of \c CastAndCall that tries specified lists of point
  /// coordinates, types, and storage.
  ///
  template<typename Functor,
           typename PointCoordinatesList,
           typename TypeList,
           typename StorageList>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f,
                   PointCoordinatesList,
                   TypeList,
                   StorageList) const;

private:
  boost::shared_ptr<vtkm::cont::internal::PointCoordinatesBase>
      PointCoordinatesContainer;

  template<typename PointCoordinatesType>
  VTKM_CONT_EXPORT
  PointCoordinatesType *
  TryCastPointCoordinatesType() const {
    VTKM_IS_POINT_COORDINATES(PointCoordinatesType);
    return dynamic_cast<PointCoordinatesType *>(
          this->PointCoordinatesContainer.get());
  }
};

namespace detail {

template<typename Functor, typename TypeList, typename StorageList>
struct DynamicPointCoordinatesTryStorage
{
  const DynamicPointCoordinates PointCoordinates;
  const Functor &Function;
  bool FoundCast;

  VTKM_CONT_EXPORT
  DynamicPointCoordinatesTryStorage(
      const DynamicPointCoordinates &pointCoordinates,
      const Functor &f)
    : PointCoordinates(pointCoordinates), Function(f), FoundCast(false)
  {  }

  template<typename PointCoordinatesType>
  VTKM_CONT_EXPORT
  void operator()(PointCoordinatesType) {
    if (!this->FoundCast &&
        this->PointCoordinates.IsPointCoordinateType(PointCoordinatesType()))
    {
      PointCoordinatesType pointCoordinates =
          this->PointCoordinates.CastToPointCoordinates(PointCoordinatesType());
      pointCoordinates.CastAndCall(this->Function, TypeList(), StorageList());
      this->FoundCast = true;
    }
  }
};

} // namespace detail

template<typename Functor,
         typename PointCoordinatesList,
         typename TypeList,
         typename StorageList>
VTKM_CONT_EXPORT
void DynamicPointCoordinates::CastAndCall(const Functor &f,
                                          PointCoordinatesList,
                                          TypeList,
                                          StorageList) const
{
  VTKM_IS_LIST_TAG(PointCoordinatesList);
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);
  typedef detail::DynamicPointCoordinatesTryStorage<
      Functor, TypeList, StorageList> TryTypeType;
  TryTypeType tryType = TryTypeType(*this, f);
  vtkm::ListForEach(tryType, PointCoordinatesList());
  if (!tryType.FoundCast)
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Could not find appropriate cast for point coordinates in CastAndCall.");
  }
}

namespace internal {

template<typename PointCoordinatesList,
         typename TypeList,
         typename StorageList>
class DynamicPointCoordinatesCast : public vtkm::cont::DynamicPointCoordinates
{
  VTKM_IS_LIST_TAG(PointCoordinatesList);
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);

public:
  VTKM_CONT_EXPORT
  DynamicPointCoordinatesCast() : DynamicPointCoordinates() {  }

  VTKM_CONT_EXPORT
  DynamicPointCoordinatesCast(const vtkm::cont::DynamicPointCoordinates &coords)
    : DynamicPointCoordinates(coords) {  }

  template<typename SrcPointCoordinatesList,
           typename SrcTypeList,
           typename SrcStorageList>
  VTKM_CONT_EXPORT
  DynamicPointCoordinatesCast(
      const DynamicPointCoordinatesCast<SrcPointCoordinatesList,SrcTypeList,SrcStorageList> &coords)
    : DynamicPointCoordinates(coords)
  {  }

  template<typename NewPointCoordinatesList>
  VTKM_CONT_EXPORT
  DynamicPointCoordinatesCast<NewPointCoordinatesList,TypeList,StorageList>
  ResetPointCoordinatesList(
      NewPointCoordinatesList = NewPointCoordinatesList()) const {
    VTKM_IS_LIST_TAG(NewPointCoordinatesList);
    return DynamicPointCoordinatesCast<
        NewPointCoordinatesList,TypeList,StorageList>(*this);
  }

  template<typename NewTypeList>
  VTKM_CONT_EXPORT
  DynamicPointCoordinatesCast<PointCoordinatesList,NewTypeList,StorageList>
  ResetTypeList(NewTypeList = NewTypeList()) const {
    VTKM_IS_LIST_TAG(NewTypeList);
    return DynamicPointCoordinatesCast<
        PointCoordinatesList,NewTypeList,StorageList>(*this);
  }

  template<typename NewStorageList>
  VTKM_CONT_EXPORT
  DynamicPointCoordinatesCast<PointCoordinatesList,TypeList,NewStorageList>
  ResetStorageList(NewStorageList = NewStorageList()) const {
    VTKM_IS_LIST_TAG(NewStorageList);
    return DynamicPointCoordinatesCast<
        PointCoordinatesList,TypeList,NewStorageList>(*this);
  }

  template<typename Functor>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f) const
  {
    this->CastAndCall(f, PointCoordinatesList(), TypeList(), StorageList());
  }

  template<typename Functor, typename PCL, typename TL, typename CL>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f, PCL, TL, CL) const
  {
    this->DynamicPointCoordinates::CastAndCall(f, PCL(), TL(), CL());
  }
};

template<>
struct DynamicTransformTraits<vtkm::cont::DynamicPointCoordinates> {
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};

template<typename PointCoordinatesList,
         typename TypeList,
         typename StorageList>
struct DynamicTransformTraits<
    vtkm::cont::internal::DynamicPointCoordinatesCast<
      PointCoordinatesList,TypeList,StorageList> >
{
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_DynamicPointCoordinates_h
