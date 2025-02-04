//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_Flags_h
#define viskores_Flags_h

namespace viskores
{

/// @brief Identifier used to specify whether a function should deep copy data.
enum struct CopyFlag
{
  Off = 0,
  On = 1
};

}

#endif // viskores_Flags_h
