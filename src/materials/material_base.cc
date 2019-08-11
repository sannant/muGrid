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

  //----------------------------------------------------------------------------//
  template <Dim_t DimS, Dim_t DimM>
  MaterialBase<DimS, DimM>::MaterialBase(const std::string & name,
                                         const Dim_t & spatial_dimension,
                                         const Dim_t & nb_quad_pts)
      : name(name), internal_fields{spatial_dimension, nb_quad_pts} {
    static_assert((DimM == oneD) || (DimM == twoD) || (DimM == threeD),
                  "only 1, 2, or threeD supported");
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  const std::string & MaterialBase<DimS, DimM>::get_name() const {
    return this->name;
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  void MaterialBase<DimS, DimM>::add_pixel(const size_t & global_index) {
    this->internal_fields.add_pixel(global_index);
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  void MaterialBase<DimS, DimM>::add_pixel_split(const Ccoord & local_ccoord,
                                                 Real ratio) {
    auto & this_mat = static_cast<MaterialBase &>(*this);
    this_mat.add_pixel(local_ccoord);
    this->assigned_ratio.value().get().push_back(ratio);
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  void
  MaterialBase<DimS, DimM>::allocate_optional_fields(SplitCell is_cell_split) {
    if (is_cell_split == SplitCell::simple) {
      this->assigned_ratio =
          muGrid::make_field<MScalarField_t>("ratio", this->internal_fields);
    }
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  void MaterialBase<DimS, DimM>::compute_stresses(const Field_t & F,
                                                  Field_t & P, Formulation form,
                                                  SplitCell is_cell_split) {
    this->compute_stresses(StrainField_t::check_ref(F),
                           StressField_t::check_ref(P), form, is_cell_split);
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  void MaterialBase<DimS, DimM>::get_assigned_ratios(
      std::vector<Real> & pixel_assigned_ratios, Ccoord subdomain_resolutions,
      Ccoord subdomain_locations) {
    auto assigned_ratio_mapped = this->assigned_ratio.value().get().get_map();
    for (auto && key_val :
         this->assigned_ratio.value().get().get_map().enumerate()) {
      const auto & ccoord = std::get<0>(key_val);
      const auto & val = std::get<1>(key_val);
      auto index = muGrid::CcoordOps::get_index(subdomain_resolutions,
                                                subdomain_locations, ccoord);
      pixel_assigned_ratios[index] += val;
    }
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  Real MaterialBase<DimS, DimM>::get_assigned_ratio(Ccoord pixel) {
    return this->assigned_ratio.value().get().get_map()[pixel];
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  auto MaterialBase<DimS, DimM>::get_assigned_ratio_field()
      -> MScalarField_t & {
    return this->assigned_ratio.value().get();
  }

  //----------------------------------------------------------------------------//
  template <Dim_t DimS, Dim_t DimM>
  void MaterialBase<DimS, DimM>::compute_stresses_tangent(
      const Field_t & F, Field_t & P, Field_t & K, Formulation form,
      SplitCell is_cell_split) {
    this->compute_stresses_tangent(
        StrainField_t::check_ref(F), StressField_t::check_ref(P),
        TangentField_t::check_ref(K), form, is_cell_split);
  }

  template <Dim_t DimS, Dim_t DimM>
  auto MaterialBase<DimS, DimM>::get_real_field(std::string field_name)
      -> EigenMap {
    if (not this->internal_fields.field_exists(field_name)) {
      std::stringstream err{};
      err << "Field '" << field_name << "' does not exist in material '"
          << this->name << "'.";
      throw muGrid::FieldCollectionError(err.str());
    }
    auto & field{this->internal_fields.get_field(field_name)};
    if (field.get_stored_typeid() != typeid(Real)) {
      std::stringstream err{};
      err << "Field '" << field_name << "' is not real-valued";
      throw muGrid::FieldCollectionError(err.str());
    }

    return static_cast<muGrid::RealNField &>(field).eigen_quad_pt();
  }

  /* ---------------------------------------------------------------------- */
  // template <Dim_t DimS, Dim_t DimM>
  // std::vector<std::string> MaterialBase<DimS, DimM>::list_fields() const {
  //   return this->internal_fields.list_fields();
  // }

  template class MaterialBase<2, 2>;
  template class MaterialBase<2, 3>;
  template class MaterialBase<3, 3>;

}  // namespace muSpectre
