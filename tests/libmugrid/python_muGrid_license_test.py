#!/usr/bin/env python3
# -*- coding:utf-8 -*-
"""
@file python_muGrid_license_test.py

@author Ali Falsafi<ali.falsafi @epfl.ch>

@date 17 Sep 2019

@brief description

Copyright © 2018 Till Junge

µGrid is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3, or (at
your option) any later version.

µGrid is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with µGrid; see the file COPYING. If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

Additional permission under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or combining it
with proprietary FFT implementations or numerical libraries, containing parts
covered by the terms of those libraries' licenses, the licensors of this
Program grant you additional permission to convey the resulting work.
"""

import unittest
import sys
import os
sys.path.insert(1, os.path.join(sys.path[0], '..'))
import python_license_test as lic_test


# muGrid_sources = ["../src/libmugrid", "../tests/libmugrid"]

lic_paras = [" µGrid is free software; you can redistribute it and/or "
             "modify it under the terms of the GNU Lesser General Public "
             "License as published by the Free Software Foundation, either"
             " version 3, or (at your option) any later version. ",
             " µGrid is distributed in the hope that it will be useful,"
             " but WITHOUT ANY WARRANTY; without even the implied warranty of"
             " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the"
             " GNU Lesser General Public License for more details. ",
             " You should have received a copy of the GNU Lesser General"
             " Public License along with µGrid; see the file COPYING."
             " If not, write to the Free Software Foundation, Inc., 59"
             " Temple Place - Suite 330, Boston, MA 02111-1307, USA. ",
             " Additional permission under GNU GPL version 3 section 7 ",
             " If you modify this Program, or any covered work, by linking or"
             " combining it with proprietary FFT implementations or numerical"
             " libraries, containing parts covered by the terms of those"
             " libraries\' licenses, the licensors of this Program grant you"
             " additional permission to convey the resulting work. "]

py_lic_paras = ["µGrid is free software; you can redistribute it and/or\n"
                "modify it under the terms of the GNU Lesser General Public"
                " License as\npublished by the Free Software Foundation,"
                " either version 3, or (at\nyour option) any later version.",
                "µGrid is distributed in the hope that it will be useful,"
                " but\nWITHOUT ANY WARRANTY; without even the implied warranty"
                " of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See"
                " the GNU\nLesser General Public License for more details.",
                "You should have received a copy of the GNU Lesser General"
                " Public License\nalong with µGrid; see the file COPYING."
                " If not, write to the\nFree Software Foundation, Inc.,"
                " 59 Temple Place - Suite 330,\nBoston, MA 02111-1307, USA.",
                'Additional permission under GNU GPL version 3 section 7',
                "If you modify this Program, or any covered work, by linking"
                " or combining it\nwith proprietary FFT implementations or"
                " numerical libraries, containing parts\ncovered by the terms"
                " of those libraries' licenses, the licensors of this\nProgram"
                " grant you additional permission to convey the resulting"
                " work.\n"]


class CheckMuGridHeaderFiles():

    def test_muGrid_header_files(self, muGrid_sources):
        lic_test.header_license_test(muGrid_sources, lic_paras)


class CheckMuGridSourceFiles():

    def test_muGrid_source_files(self, muGrid_sources):
        lic_test.source_license_test(muGrid_sources, lic_paras)


class CheckMuGridPythonFiles():

    def test_muGrid_python_files(self, muGrid_sources):
        lic_test.python_license_test(muGrid_sources, py_lic_paras)


def main():
    muGrid_sources = lic_test.arg_parser.parse_args(sys.argv[1:])
    header_test_case = CheckMuGridHeaderFiles
    header_test_case.test_muGrid_header_files(header_test_case,
                                              muGrid_sources)
    source_test_case = CheckMuGridSourceFiles
    source_test_case.test_muGrid_source_files(source_test_case,
                                              muGrid_sources)

    python_test_case = CheckMuGridPythonFiles
    python_test_case.test_muGrid_python_files(python_test_case,
                                              muGrid_sources)


if __name__ == "__main__":
    main()
