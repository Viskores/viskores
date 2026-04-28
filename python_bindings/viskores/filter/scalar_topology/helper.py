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

from ...cont import ArrayHandleGroupVecVariableId

__all__ = ["group_points_by_superarc"]


def _group_points_by_superarc_csr(point_superarc_ids):
    point_superarc_ids = np.asarray(point_superarc_ids, dtype=np.int64)
    if point_superarc_ids.ndim != 1:
        raise ValueError("point_superarc_ids must be a 1D array.")

    if point_superarc_ids.size == 0:
        return np.array([0], dtype=np.int64), np.array([], dtype=np.int64)

    if np.any(point_superarc_ids < 0):
        raise ValueError("point_superarc_ids must contain non-negative superarc ids.")

    point_ids = np.argsort(point_superarc_ids, kind="stable").astype(np.int64, copy=False)
    sorted_superarcs = point_superarc_ids[point_ids]
    number_of_superarcs = int(sorted_superarcs[-1]) + 1
    counts = np.bincount(sorted_superarcs, minlength=number_of_superarcs)
    offsets = np.empty(number_of_superarcs + 1, dtype=np.int64)
    offsets[0] = 0
    np.cumsum(counts, out=offsets[1:])
    return offsets, point_ids


def group_points_by_superarc(point_superarc_ids):
    """Return original point ids grouped by containing superarc.

    The input is typically the result of
    ``ContourTreeAugmented.GetPointSuperarcIds()``. The return value is an
    ``ArrayHandleGroupVecVariableId`` where entry ``i`` contains the original
    grid point ids on superarc ``i``.
    """

    offsets, point_ids = _group_points_by_superarc_csr(point_superarc_ids)
    return ArrayHandleGroupVecVariableId(point_ids, offsets)
