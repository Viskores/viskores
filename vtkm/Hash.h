//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_Hash_h
#define vtk_m_Hash_h

#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{

using HashType = vtkm::UInt32;

namespace detail
{

static constexpr vtkm::HashType FNV1A_OFFSET = 2166136261;
static constexpr vtkm::HashType FNV1A_PRIME = 16777619;

/// \brief Performs an FNV-1a hash on 32-bit integers returning a 32-bit hash
///
template <typename InVecType>
VTKM_EXEC_CONT inline vtkm::HashType HashFNV1a32(const InVecType& inVec)
{
  using Traits = vtkm::VecTraits<InVecType>;
  const vtkm::IdComponent numComponents = Traits::GetNumberOfComponents(inVec);

  vtkm::HashType hash = FNV1A_OFFSET;
  for (vtkm::IdComponent index = 0; index < numComponents; index++)
  {
    vtkm::HashType dataBits = static_cast<vtkm::HashType>(Traits::GetComponent(inVec, index));
    hash = (hash * FNV1A_PRIME) ^ dataBits;
  }

  return hash;
}

/// \brief Performs an FNV-1a hash on 64-bit integers returning a 32-bit hash
///
template <typename InVecType>
VTKM_EXEC_CONT inline vtkm::HashType HashFNV1a64(const InVecType& inVec)
{
  using Traits = vtkm::VecTraits<InVecType>;
  const vtkm::IdComponent numComponents = Traits::GetNumberOfComponents(inVec);

  vtkm::HashType hash = FNV1A_OFFSET;
  for (vtkm::IdComponent index = 0; index < numComponents; index++)
  {
    vtkm::UInt64 allDataBits = static_cast<vtkm::UInt64>(Traits::GetComponent(inVec, index));
    vtkm::HashType upperDataBits =
      static_cast<vtkm::HashType>((allDataBits & 0xFFFFFFFF00000000L) >> 32);
    hash = (hash * FNV1A_PRIME) ^ upperDataBits;
    vtkm::HashType lowerDataBits = static_cast<vtkm::HashType>(allDataBits & 0x00000000FFFFFFFFL);
    hash = (hash * FNV1A_PRIME) ^ lowerDataBits;
  }

  return hash;
}

// If you get a compile error saying that there is no implementation of the class HashChooser,
// then you have tried to make a hash from an invalid type (like a float).
template <typename NumericTag, std::size_t DataSize>
struct HashChooser;

template <>
struct HashChooser<vtkm::TypeTraitsIntegerTag, 4>
{
  template <typename InVecType>
  VTKM_EXEC_CONT static vtkm::HashType Hash(const InVecType& inVec)
  {
    return vtkm::detail::HashFNV1a32(inVec);
  }
};

template <>
struct HashChooser<vtkm::TypeTraitsIntegerTag, 8>
{
  template <typename InVecType>
  VTKM_EXEC_CONT static vtkm::HashType Hash(const InVecType& inVec)
  {
    return vtkm::detail::HashFNV1a64(inVec);
  }
};

} // namespace detail

/// \brief Returns a 32-bit hash on a group of integer-type values.
///
/// The input to the hash is expected to be a vtkm::Vec or a Vec-like object. The values can be
/// either 32-bit integers or 64-bit integers (signed or unsigned). Regardless, the resulting hash
/// is an unsigned 32-bit integer.
///
/// The hash is designed to minimize the probability of collisions, but collisions are always
/// possible. Thus, when using these hashes there should be a contingency for dealing with
/// collisions.
///
template <typename InVecType>
VTKM_EXEC_CONT inline vtkm::HashType Hash(const InVecType& inVec)
{
  using VecTraits = vtkm::VecTraits<InVecType>;
  using ComponentType = typename VecTraits::ComponentType;
  using ComponentTraits = vtkm::TypeTraits<ComponentType>;
  using Chooser = detail::HashChooser<typename ComponentTraits::NumericTag, sizeof(ComponentType)>;
  return Chooser::Hash(inVec);
}

} // namespace vtkm

#endif //vtk_m_Hash_h
