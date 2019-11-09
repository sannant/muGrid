/**
 * @file   bind_py_solvers.cc
 *
 * @author Till Junge <till.junge@epfl.ch>
 *
 * @date   09 Jan 2018
 *
 * @brief  python bindings for the muSpectre solvers
 *
 * Copyright © 2018 Till Junge
 *
 * µSpectre is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * µSpectre is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with µSpectre; see the file COPYING. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * * Boston, MA 02111-1307, USA.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with proprietary FFT implementations or numerical libraries, containing parts
 * covered by the terms of those libraries' licenses, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 *
 */

#include "common/muSpectre_common.hh"
#include "solver/solvers.hh"
#include "solver/solver_cg.hh"
#include "solver/solver_eigen.hh"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>

using muSpectre::Dim_t;
using muSpectre::OptimizeResult;
using muSpectre::Real;
using muSpectre::Uint;
using pybind11::literals::operator""_a;
using muSpectre::IsStrainInitialised;
namespace py = pybind11;

/**
 * Solvers instanciated for cells with equal spatial and material dimension
 */

template <class Solver>
void add_iterative_solver_helper(py::module & mod, std::string name) {
  py::class_<Solver, typename Solver::Parent>(mod, name.c_str())
      .def(py::init<muSpectre::NCell &, Real, Uint, bool>(), "cell"_a, "tol"_a,
           "maxiter"_a, "verbose"_a = false)
    .def_property_readonly("name", &Solver::get_name);
}

void add_iterative_solver(py::module & mod) {
  std::stringstream name{};
  name << "SolverBase";
  py::class_<muSpectre::SolverBase>(mod, name.str().c_str());
  add_iterative_solver_helper<muSpectre::SolverCG>(mod, "SolverCG");
  add_iterative_solver_helper<muSpectre::SolverCGEigen>(mod, "SolverCGEigen");
  add_iterative_solver_helper<muSpectre::SolverGMRESEigen>(mod,
                                                           "SolverGMRESEigen");
  add_iterative_solver_helper<muSpectre::SolverBiCGSTABEigen>(
      mod, "SolverBiCGSTABEigen");
  add_iterative_solver_helper<muSpectre::SolverDGMRESEigen>(
      mod, "SolverDGMRESEigen");
  add_iterative_solver_helper<muSpectre::SolverMINRESEigen>(
      mod, "SolverMINRESEigen");
}

void add_newton_cg_helper(py::module & mod) {
  const char name[]{"newton_cg"};
  using solver = muSpectre::SolverBase;
  using grad = py::EigenDRef<Eigen::MatrixXd>;
  using grad_vec = muSpectre::LoadSteps_t;

  py::enum_<IsStrainInitialised>(mod, "IsStrainInitialised")
      .value("Yes", IsStrainInitialised::True)
      .value("No", IsStrainInitialised::False);

  mod.def(name,
          [](muSpectre::NCell & s, const grad & g, solver & so, Real nt,
             Real eqt, Dim_t verb,
             IsStrainInitialised strain_init) -> OptimizeResult {
            Eigen::MatrixXd tmp{g};
            return newton_cg(s, tmp, so, nt, eqt, verb, strain_init);
          },
          "cell"_a, "ΔF₀"_a, "solver"_a, "newton_tol"_a, "equil_tol"_a,
          "verbose"_a = 0,
          "IsStrainInitialised"_a = IsStrainInitialised::False);
  mod.def(name,
          [](muSpectre::NCell & s, const grad_vec & g, solver & so, Real nt,
             Real eqt, Dim_t verb,
             IsStrainInitialised strain_init) -> std::vector<OptimizeResult> {
            return newton_cg(s, g, so, nt, eqt, verb, strain_init);
          },
          "cell"_a, "ΔF₀"_a, "solver"_a, "newton_tol"_a, "equilibrium_tol"_a,
          "verbose"_a = 0,
          "IsStrainInitialised"_a = IsStrainInitialised::False);
}

void add_de_geus_helper(py::module & mod) {
  const char name[]{"de_geus"};
  using solver = muSpectre::SolverBase;
  using grad = py::EigenDRef<Eigen::MatrixXd>;
  using grad_vec = muSpectre::LoadSteps_t;

  mod.def(name,
          [](muSpectre::NCell & s, const grad & g, solver & so, Real nt,
             Real eqt, Dim_t verb) -> OptimizeResult {
            Eigen::MatrixXd tmp{g};
            return de_geus(s, tmp, so, nt, eqt, verb);
          },
          "cell"_a, "ΔF₀"_a, "solver"_a, "newton_tol"_a, "equilibrium_tol"_a,
          "verbose"_a = 0);
  mod.def(name,
          [](muSpectre::NCell & s, const grad_vec & g, solver & so, Real nt,
             Real eqt, Dim_t verb) -> std::vector<OptimizeResult> {
            return de_geus(s, g, so, nt, eqt, verb);
          },
          "cell"_a, "ΔF₀"_a, "solver"_a, "newton_tol"_a, "equilibrium_tol"_a,
          "verbose"_a = 0);
}

void add_solver_helper(py::module & mod) {
  add_newton_cg_helper(mod);
  add_de_geus_helper(mod);
}

void add_solvers(py::module & mod) {
  auto solvers{mod.def_submodule("solvers")};
  solvers.doc() = "bindings for solvers";

  py::class_<OptimizeResult>(mod, "OptimizeResult")
      .def_readwrite("grad", &OptimizeResult::grad)
      .def_readwrite("stress", &OptimizeResult::stress)
      .def_readwrite("success", &OptimizeResult::success)
      .def_readwrite("status", &OptimizeResult::status)
      .def_readwrite("message", &OptimizeResult::message)
      .def_readwrite("nb_it", &OptimizeResult::nb_it)
      .def_readwrite("nb_fev", &OptimizeResult::nb_fev)
      .def_readwrite("formulation", &OptimizeResult::formulation);

  add_iterative_solver(solvers);

  add_solver_helper(solvers);
}
