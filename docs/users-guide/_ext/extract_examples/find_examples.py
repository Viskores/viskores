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

import re

class SourceLine:
  '''Class to hold lines to include as example along with line numbers.'''
  def __init__(self, code, lineno):
    self.code = code
    self.lineno = lineno

class Example:
  '''Class to hold an extracted example.
  The `name` field contains the name of the example. The `lines` field
  contains an array of `SourceLine` objects containing the contents.'''
  def __init__(self, name, sourcefile, startline):
    self.name = name
    self.sourcefile = sourcefile
    self.startline = startline
    self.lines = []
    self.labels = {}

  def add_line(self, code, lineno):
    self.lines.append(SourceLine(code, lineno))

class ReadError(Exception):
  '''This class is thrown as an exception if a read error occurs.'''
  def __init__(self, message, filename, lineno, line):
    super().__init__('%s:%s: %s\n%s' % (filename, lineno, message, line))

loaded_examples = {}
loaded_files = []

def read_file(filename, command_comment='////', verbose=False):
  '''Reads a source file and finds examples declared by special command
  comments. This method returns an array of `Example` objects.'''

  if verbose:
    print('Reading file %s' % filename)

  lines = open(filename, 'r').readlines()

  examples = []
  active_examples = {}
  lineno = 0
  paused = False
  for line in lines:
    lineno += 1
    index = line.find(command_comment)
    if index >= 0:
      command = line[(index + len(command_comment)):].split()
      if len(command) < 1:
        pass
      elif command[0] == 'BEGIN-EXAMPLE':
        if len(command) != 2:
          raise ReadError('BEGIN-EXAMPLE requires exactly one argument.',
                           filename,
                           lineno,
                           line)
        example_name = command[1]
        if verbose:
          print('Extracting example `%s`' % example_name)
        if example_name in active_examples:
          raise ReadError('Example %s declared within itself.' % example_name,
                           filename,
                           lineno,
                           line)
        active_examples[example_name] = Example(example_name, filename, lineno)
      elif command[0] == 'END-EXAMPLE':
        if len(command) != 2:
          raise ReadError('END-EXAMPLE requires exactly one argument.',
                           filename,
                           lineno,
                           line)
        example_name = command[1]
        if example_name not in active_examples:
          raise ReadError('Example %s ended before it began.' % example_name,
                           filename,
                           lineno,
                           line)
        examples.append(active_examples[example_name])
        del active_examples[example_name]
      elif command[0] == 'PAUSE-EXAMPLE':
        if paused:
          raise ReadError('Example iteratively paused.',
                           filename, lineno, line)
        paused = True
      elif command[0] == 'RESUME-EXAMPLE':
        if not paused:
          raise ReadError('Example resumed without being paused.',
                           filename, lineno, line)
        paused = False
      elif command[0] == 'LABEL':
        if len(command) != 2:
          raise ReadError('LABEL requires exactly one argument.',
                           filename,
                           lineno,
                           line)
        label = command[1]
        for name in active_examples:
          nextline = len(active_examples[name].lines) + 1
          active_examples[name].labels[label] = nextline
      else:
        raise ReadError('Command %s not recognized.' % command[0],
                         filename,
                         lineno,
                         line)
    else:
      # Line not a command. Add it to any active examples.
      if not paused:
        for name in active_examples:
          active_examples[name].add_line(line.rstrip(), lineno)

  if active_examples:
    raise ReadError(
        'Unterminated example: %s' % next(iter(active_examples.keys())),
        filename,
        lineno,
        line)

  return examples

def load_file(filename, command_comment='////', verbose=False):
  '''Loads the examples in the given file. The examples a placed in the
  `loaded_examples` dictionary, which is indexed by example name. If the
  file was previously loaded, nothing happens.'''
  if filename not in loaded_files:
    examples = read_file(filename, command_comment, verbose)
    for example in examples:
      name = example.name
      if name in loaded_examples:
        raise Exception('Example named %s found in both %s:%d and %s:%d.' %
                        (name, example.sourcefile, example.startline,
                         loaded_examples[name].sourcefile,
                         loaded_examples[name].startline))
      loaded_examples[name] = example
    loaded_files.append(filename)

def get_example(name, filename=None, command_comment='////', verbose=False):
  '''Returns an `Example` object containing the named example. If a filename
  is provided, that file is first scanned for examples (if it has not already
  been scanned for examples).'''
  if filename:
    load_file(filename, command_comment, verbose)
  if name not in loaded_examples:
    raise Exception('No example named %s found.' % name)
  example = loaded_examples[name]
  if filename and filename != example.sourcefile:
    print('WARNING: Example %s was expected in file %s but found in %s' %
          (name, filename, example.sourcefile))
  return example
