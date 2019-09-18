//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_block_writer_h
#define vtk_m_worklet_zfp_block_writer_h

#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

using Word = vtkm::UInt64;

template <int block_size, typename AtomicPortalType>
struct BlockWriter
{
  union UIntInt {
    vtkm::UInt64 uintpart;
    vtkm::Int64 intpart;
  };

  vtkm::Id m_word_index;
  vtkm::Int32 m_start_bit;
  vtkm::Int32 m_current_bit;
  const int m_maxbits;
  AtomicPortalType& Portal;

  VTKM_EXEC BlockWriter(AtomicPortalType& portal, const int& maxbits, const vtkm::Id& block_idx)
    : m_current_bit(0)
    , m_maxbits(maxbits)
    , Portal(portal)
  {
    m_word_index = (block_idx * maxbits) / vtkm::Int32(sizeof(Word) * 8);
    m_start_bit = vtkm::Int32((block_idx * maxbits) % vtkm::Int32(sizeof(Word) * 8));
  }

  inline VTKM_EXEC void Add(const vtkm::Id index, Word& value)
  {
    UIntInt newval;
    UIntInt old;
    (void)old;
    newval.uintpart = value;
    Portal.Add(index, newval.intpart);
  }

  inline VTKM_EXEC vtkm::UInt64 write_bits(const vtkm::UInt64& bits, const unsigned int& n_bits)
  {
    const int wbits = sizeof(Word) * 8;
    unsigned int seg_start = (m_start_bit + m_current_bit) % wbits;
    vtkm::Id write_index = m_word_index;
    write_index += vtkm::Id((m_start_bit + m_current_bit) / wbits);
    unsigned int seg_end = seg_start + n_bits - 1;
    //int write_index = m_word_index;
    unsigned int shift = seg_start;
    // we may be asked to write less bits than exist in 'bits'
    // so we have to make sure that anything after n is zero.
    // If this does not happen, then we may write into a zfp
    // block not at the specified index
    // uint zero_shift = sizeof(Word) * 8 - n_bits;
    Word left = (bits >> n_bits) << n_bits;

    Word b = bits - left;
    Word add = b << shift;
    Add(write_index, add);

    // n_bits straddles the word boundary
    bool straddle = seg_start < sizeof(Word) * 8 && seg_end >= sizeof(Word) * 8;
    if (straddle)
    {
      Word rem = b >> (sizeof(Word) * 8 - shift);
      Add(write_index + 1, rem);
    }
    m_current_bit += n_bits;
    return bits >> (Word)n_bits;
  }

  // TODO: optimize
  vtkm::UInt32 VTKM_EXEC write_bit(const unsigned int& bit)
  {
    const int wbits = sizeof(Word) * 8;
    unsigned int seg_start = (m_start_bit + m_current_bit) % wbits;
    vtkm::Id write_index = m_word_index;
    write_index += vtkm::Id((m_start_bit + m_current_bit) / wbits);
    unsigned int shift = seg_start;
    // we may be asked to write less bits than exist in 'bits'
    // so we have to make sure that anything after n is zero.
    // If this does not happen, then we may write into a zfp
    // block not at the specified index
    // uint zero_shift = sizeof(Word) * 8 - n_bits;

    Word add = (Word)bit << shift;
    Add(write_index, add);
    m_current_bit += 1;

    return bit;
  }
};

} // namespace zfp
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_block_writer_h
