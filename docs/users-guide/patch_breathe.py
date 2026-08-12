from pathlib import Path

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
