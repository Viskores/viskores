//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_internal_FastVec_h
#define vtk_m_exec_internal_FastVec_h

#include <vtkm/Types.h>
#include <vtkm/VecVariable.h>

namespace vtkm
{
namespace exec
{
namespace internal
{

/// Use this class to convert Vecs of any type to an efficient stack based Vec
/// type. The template parameters are the input Vec type and the maximum
/// number of components it may have. Specializations exist to optimize
/// the copy and stack usage away for already efficient types.
/// This class is useful when several accesses will be performed on
/// potentially inefficient Vec types such as VecFromPortalPermute.
///
template <typename VecType, vtkm::IdComponent MaxSize>
class FastVec
{
public:
  using Type = vtkm::VecVariable<typename VecType::ComponentType, MaxSize>;

  explicit VTKM_EXEC FastVec(const VecType& vec)
    : Vec(vec)
  {
  }

  VTKM_EXEC const Type& Get() const { return this->Vec; }

private:
  Type Vec;
};

template <typename ComponentType, vtkm::IdComponent NumComponents, vtkm::IdComponent MaxSize>
class FastVec<vtkm::Vec<ComponentType, NumComponents>, MaxSize>
{
public:
  using Type = vtkm::Vec<ComponentType, NumComponents>;

  explicit VTKM_EXEC FastVec(const Type& vec)
    : Vec(vec)
  {
    VTKM_ASSERT(vec.GetNumberOfComponents() <= MaxSize);
  }

  VTKM_EXEC const Type& Get() const { return this->Vec; }

private:
  const Type& Vec;
};

template <typename ComponentType, vtkm::IdComponent MaxSize1, vtkm::IdComponent MaxSize2>
class FastVec<vtkm::VecVariable<ComponentType, MaxSize1>, MaxSize2>
{
public:
  using Type = vtkm::VecVariable<ComponentType, MaxSize1>;

  explicit VTKM_EXEC FastVec(const Type& vec)
    : Vec(vec)
  {
    VTKM_ASSERT(vec.GetNumberOfComponents() <= MaxSize2);
  }

  VTKM_EXEC const Type& Get() const { return this->Vec; }

private:
  const Type& Vec;
};
}
}
} // vtkm::exec::internal

#endif // vtk_m_exec_internal_FastVec_h
