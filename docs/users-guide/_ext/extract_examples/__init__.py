##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

from extract_examples import find_examples
import sphinx_tools

import docutils.nodes
import docutils.parsers.rst.directives

import sphinx.util.docutils

import re

class LoadExampleDirective(sphinx.util.docutils.SphinxDirective):
  has_content = False
  required_arguments = 1
  option_spec = {
    'file': docutils.parsers.rst.directives.unchanged,
    'nolinenos': docutils.parsers.rst.directives.flag,
    'caption': docutils.parsers.rst.directives.unchanged_required,
    'language': docutils.parsers.rst.directives.unchanged,
    'command-comment': docutils.parsers.rst.directives.unchanged,
  }

  def run(self):
    reparse = sphinx_tools.ReparseNodes()
    source_file = self.state.document.current_source
    source_line = self.lineno
    example_name = self.arguments[0]

    filename = None
    if 'file' in self.options:
      filename = self.options.get('file')
      if self.config.example_directory:
        filename = self.config.example_directory + '/' + filename
    else:
      print('WARNING %s:%d: Example `%s` loaded without filename.' %
            (source_file, source_line, example_name))

    if 'language' in self.options:
      language = self.options.get('language')
    else:
      language = self.config.example_language

    reparse.add_line('.. code-block:: %s' % language, source_file, source_line)
    reparse.add_line('   :name: ex:%s' % example_name, source_file, source_line)
    if 'nolinenos' not in self.options:
      reparse.add_line('   :linenos:', source_file, source_line)
    if 'caption' in self.options:
      reparse.add_line('   :caption: %s' % self.options.get('caption'),
                       source_file, source_line)
    reparse.add_line('', source_file, source_line)

    try:
      if 'command-comment' in self.options:
        command_comment = self.options.get('command-comment')
      else:
        command_comment = self.config.example_command_comment
      example = find_examples.get_example(example_name,
                                          filename=filename,
                                          command_comment=command_comment)
      for line in example.lines:
        reparse.add_line('   %s' % line.code, example.sourcefile, line.lineno)
    except Exception as e:
      error = self.state_machine.reporter.error(
        str(e),
        docutils.nodes.literal_block(self.block_text, self.block_text),
        lineno=self.lineno,
        )
      return [error]

    reparse.add_line('', source_file, source_line)
    return reparse.get_nodes(self)

def exlineref_role(name, rawtext, text, lineno, inliner, options = {}, content = []):
  match = re.fullmatch(r'(.*)<(.*)>', text, re.DOTALL)
  if match:
    pattern = match.group(1)
    ref = match.group(2)
  else:
    pattern = 'Example {number}, line {line}'
    ref = text

  match = re.fullmatch(r'(.*):([^:]*)', ref)
  if not match:
    message = inliner.reporter.error(
      'References for :exlineref: must be of the form example-name:label-name.;'
      ' `%s` is invalid.' % ref, line=lineno)
    problematic = inliner.problematic(rawtext, rawtext, message)
    return [problematic, message]
  examplename = match.group(1)
  linelabel = match.group(2)
  # Strip optional `ex:` prefix.
  match = re.fullmatch(r'ex:(.*)', examplename)
  if match:
    examplename = match.group(1)

  try:
    example = find_examples.get_example(examplename)
    if linelabel not in example.labels:
      raise Exception(
        'Label `%s` not in example `%s`' % (linelabel, examplename))
    lineno = example.labels[linelabel]
  except Exception as e:
    message = inliner.reporter.error(str(e), line=lineno)
    problematic = inliner.problematic(rawtext, rawtext, message)
    return [problematic, message]

  pattern = re.sub(r'%s', str(lineno), pattern, 1, re.DOTALL)
  pattern = pattern.format(line=lineno, number='{number}', name='{name}')
  if (pattern.find('{number}') >=0) or (pattern.find('{name}') >= 0):
    return sphinx_tools.role_reparse(
      ':numref:`%s <ex:%s>`' % (pattern, examplename), lineno, inliner)
  else:
    return sphinx_tools.role_reparse(
      ':ref:`%s <ex:%s>`' % (pattern, examplename), lineno, inliner)

def setup(app):
  app.add_config_value('example_directory',
                       default=None,
                       rebuild='env',
                       types=[str])
  app.add_config_value('example_command_comment',
                       default='####',
                       rebuild='env')
  app.add_config_value('example_language',
                       default='',
                       rebuild='env')
  app.add_directive('load-example', LoadExampleDirective)
  app.add_role('exlineref', exlineref_role)
