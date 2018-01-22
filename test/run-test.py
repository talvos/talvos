# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

import os
import subprocess
import sys

if len(sys.argv) != 3:
    print 'Usage: python run-test.py TCF EXE'
    exit(1)

filename = sys.argv[1]

if not os.path.isfile(filename):
  print('TCF file not found')
  sys.exit(1)

test_dir  = os.path.dirname(os.path.realpath(filename))
test_file = os.path.basename(filename)
exe = os.path.realpath(sys.argv[2])

os.chdir(test_dir)

# Run talvos-cmd
cmd = [exe]
cmd.append(test_file)
proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
output = proc.communicate()
retval = proc.returncode

oln = 0
output = output[0].splitlines()

# Loop over lines in test file
tcf = open(test_file)
lines = tcf.read().splitlines()
for ln in range(len(lines)):
    line = lines[ln]

    if line.startswith('# CHECK '):
        pattern = line[8:]
        matched = False
        for o in range(oln, len(output)):
            if output[o] == pattern:
                matched = True
                oln = o
                break
        if not matched:
            print 'CHECK on line %d not found' % ln
            exit(1)
    elif line.startswith('# EXIT '):
        expected = int(line[7:])
        if retval != expected:
            print 'Exit code %d does not match expected value of %d' \
                % (retval, expected)
            exit(1)
