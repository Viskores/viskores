//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_block_reader_h
#define vtk_m_worklet_zfp_block_reader_h

#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

using Word = vtkm::UInt64;

template <vtkm::Int32 block_size, typename WordsPortalType>
struct BlockReader
{
  const WordsPortalType& Words;
  const vtkm::Int32 m_maxbits;
  int m_block_idx;

  vtkm::Int32 m_current_bit;
  vtkm::Id Index;

  Word m_buffer;
  const vtkm::Id MaxIndex;

  VTKM_EXEC
  BlockReader(const WordsPortalType& words, const int& maxbits, const int& block_idx)
    : Words(words)
    , m_maxbits(maxbits)
    , MaxIndex(words.GetNumberOfValues() - 1)
  {
    Index = vtkm::Id(static_cast<size_t>(block_idx * maxbits) / (sizeof(Word) * 8));
    m_buffer = static_cast<Word>(Words.Get(Index));
    m_current_bit = (block_idx * maxbits) % vtkm::Int32((sizeof(Word) * 8));

    m_buffer >>= m_current_bit;
    m_block_idx = block_idx;
  }

  inline VTKM_EXEC unsigned int read_bit()
  {
    vtkm::UInt32 bit = vtkm::UInt32(m_buffer) & 1u;
    ++m_current_bit;
    m_buffer >>= 1;
    // handle moving into next word
    if (m_current_bit >= vtkm::Int32(sizeof(Word) * 8))
    {
      m_current_bit = 0;
      ++Index;
      if (Index > MaxIndex)
        return false;
      m_buffer = static_cast<Word>(Words.Get(Index));
    }
    return bit;
  }


  // note this assumes that n_bits is <= 64
  inline VTKM_EXEC vtkm::UInt64 read_bits(const int& n_bits)
  {
    vtkm::UInt64 bits;
    // rem bits will always be positive
    vtkm::Int32 rem_bits = vtkm::Int32(sizeof(Word) * 8) - m_current_bit;

    vtkm::Int32 first_read = vtkm::Min(rem_bits, n_bits);
    // first mask
    Word mask = ((Word)1 << ((first_read))) - 1;
    bits = m_buffer & mask;
    m_buffer >>= n_bits;
    m_current_bit += first_read;
    vtkm::Int32 next_read = 0;
    if (n_bits >= rem_bits)
    {
      m_current_bit = 0;
      // just read in 0s if someone asks for more bits past the end of the array.
      // not sure what the best way to deal with this i
      Index = vtkm::Min(MaxIndex, Index + 1);
      m_buffer = static_cast<Word>(Words.Get(Index));
      next_read = n_bits - first_read;
    }

    // this is basically a no-op when first read constained
    // all the bits. TODO: if we have aligned reads, this could
    // be a conditional without divergence
    mask = ((Word)1 << ((next_read))) - 1;
    bits += (m_buffer & mask) << first_read;
    m_buffer >>= next_read;
    m_current_bit += next_read;
    return bits;
  }

private:
  VTKM_EXEC BlockReader() {}

}; // block reader

} // namespace zfp
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_block_reader_h
