# `ArrayHandleExtractComponent` target component is now set at runtime.

Rather than embedding the extracted component in a template parameter, the
extract operation is now defined at runtime.

This is easier to use and keeps compile times / sizes / memory requirements
down.
