##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

# This code patches the `breathe` package to execute faster. This is designed to
# explicitly patch breathe version 4.36.0. Problems might occur if applied to
# any other version.
#
# The problem with breathe 4.36.0 is reported at
# https://github.com/breathe-doc/breathe/issues/1069 .
# The issue that occurs is that this version of breathe is using the
# `pathlib.Path.resolve()` method in several places, and that can slow things
# down significantly in situations where disk access is slow such as in
# containers or networked drives. This script works by finding instances
# of the `resolve()` method and replace them with `absolute()`. For this
# specific version of breathe, this text substitution correctly modifies the
# code. Things might break in other versions.
#
# This code is executed in `.readthedocs.yaml` during the building of the guide
# by readthedocs.org. It modifies the locally-installed version of breathe. You
# can also execute it locally to patch your pip-installed packages. The change
# may have less effect on a local system.
#
# If the version of breathe changes in docs/users-guide/requirements.txt, then
# this script should be modified or removed and .readthedocs.yaml should be
# modified.

from pathlib import Path

import breathe

expected_version = "4.36.0"
if breathe.__version__ != expected_version:
  print("Incorrect version:", breathe.__version__)
  print("This patch script is designed to work specifically with", expected_version)
  print("If the version of breathe was changed in docs/users-guide/requirements.txt,")
  print("then this patch should be updated or removed. If removed, be sure to remove")
  print("the corresponding line from .readthedocs.yaml.")
  exit(1)

print("Patching breathe", breathe.__version__)

import breathe.file_state_cache
import breathe.parser
import breathe.path_handler
import breathe.project
import breathe.renderer.sphinxrenderer

def remove_resolve(module):
  old = ").resolve()"
  new = ").absolute()"

  path = Path(module.__file__)

  text = path.read_text()
  if old not in text:
    raise RuntimeError(f"Expected .resolve() call not found in {path}")

  path.write_text(text.replace(old, new))
  print(f"Patched {path}")

remove_resolve(breathe.file_state_cache)
remove_resolve(breathe.parser)
remove_resolve(breathe.path_handler)
remove_resolve(breathe.project)
remove_resolve(breathe.renderer.sphinxrenderer)
