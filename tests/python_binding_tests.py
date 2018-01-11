#!/usr/bin/env python3
"""
file   python_binding_tests.py

@author Till Junge <till.junge@epfl.ch>

@date   09 Jan 2018

@brief  Unit tests for python bindings

@section LICENCE

Copyright © 2018 Till Junge

µSpectre is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3, or (at
your option) any later version.

µSpectre is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs; see the file COPYING. If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
"""

import unittest
import sys
import os
import numpy as np

sys.path.append(os.path.join(os.getcwd(), "src/language_bindings/python"))

try:
    import common
    import system
    import solvers
    import material
except ImportError as err:
    print(err)
    sys.exit(-1)

class SystemCheck(unittest.TestCase):
    def test_Construction(self):
        """
        Simple check for system constructors
        """
        resolution = [5,7]
        lengths = [5.2, 8.3]
        formulation = common.Formulation.small_strain
        try:
            sys = system.SystemFactory2d(resolution,
                                         lengths,
                                         formulation)
            mat = material.MaterialHooke2d.make(sys, "material", 210e9, .33)
        except Exception(err):
            print(err)
            raise err

class MaterialHooke2dCheck(unittest.TestCase):
    def setUp(self):
        self.resolution = [5,7]
        self.lengths = [5.2, 8.3]
        self.formulation = common.Formulation.small_strain
        self.sys = system.SystemFactory2d(self.resolution,
                                          self.lengths,
                                          self.formulation)
        self.mat = material.MaterialHooke2d.make(
            self.sys, "material", 210e9, .33)

    def test_add_material(self):
        self.mat.add_pixel([2,1])


class SolverCheck(unittest.TestCase):
    def setUp(self):
        self.resolution = [5,7]
        self.lengths = [5.2, 8.3]
        self.formulation = common.Formulation.small_strain
        self.sys = system.SystemFactory2d(self.resolution,
                                          self.lengths,
                                          self.formulation)
        self.hard = material.MaterialHooke2d.make(
            self.sys, "hard", 210e9, .33)
        self.soft = material.MaterialHooke2d.make(
            self.sys, "soft",  70e9, .33)

    def test_solve(self):
        for i, pixel in enumerate(self.sys):
            if i < 3:
                self.hard.add_pixel(pixel)
            else:
                self.soft.add_pixel(pixel)

        self.sys.initialise()
        tol = 1e-6
        Del0 = np.array([[0, .1],
                         [0,  0]])
        maxiter = 100
        verbose = 2
        # the following segfaults:
        #TODO solvers.de_geus2d(self.sys, Del0, self.formulation,
        #                  tol, tol, maxiter, verbose)




if __name__ == '__main__':
    unittest.main()
