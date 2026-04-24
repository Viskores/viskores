#!/usr/bin/env python3

##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import numpy as np
from OpenGL import GL

import viskores
from viskores.cont import Association
from viskores.interop import BufferState, TransferToOpenGL


def main():
    state = BufferState()
    assert state.GetHandle() == 0
    assert not state.HasType()
    assert state.GetType() == GL.GL_INVALID_VALUE
    assert state.GetSize() == 0
    assert state.GetCapacity() == 0

    state.SetType(GL.GL_ARRAY_BUFFER)
    assert state.HasType()
    assert state.GetType() == GL.GL_ARRAY_BUFFER

    state = BufferState(handle=17, buffer_type=GL.GL_ARRAY_BUFFER)
    assert state.GetHandle() == 17
    assert state.GetType() == GL.GL_ARRAY_BUFFER
    state.SetHandle(23)
    assert state.GetHandle() == 23

    dataset = viskores.create_uniform_dataset((4, 4))
    colors = np.zeros((16, 4), dtype=np.uint8)
    colors[:, 1] = np.arange(16, dtype=np.uint8)
    colors[:, 3] = 255
    dataset.AddPointField("colors", colors)

    roundtrip = dataset.GetField("colors", association=Association.POINTS)
    assert roundtrip.dtype == np.uint8
    assert roundtrip.shape == (16, 4)
    np.testing.assert_array_equal(roundtrip, colors)

    assert callable(TransferToOpenGL)


if __name__ == "__main__":
    main()
