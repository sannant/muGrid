/**
 * @file   material_base.cc
 *
 * @author Till Junge <till.junge@epfl.ch>
 *
 * @date   01 Nov 2017
 *
 * @brief  implementation of material
 *
 * Copyright © 2017 Till Junge
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

#include "materials/material_base.hh"

#include <libmugrid/nfield.hh>
#include <libmugrid/nfield_typed.hh>

namespace muSpectre {

  /* ---------------------------------------------------------------------- */
  MaterialBase::MaterialBase(const std::string & name,
                             const Dim_t & spatial_dimension,
                             const Dim_t & material_dimension,
                             const Dim_t & nb_quad_pts)
      : name(name), internal_fields{spatial_dimension, nb_quad_pts},
        material_dimension{material_dimension} {
    if (not((this->material_dimension == oneD) ||
            (this->material_dimension == twoD) ||
            (this->material_dimension == threeD))) {
      throw MaterialError("only 1, 2, or threeD supported");
    }
  }

  /* ---------------------------------------------------------------------- */
  const std::string & MaterialBase::get_name() const { return this->name; }

  /* ---------------------------------------------------------------------- */
  void MaterialBase::add_pixel(const size_t & global_index) {
    this->internal_fields.add_pixel(global_index);
  }

  /* ---------------------------------------------------------------------- */
  void MaterialBase::add_pixel_split(const size_t & local_ccoord,
                                     const Real & ratio) {
    this->add_pixel(local_ccoord);
    this->assigned_ratio->get_field().push_back(ratio);
  }

  void MaterialBase::compute_stresses(const muGrid::NField & F,
                                      muGrid::NField & P, Formulation form,
                                      SplitCell is_cell_split) {
    const auto t2_dim{muGrid::ipow(this->material_dimension, 2)};
    const auto & real_F{muGrid::RealNField::safe_cast(F, t2_dim)};
    auto & real_P{muGrid::RealNField::safe_cast(P, t2_dim)};
    this->compute_stresses(real_F, real_P, form, is_cell_split);
  }

  /* ---------------------------------------------------------------------- */
  void MaterialBase::allocate_optional_fields(SplitCell is_cell_split) {
    if (is_cell_split == SplitCell::simple) {
      this->assigned_ratio = std::make_unique<
          muGrid::MappedScalarNField<Real, muGrid::Mapping::Mut>>(
          "ratio", this->internal_fields);
    }
  }
  /* ---------------------------------------------------------------------- */
  void MaterialBase::get_assigned_ratios(
      std::vector<Real> & quad_pt_assigned_ratios) {
    for (auto && tup : akantu::zip(this->get_quad_pt_indices(),
                                   this->assigned_ratio->get_map())) {
      const auto & index = std::get<0>(tup);
      const auto & val = std::get<1>(tup);
      quad_pt_assigned_ratios[index] += val;
    }
  }

  /* ---------------------------------------------------------------------- */
  Real MaterialBase::get_assigned_ratio(const size_t & pixel_id) {
    return this->assigned_ratio->get_map()[pixel_id];
  }

  // /* ----------------------------------------------------------------------
  // */ template <Dim_t DimS, Dim_t DimM> auto MaterialBase<DimS,
  // DimM>::get_assigned_ratio_field()
  //     -> MScalarField_t & {
  //   return this->assigned_ratio.value().get();
  // }

  /* ---------------------------------------------------------------------- */
  void MaterialBase::compute_stresses_tangent(const muGrid::NField & F,
                                              muGrid::NField & P,
                                              muGrid::NField & K,
                                              Formulation form,
                                              SplitCell is_cell_split) {
    const auto t2_dim{muGrid::ipow(this->material_dimension, 2)};
    const auto & real_F{muGrid::RealNField::safe_cast(F, t2_dim)};
    auto & real_P{muGrid::RealNField::safe_cast(P, t2_dim)};
    auto & real_K{muGrid::RealNField::safe_cast(K, muGrid::ipow(t2_dim, 2))};
    this->compute_stresses_tangent(real_F, real_P, real_K, form, is_cell_split);
  }

  /* ---------------------------------------------------------------------- */
  auto MaterialBase::get_pixel_indices() const ->
      typename muGrid::LocalNFieldCollection::PixelIndexIterable {
    return this->internal_fields.get_pixel_indices_fast();
  }

  /* ---------------------------------------------------------------------- */
  auto MaterialBase::get_quad_pt_indices() const ->
      typename muGrid::LocalNFieldCollection::IndexIterable {
    return this->internal_fields.get_quad_pt_indices();
  }

  /* ---------------------------------------------------------------------- */
  std::vector<std::string> MaterialBase::list_fields() const {
    return this->internal_fields.list_fields();
  }

  /* ---------------------------------------------------------------------- */
  void MaterialBase::initialise() {
    if (!this->is_initialised) {
      this->internal_fields.initialise();
      this->is_initialised = true;
    }
  }

}  // namespace muSpectre
