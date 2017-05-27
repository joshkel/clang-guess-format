#!/usr/bin/env python

import editdistance
import json
import os
import subprocess
import sys

CLANG_VERSION = '3.9.1'

with open(CLANG_VERSION + '.json') as f:
    doc = json.load(f)

filename = sys.argv[1]

with open(filename) as f:
    orig_source = f.read()

for style in ['LLVM', 'Google', 'Chromium', 'Mozilla', 'WebKit']:
    formatted_source = subprocess.check_output(['clang-format-3.9', filename, '--style={BasedOnStyle: %s}' % style], universal_newlines=True)
    print('%s: %i' % (style, editdistance.eval(orig_source, formatted_source)))
