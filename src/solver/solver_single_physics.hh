/**
 * @file   solver_single_physics.hh
 *
 * @author Till Junge <till.junge@altermail.ch>
 *
 * @date   10 Jul 2020
 *
 * @brief  Base class for single physics solvers
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

#include "solver_base.hh"

#include <libmugrid/units.hh>

#ifndef SRC_SOLVER_SOLVER_SINGLE_PHYSICS_HH_
#define SRC_SOLVER_SOLVER_SINGLE_PHYSICS_HH_

namespace muSpectre {

  class SolverSinglePhysics : public SolverBase {
    using Parent = SolverBase;

   public:
    using EigenStrainOptFunc_ref = Parent::EigenStrainOptFunc_ref;

    //! Default constructor
    SolverSinglePhysics() = delete;

    //! constructor with units of input fields
    SolverSinglePhysics(std::shared_ptr<CellData> cell_data,
                        const muGrid::Verbosity & verbosity);

    //! Copy constructor
    SolverSinglePhysics(const SolverSinglePhysics & other) = delete;

    //! Move constructor
    SolverSinglePhysics(SolverSinglePhysics && other) = default;

    //! Destructor
    virtual ~SolverSinglePhysics() = default;

    //! Copy assignment operator
    SolverSinglePhysics & operator=(const SolverSinglePhysics & other) = delete;

    //! Move assignment operator
    SolverSinglePhysics & operator=(SolverSinglePhysics && other) = delete;

    using Parent::solve_load_increment;
    /**
     * solve for a single increment without having to specify units. This
     * function cannot handle eigenload. If you have an eigenload problem, use
     * `apply_load_increment(const LoadStep & load_step)`, thenn apply your
     * eigen load followed by `solve_load_increment()` (without argument).
     */
    template <class Derived>
    OptimizeResult solve_load_increment(
        const Eigen::MatrixBase<Derived> & load_step,
        EigenStrainOptFunc_ref eigen_strain_func = muGrid::nullopt) {
      LoadStep step{};
      step[this->domain] = load_step;
      return this->solve_load_increment(step, eigen_strain_func);
    }

    /**
     * check whether this is a mechanics problem (Information needed for
     * handling finite/small strainn distinction correctly)
     */
    bool is_mechanics() const;

    /**
     * evaluates and returns the stress for the currently set strain
     */
    const MappedField_t & evaluate_stress();
    using Parent::evaluate_stress;

    /**
     * evaluates and returns the stress and tangent moduli for the currently set
     * strain
     */
    std::tuple<const MappedField_t &, const MappedField_t &>
    evaluate_stress_tangent();
    using Parent::evaluate_stress_tangent;

   protected:
    muGrid::PhysicsDomain domain;
  };

}  // namespace muSpectre

#endif  // SRC_SOLVER_SOLVER_SINGLE_PHYSICS_HH_
