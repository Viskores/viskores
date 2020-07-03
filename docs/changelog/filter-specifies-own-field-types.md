# Filters specify their own field types

Previously, the policy specified which field types the filter should
operate on. The filter could remove some types, but it was not able to
add any types.

This is backward. Instead, the filter should specify what types its
supports and the policy may cull out some of those.
