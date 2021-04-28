/**
 * @file   krylov_solver_trust_region_pcg.cc
 *
 * @author Till Junge <till.junge@altermail.ch>
 * @author Ali Falsafi <ali.falsafi@epfl.ch>
 *
 * @date   28 Aug 2020
 *
 * @brief  Implementation for preconditioned conjugate gradient solver
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

#include "krylov_solver_trust_region_pcg.hh"
#include "matrix_adaptor.hh"

#include <iomanip>

namespace muSpectre {

  /* ---------------------------------------------------------------------- */
  KrylovSolverTrustRegionPCG::KrylovSolverTrustRegionPCG(
      std::shared_ptr<MatrixAdaptable> matrix_holder,
      std::shared_ptr<MatrixAdaptable> inv_preconditioner, const Real & tol,
      const Uint & maxiter, const Real & trust_region,
      const Verbosity & verbose, const ResetCG & reset,
      const Uint & reset_iter_count)
      : Parent{matrix_holder, tol,   maxiter,         trust_region,
               verbose,       reset, reset_iter_count},
        FeaturesPC{inv_preconditioner}, r_k(this->get_nb_dof()),
        y_k(this->get_nb_dof()), p_k(this->get_nb_dof()),
        Ap_k(this->get_nb_dof()), x_k(this->get_nb_dof()) {}

  /* ---------------------------------------------------------------------- */
  KrylovSolverTrustRegionPCG::KrylovSolverTrustRegionPCG(
      const Real & tol, const Uint & maxiter, const Real & trust_region,
      const Verbosity & verbose, const ResetCG & reset,
      const Uint & reset_iter_count)
      : Parent{tol, maxiter, trust_region, verbose, reset, reset_iter_count},
        r_k{}, y_k{}, p_k{}, Ap_k{}, x_k{} {}

  /* ---------------------------------------------------------------------- */
  auto KrylovSolverTrustRegionPCG::solve(const ConstVector_ref rhs)
      -> Vector_map {
    if (this->matrix_ptr.expired()) {
      std::stringstream error_message{};
      error_message << "The system matrix has been destroyed. Did you set the "
                       "matrix using a weak_ptr instead of a shared_ptr?";
      throw SolverError{error_message.str()};
    }

    // Following implementation of algorithm 5.3 in Nocedal's
    // Numerical Optimization (p. 119)

    // reset the on_bound flag of the Krylov solver
    this->is_on_bound = false;

    this->x_k.setZero();
    Real trust_region2{this->trust_region * this->trust_region};

    /* initialisation:
     *           Set r₀ ← Ax₀-b
     *           Set y₀ ← M⁻¹r₀
     *           Set p₀ ← -y₀, k ← 0 */
    this->r_k = this->matrix * this->x_k - rhs;
    this->y_k = this->preconditioner * this->r_k;
    this->p_k = -this->y_k;

    Real rdr{this->comm.sum(this->r_k.squaredNorm())};
    Real rdy{this->comm.sum(this->r_k.dot(this->y_k))};
    Real rhs_norm2{this->comm.sum(rhs.squaredNorm())};

    if (rhs_norm2 == 0) {
      std::stringstream msg{};
      msg << "You are invoking conjugate gradient"
          << "solver with absolute zero RHS.\n"
          << "Please check the load steps of your problem "
          << "to ensure nothing is missed.\n"
          << "You might need to set equilibrium tolerance to a positive "
          << "small value to avoid calling the conjugate gradient solver in "
          << "case of having zero RHS (relatively small RHS).\n"
          << std::endl;
      std::cout << "WARNING: ";
      std::cout << msg.str();
      this->convergence = Convergence::ReachedTolerance;
      return Vector_map(this->x_k.data(), this->x_k.size());
    }

    if (this->verbose > Verbosity::Silent && this->comm.rank() == 0) {
      std::cout << "Norm of rhs in preconditioned CG = " << rhs_norm2
                << std::endl;
    }

    // Multiplication with the norm of the right hand side to get a relative
    // convergence criterion
    Real rel_tol2 = muGrid::ipow(this->tol, 2) * rhs_norm2;

    size_t count_width{};  // for output formatting in verbose case
    if (this->verbose > Verbosity::Silent) {
      count_width = size_t(std::log10(this->maxiter)) + 1;
    }

    // because of the early termination criterion, we never count the last
    // iteration
    ++this->counter;
    Uint iter_counter{0};
    for (Uint i = 0; i < this->maxiter; ++i, ++this->counter, ++iter_counter) {
      this->Ap_k = matrix * this->p_k;
      Real pAp{this->comm.sum(this->p_k.dot(this->Ap_k))};

      if (pAp <= 0) {
        // Hessian is not positive definite, the minimizer is on the trust
        // region bound
        if (verbose > Verbosity::Silent && comm.rank() == 0) {
          std::cout << "  CG finished, reason: Hessian is not positive "
                       "definite (pdAp:"
                    << pAp << ")" << std::endl;
        }
        this->convergence = Convergence::HessianNotPositiveDefinite;
        return this->bound(rhs);
      }

      /*                    rᵀₖyₖ
       *             αₖ ← ————––
       *                    pᵀₖApₖ                                  */
      Real alpha{rdy / pAp};

      //             xₖ₊₁ ← xₖ + αₖpₖ
      this->x_k += alpha * this->p_k;

      // we are exceeding the trust region, the minimizer is on the trust
      // region bound
      if (this->x_k.squaredNorm() >= trust_region2) {
        if (verbose > Verbosity::Silent && comm.rank() == 0) {
          std::cout << "  CG finished, reason: step exceeded trust region "
                       "bounds"
                    << std::endl;
        }
        this->convergence = Convergence::ExceededTrustRegionBound;
        return this->bound(rhs);
      }

      if (this->reset == ResetCG::gradient_orthogonality) {
        this->r_k_copy = this->r_k;
      }

      //             rₖ₊₁ ← rₖ + αₖApₖ
      this->r_k += alpha * this->Ap_k;
      rdr = this->comm.sum(this->r_k.squaredNorm());
      if (this->verbose > Verbosity::Silent && this->comm.rank() == 0) {
        std::cout << "  at CG step " << std::setw(count_width) << i
                  << ": |r|/|b| = " << std::setw(15)
                  << std::sqrt(rdr / rhs_norm2) << ", cg_tol = " << this->tol
                  << std::endl;
      }
      if (rdr < rel_tol2) {
        break;
      }

      //             yₖ₊₁ ← M⁻¹rₖ₊₁
      this->y_k = this->preconditioner * this->r_k;
      /*                     rᵀₖ₊₁yₖ₊₁
       *             βₖ₊₁ ← ————————–
       *                      rᵀₖyₖ                                */
      Real new_rdy{this->comm.sum(this->r_k.dot(this->y_k))};
      Real beta{new_rdy / rdy};

      //! CG reset worker
      auto && reset_cg{[&beta, this, &rhs]() {
        this->r_k = this->matrix * this->x_k - rhs;
        beta = 0.0;
      }};

      switch (this->reset) {
      case ResetCG::no_reset: {
        break;
      }
      case ResetCG::fixed_iter_count: {
        if (iter_counter > (this->get_nb_dof() / 4)) {
          reset_cg();
        } else {
          iter_counter++;
        }
        break;
      }
      case ResetCG::user_defined_iter_count: {
        if (this->reset_iter_count == 0) {
          throw SolverError(
              "Positive valued reset_iter_count is needed to perform user "
              "defined iteration count restart for the CG solver");
        }

        if (iter_counter > this->reset_iter_count) {
          reset_cg();
        } else {
          iter_counter++;
        }
        break;
      }
      case ResetCG::gradient_orthogonality: {
        if ((comm.sum(this->r_k.dot(this->r_k_copy)) / rdr) > 0.2) {
          reset_cg();
        }
        break;
      }
      case ResetCG::valid_direction: {
        if (comm.sum(this->r_k.dot(this->p_k)) > 0) {
          reset_cg();
        }
        break;
      }
      default: {
        throw SolverError("Unknown CG reset strategy ");
        break;
      }
      }

      rdy = new_rdy;

      //             pₖ₊₁ ← -yₖ₊₁ + βₖ₊₁pₖ
      this->p_k = -this->y_k + beta * this->p_k;
    }

    if (rdr < rel_tol2) {
      this->convergence = Convergence::ReachedTolerance;
    } else {
      std::stringstream err{};
      err << " After " << this->counter << " steps, the solver "
          << " FAILED with  |r|/|b| = " << std::setw(15)
          << std::sqrt(rdr / rhs_norm2) << ", cg_tol = " << this->tol
          << std::endl;
      throw ConvergenceError("Conjugate gradient has not converged." +
                             err.str());
    }
    return Vector_map(this->x_k.data(), this->x_k.size());
  }

  /* ---------------------------------------------------------------------- */
  std::string KrylovSolverTrustRegionPCG::get_name() const { return "PCG"; }

  /* ---------------------------------------------------------------------- */
  void KrylovSolverTrustRegionPCG::set_matrix(
      std::shared_ptr<MatrixAdaptable> matrix_adaptable) {
    Parent::set_matrix(matrix_adaptable);
    this->set_internal_arrays();
  }

  /* ---------------------------------------------------------------------- */
  void KrylovSolverTrustRegionPCG::set_matrix(
      std::weak_ptr<MatrixAdaptable> matrix_adaptable) {
    Parent::set_matrix(matrix_adaptable);
    this->set_internal_arrays();
  }

  /* ---------------------------------------------------------------------- */
  void KrylovSolverTrustRegionPCG::set_internal_arrays() {
    this->comm = this->matrix_ptr.lock()->get_communicator();
    auto && nb_dof{this->matrix_ptr.lock()->get_nb_dof()};
    this->r_k.resize(nb_dof);
    this->p_k.resize(nb_dof);
    this->Ap_k.resize(nb_dof);
    this->x_k.resize(nb_dof);
  }

  /* ---------------------------------------------------------------------- */
  void KrylovSolverTrustRegionPCG::set_preconditioner(
      std::shared_ptr<MatrixAdaptable> inv_preconditioner) {
    FeaturesPC::set_preconditioner(inv_preconditioner);
  }

  /* ---------------------------------------------------------------------- */
  auto KrylovSolverTrustRegionPCG::bound(const ConstVector_ref rhs)
      -> Vector_map {
    this->is_on_bound = true;
    Real trust_region2{this->trust_region * this->trust_region};

    Real pdp{this->comm.sum(this->p_k.squaredNorm())};
    Real xdx{this->comm.sum(this->x_k.squaredNorm())};
    Real pdx{this->comm.sum(this->p_k.dot(this->x_k))};
    Real tmp{sqrt(pdx * pdx - pdp * (xdx - trust_region2))};
    Real tau1{-(pdx + tmp) / pdp};
    Real tau2{-(pdx - tmp) / pdp};

    this->x_k += tau1 * this->p_k;
    Real m1{-rhs.dot(this->x_k) +
            0.5 * this->x_k.dot(this->matrix * this->x_k)};
    this->x_k += (tau2 - tau1) * this->p_k;
    Real m2{-rhs.dot(this->x_k) +
            0.5 * this->x_k.dot(this->matrix * this->x_k)};

    // check which direction is the minimizer
    if (m2 < m1) {
      return Vector_map(this->x_k.data(), this->x_k.size());
    } else {
      this->x_k += (tau1 - tau2) * this->p_k;
      return Vector_map(this->x_k.data(), this->x_k.size());
    }
  }

}  // namespace muSpectre
