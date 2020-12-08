/**
 * @file   krylov_solver_preconditioned_base.cc
 *
 * @author Till Junge <till.junge@altermail.ch>
 *
 * @date   30 Aug 2020
 *
 * @brief  implementation for Krylov solver with preconditioner
 *
 * Copyright © 2020 Till Junge
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
 * Boston, MA 02111-1307, USA.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with proprietary FFT implementations or numerical libraries, containing parts
 * covered by the terms of those libraries' licenses, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 *
 */

#include "krylov_solver_preconditioned_base.hh"

namespace muSpectre {

  /* ---------------------------------------------------------------------- */
  KrylovSolverPreconditionedBase::KrylovSolverPreconditionedBase(
      std::shared_ptr<MatrixAdaptable> matrix_adaptable,
      std::shared_ptr<MatrixAdaptable> preconditioner_adaptable,
      const Real & tol, const Uint & maxiter, const Verbosity & verbose)
      : Parent{matrix_adaptable, tol, maxiter, verbose},
        preconditioner_holder{preconditioner_adaptable},
        preconditioner{preconditioner_adaptable->get_adaptor()} {}

  /* ---------------------------------------------------------------------- */
  KrylovSolverPreconditionedBase::KrylovSolverPreconditionedBase(
      const Real & tol, const Uint & maxiter, const Verbosity & verbose)
      : Parent{tol, maxiter, verbose} {}

  /* ---------------------------------------------------------------------- */
  void KrylovSolverPreconditionedBase::set_preconditioner(
      std::shared_ptr<MatrixAdaptable> preconditioner_adaptable) {
    this->preconditioner_holder = preconditioner_adaptable;
    this->preconditioner = this->preconditioner_holder->get_adaptor();
  }

}  // namespace muSpectre
