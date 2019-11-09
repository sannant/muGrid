/**
 * @file   material_linear_elastic_generic1.cc
 *
 * @author Till Junge <till.junge@altermail.ch>
 *
 * @date   21 Sep 2018
 *
 * @brief  implementation for MaterialLinearElasticGeneric
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

#include "materials/material_linear_elastic_generic1.hh"
#include "common/voigt_conversion.hh"

namespace muSpectre {

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimM>
  MaterialLinearElasticGeneric1<DimM>::MaterialLinearElasticGeneric1(
      const std::string & name, const Dim_t & spatial_dimension,
      const Dim_t & nb_quad_pts, const CInput_t & C_voigt)
      : Parent{name, spatial_dimension, nb_quad_pts},
        C_holder{std::make_unique<muGrid::T4Mat<Real, DimM>>()},
        C{*this->C_holder} {
    using muGrid::get;
    using VC_t = VoigtConversion<DimM>;
    constexpr Dim_t VSize{vsize(DimM)};
    if (not(C_voigt.rows() == VSize) or not(C_voigt.cols() == VSize)) {
      std::stringstream err_str{};
      err_str << "The stiffness tensor should be input as a " << VSize << " × "
              << VSize << " Matrix in Voigt notation. You supplied"
              << " a " << C_voigt.rows() << " × " << C_voigt.cols()
              << " matrix";
    }

    const auto & sym_mat{VC_t::get_sym_mat()};
    for (int i{0}; i < DimM; ++i) {
      for (int j{0}; j < DimM; ++j) {
        for (int k{0}; k < DimM; ++k) {
          for (int l{0}; l < DimM; ++l) {
            get(*this->C_holder, i, j, k, l) =
                C_voigt(sym_mat(i, j), sym_mat(k, l));
          }
        }
      }
    }
  }

  template class MaterialLinearElasticGeneric1<twoD>;
  template class MaterialLinearElasticGeneric1<threeD>;

}  // namespace muSpectre
