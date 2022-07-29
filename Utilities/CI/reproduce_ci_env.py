#!/usr/bin/env python3

#=============================================================================
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See LICENSE.txt for details.
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the above copyright notice for more information.
#
#===============

import enum
import os
import tempfile
import string
import subprocess
import sys
import platform
import re
import yaml

def get_root_dir():
  dir_path = os.path.dirname(os.path.realpath(__file__))
  #find the where .gitlab-ci.yml is located
  try:
    src_root = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=dir_path)
    src_root = str(src_root, 'utf-8')
    src_root = src_root.rstrip('\n')
    # Corrections in case the filename is a funny Cygwin path
    src_root = re.sub(r'^/cygdrive/([a-z])/', r'\1:/', src_root)
    return src_root
  except subprocess.CalledProcessError:
    return None

def extract_stage_job_from_cmdline(*args):
  if len(args) == 1:
    stage_and_job = str(args[0]).split(':')
    if len(stage_and_job) == 1:
      stage_and_job = ['build', stage_and_job[0]]
    return stage_and_job
  return args

def load_ci_file(ci_file_path):
  ci_state = {}
  if ci_file_path:
    root_dir = os.path.dirname(ci_file_path)
    ci_state = yaml.safe_load(open(ci_file_path))
    if 'include' in ci_state:
      for inc in ci_state['include']:
        if 'local' in inc:
          #the local paths can start with '/'
          include_path = inc['local'].lstrip('/')
          include_path = os.path.join(root_dir, include_path)
          ci_state.update(yaml.safe_load(open(include_path)))
  return ci_state

# Recursively updates the target dictionary with the source dictionary.
# By recursive, I mean that if source contains a sub-dictionary that
# target also contains, the target sub-dictionary is updated rather than
# replaced. Likewise for sub-sub-dictionaries and so on. This code is
# taken from StackOverflow:
#   https://stackoverflow.com/questions/3232943/update-value-of-a-nested-dictionary-of-varying-depth
def recursive_dictionary_update(target, source):
  import collections.abc
  for key, value in source.items():
    if isinstance(value, collections.abc.Mapping):
      target[key] = recursive_dictionary_update(target.get(key, {}), value)
    else:
      target[key] = value
  return target

def flattened_entry_copy(ci_state, name):
  import copy
  entry = copy.deepcopy(ci_state[name])

  #Flatten 'extends' entries, only presume the first level of inheritance is
  #important
  if 'extends' in entry:
    if not isinstance(entry['extends'], list):
      entry['extends'] = [ entry['extends'] ]

    for e in entry['extends']:
      recursive_dictionary_update(entry, ci_state[e])
    del entry['extends']
  return entry

def ci_stages_and_jobs(ci_state):
  stages = ci_state['stages']
  jobs = dict((s,[]) for s in stages)
  for key in ci_state:
    entry = flattened_entry_copy(ci_state, key)

    is_job = False
    if 'stage' in entry:
      stage = entry['stage']
      if stage in stages:
        is_job = True

    # if we have a job ( that isn't private )
    if is_job and not key.startswith('.'):
      # clean up the name
      clean_name = key
      if ':' in key:
        clean_name = key.split(':')[1]
      jobs[stage].append(clean_name)


  return jobs

def subset_yml(ci_state, stage, name):
  #given a stage and name generate a new yaml
  #file that only contains information for stage and name.
  #Does basic extend merging so that recreating the env is easier
  runner_yml = {}

  if stage+":"+name in ci_state:
    name = stage+":"+name

  runner_yml[name] = flattened_entry_copy(ci_state, name)
  return runner_yml

class CallMode(enum.Enum):
  call = 1
  output = 2


def subprocess_call_docker(cmd, cwd, mode=CallMode.call):
  system = platform.system()
  if (system == 'Windows') or (system == 'Darwin'):
    # Windows and MacOS run Docker in a VM, so they don't need sudo
    full_cmd = ['docker'] + cmd
  else:
    # Unix needs to run docker with root privileges
    full_cmd = ['sudo', 'docker'] + cmd
  print(" ".join(full_cmd), flush=True)

  if mode is CallMode.call:
    return subprocess.check_call(full_cmd, cwd=cwd)
  if mode is CallMode.output:
    return subprocess.check_output(full_cmd, cwd=cwd)

###############################################################################
#
#     User Command: 'list'
#
###############################################################################
def list_jobs(ci_file_path, *args):
  ci_state = load_ci_file(ci_file_path)
  jobs = ci_stages_and_jobs(ci_state)
  for key,values in jobs.items():
    print('Jobs for Stage:', key)
    for v in values:
      print('\t',v)
    print('')


###############################################################################
#
#     User Command: 'build' | 'setup'
#
###############################################################################
def create_container(ci_file_path, *args):
  ci_state = load_ci_file(ci_file_path)
  ci_jobs = ci_stages_and_jobs(ci_state)
  stage,name = extract_stage_job_from_cmdline(*args)

  if not stage in ci_jobs:
    print('Unable to find stage: ', stage)
    print('Valid stages are:', list(ci_jobs.keys()))
    exit(1)

  if not name in ci_jobs[stage]:
    print('Unable to find job: ', name)
    print('Valid jobs are:', ci_jobs[stage])
    exit(1)

  #we now have the relevant subset of the yml
  #fully expanded into a single definition
  subset = subset_yml(ci_state, stage, name)

  job_name = name
  if stage+":"+name in subset:
    job_name = stage+":"+name
  runner_name = stage+":"+name

  runner = subset[job_name]
  src_dir = get_root_dir()
  gitlab_env = [ k + '="' + v + '"' for k,v in runner['variables'].items()]

  # propagate any https/http proxy info
  if os.getenv('http_proxy'):
    gitlab_env = [ 'http_proxy=' + os.getenv('http_proxy') ] + gitlab_env
  if os.getenv('https_proxy'):
    gitlab_env = [ 'https_proxy=' + os.getenv('https_proxy') ] + gitlab_env

  # The script and before_script could be anywhere!
  script_search_locations = [ci_state, subset, runner]
  for loc in script_search_locations:
    if 'before_script' in loc:
      before_script = [command.strip() for command in loc['before_script']]
    if 'script' in loc:
      script = [command.strip() for command in loc['script']]

  docker_template = string.Template('''
FROM $image
ENV GITLAB_CI=1 \
    GITLAB_CI_EMULATION=1 \
    CI_PROJECT_DIR=. \
    CI_JOB_NAME=$job_name
#Copy all of this project to the src directory
COPY . /src
ENV $gitlab_env
WORKDIR /src
RUN echo "$before_script" >> /setup-gitlab-env.sh && \
    echo "$script" >> /run-gitlab-stage.sh && \
    bash /setup-gitlab-env.sh
''')

  docker_content = docker_template.substitute(image=runner['image'],
                    job_name='local-build'+runner_name,
                    src_dir=src_dir,
                    gitlab_env= " ".join(gitlab_env),
                    before_script=" && ".join(before_script)
                                              .replace('\n', ' && ')
                                              .replace('"', '\\"'),
                    script=" && ".join(script).replace('\n', ' && ')
                                              .replace('"', '\\"'))

  # Write out the file
  docker_file = tempfile.NamedTemporaryFile(delete=False)
  docker_file.write(bytes(docker_content, 'utf-8'))
  docker_file.close()

  # now we need to run docker and build this image with a name equal to the
  # ci name, and the docker context to be the current git repo root dir so
  # we can copy the current project src automagically
  try:
    subprocess_call_docker(['build', '-f', docker_file.name, '-t', runner_name, src_dir],
                           cwd=src_dir)
  except subprocess.CalledProcessError:
    print('Unable to build the docker image for: ', runner_name)
    exit(1)
  finally:
    # remove the temp file
    os.remove(docker_file.name)

###############################################################################
#
#     User Command: 'help'
#
###############################################################################
def run_container(ci_file_path, *args):
  # Exec/Run ( https://docs.docker.com/engine/reference/commandline/exec/#run-docker-exec-on-a-running-container )
  src_dir = get_root_dir()
  stage,name = extract_stage_job_from_cmdline(*args)
  image_name = stage+':'+name

  try:
    cmd = ['run', '-itd', image_name]
    container_id = subprocess_call_docker(cmd, cwd=src_dir, mode=CallMode.output)
    container_id = str(container_id, 'utf-8')
    container_id= container_id.rstrip('\n')
  except subprocess.CalledProcessError:
    print('Unable to run the docker image for: ', image_name)
    exit(1)

  try:
    cmd = ['exec', '-it', container_id, 'bash']
    subprocess_call_docker(cmd, cwd=src_dir)
  except subprocess.CalledProcessError:
    print('Unable to attach an iteractive shell to : ', container_id)
  pass

  try:
    cmd = ['container', 'stop', container_id]
    subprocess_call_docker(cmd, cwd=src_dir)
  except subprocess.CalledProcessError:
    print('Unable to stop container: ', container_id)
  pass

###############################################################################
#
#     User Command: 'help'
#
###############################################################################
def help_usage(ci_file_path, *args):
  print('Setup gitlab-ci docker environments/state locally')
  print('Usage: reproduce_ci_env.py [command] [stage] <name>')
  print('\n')
  print('Commands:\n' + \
  '\n'+\
  '  list: List all stage and job names for gitlab-ci\n'+\
  '  create: build a docker container for this gitlab-ci job.\n'+\
  '        Will match the <stage> to docker repo, and <name> to the tag. \n' +\
  '        If no explicit <stage> is provided will default to `build` stage. \n' +\
  '  run: Launch an interactive shell inside the docker image\n' +\
  '        for a given stage:name with the correct environment and will automatically\n' +\
  '        run the associated stage script.\n'
  '        If no explicit <stage> is provided will default to `build` stage. \n')
  print('Example:\n' + \
  '\n'+\
  '  reproduce_ci_env create centos7\n'+\
  '  reproduce_ci_env run build:centos7\n')

###############################################################################
def main(argv):
  ci_file_path = os.path.join(get_root_dir(), '.gitlab-ci.yml')
  if len(argv) == 0:
    help_usage( ci_file_path  )
    exit(1)
  if len(argv) > 3:
    help_usage( ci_file_path  )
    exit(1)

  #commands we want
  # - list
  # -- list all 'jobs'
  # - create | setup
  # -- create a docker image that represents a given stage:name
  # - run | exec
  # -- run the script for the stage:name inside the correct docker image
  #    and provide an interactive shell
  # -- help
  #setup arg function table
  commands = {
    'list': list_jobs,
    'create': create_container,
    'setup': create_container,
    'exec': run_container,
    'run': run_container,
    'help': help_usage
    }
  if argv[0] in commands:
    #splat the subset of the vector so they are separate call parameters
    commands[argv[0]]( ci_file_path, *argv[1:3] )
  else:
    commands['help']( ci_file_path )
    exit(1)
  exit(0)

if __name__ == '__main__':
  main(sys.argv[1:])
