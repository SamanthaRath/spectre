// Distributed under the MIT License.
// See LICENSE.txt for details

#include "Framework/TestingFramework.hpp"

#include <complex>
#include <cstddef>
#include <random>

#include "DataStructures/ComplexDataVector.hpp"
#include "DataStructures/DataVector.hpp"
#include "DataStructures/SpinWeighted.hpp"
#include "Framework/TestHelpers.hpp"
#include "Helpers/DataStructures/MakeWithRandomValues.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/Literals.hpp"
#include "Utilities/MakeWithValue.hpp"
#include "Utilities/SetNumberOfGridPoints.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TypeTraits.hpp"
#include "Utilities/TypeTraits/GetFundamentalType.hpp"

// tests for is_any_spin_weighted
static_assert(is_any_spin_weighted_v<SpinWeighted<int, 3>>,
              "failed testing is_any_spin_weighted");
static_assert(is_any_spin_weighted_v<SpinWeighted<DataVector, 0>>,
              "failed testing is_any_spin_weighted");
static_assert(not is_any_spin_weighted_v<ComplexDataVector>,
              "failed testing is_any_spin_weighted");

// tests for is_spin_weighted_of
static_assert(is_spin_weighted_of_v<DataVector, SpinWeighted<DataVector, 1>>,
              "failed testing is_spin_weighted_of");
static_assert(is_spin_weighted_of_v<ComplexDataVector,
                                    SpinWeighted<ComplexDataVector, -1>>,
              "failed testing is_spin_weighted_of");
static_assert(
    not is_spin_weighted_of_v<ComplexDataVector, SpinWeighted<DataVector, -2>>,
    "failed testing is_spin_weighted_of");
static_assert(not is_spin_weighted_of_v<ComplexDataVector, ComplexDataVector>,
              "failed testing is_spin_weighted_of");

// tests for is_spin_weighted_of_same_type
static_assert(is_spin_weighted_of_same_type_v<SpinWeighted<DataVector, -2>,
                                              SpinWeighted<DataVector, 1>>,
              "failed testing is_spin_weighted_of_same_type");
static_assert(
    is_spin_weighted_of_same_type_v<SpinWeighted<ComplexDataVector, 0>,
                                    SpinWeighted<ComplexDataVector, -1>>,
    "failed testing is_spin_weighted_of_same_type");
static_assert(not is_spin_weighted_of_same_type_v<ComplexDataVector,
                                                  SpinWeighted<DataVector, -2>>,
              "failed testing is_spin_weighted_of_same_type");
static_assert(
    not is_spin_weighted_of_same_type_v<SpinWeighted<ComplexDataVector, 1>,
                                        SpinWeighted<DataVector, 1>>,
    "failed testing is_spin_weighted_of_same_type");

namespace {
template <typename SpinWeightedType, typename CompatibleType>
void test_spinweights() {
  MAKE_GENERATOR(gen);
  UniformCustomDistribution<tt::get_fundamental_type_t<SpinWeightedType>>
      spin_weighted_dist{
          static_cast<tt::get_fundamental_type_t<SpinWeightedType>>(
              1.0),  // avoid divide by 0
          static_cast<tt::get_fundamental_type_t<SpinWeightedType>>(100.0)};

  UniformCustomDistribution<tt::get_fundamental_type_t<CompatibleType>>
      compatible_dist{
          static_cast<tt::get_fundamental_type_t<CompatibleType>>(
              1.0),  // avoid divide by 0
          static_cast<tt::get_fundamental_type_t<CompatibleType>>(100.0)};

  UniformCustomDistribution<size_t> size_dist{5, 10};
  const size_t size = size_dist(gen);

  auto spin_weight_0 =
      make_with_random_values<SpinWeighted<SpinWeightedType, 0>>(
          make_not_null(&gen), make_not_null(&spin_weighted_dist), size);
  const auto spin_weight_1 =
      make_with_random_values<SpinWeighted<SpinWeightedType, 1>>(
          make_not_null(&gen), make_not_null(&spin_weighted_dist), size);
  const auto spin_weight_m2 =
      make_with_random_values<SpinWeighted<SpinWeightedType, -2>>(
          make_not_null(&gen), make_not_null(&spin_weighted_dist), size);
  const auto no_spin_weight = make_with_random_values<SpinWeightedType>(
      make_not_null(&gen), make_not_null(&spin_weighted_dist), size);

  const auto exp_spin_weight_0 = exp(spin_weight_0);
  const auto sqrt_spin_weight_0 = sqrt(spin_weight_0);
  CHECK_ITERABLE_APPROX(exp_spin_weight_0.data(), exp(spin_weight_0.data()));
  CHECK_ITERABLE_APPROX(sqrt_spin_weight_0.data(), sqrt(spin_weight_0.data()));

  const auto serialized_and_deserialized_copy =
      serialize_and_deserialize(spin_weight_0);
  CHECK(spin_weight_0 == serialized_and_deserialized_copy);

  const auto compatible_spin_weight_0 =
      make_with_random_values<SpinWeighted<CompatibleType, 0>>(
          make_not_null(&gen), make_not_null(&compatible_dist), size);
  const auto compatible_spin_weight_1 =
      make_with_random_values<SpinWeighted<CompatibleType, 1>>(
          make_not_null(&gen), make_not_null(&compatible_dist), size);
  const auto compatible_spin_weight_m2 =
      make_with_random_values<SpinWeighted<CompatibleType, -2>>(
          make_not_null(&gen), make_not_null(&compatible_dist), size);
  const auto compatible_no_spin_weight =
      make_with_random_values<CompatibleType>(
          make_not_null(&gen), make_not_null(&compatible_dist), size);

  SpinWeighted<SpinWeightedType, 1> rvalue_assigned_spin_weight_1{
      spin_weight_1 + compatible_spin_weight_1};
  CHECK(rvalue_assigned_spin_weight_1.data() ==
        spin_weight_1.data() + compatible_spin_weight_1.data());
  rvalue_assigned_spin_weight_1 = spin_weight_1 - compatible_spin_weight_1;
  CHECK(rvalue_assigned_spin_weight_1.data() ==
        spin_weight_1.data() - compatible_spin_weight_1.data());

  SpinWeighted<SpinWeightedType, -2> lvalue_assigned_spin_weight_m2{
      spin_weight_m2};
  CHECK(lvalue_assigned_spin_weight_m2.data() == spin_weight_m2.data());
  lvalue_assigned_spin_weight_m2 = compatible_spin_weight_m2;
  CHECK(lvalue_assigned_spin_weight_m2.data() ==
        compatible_spin_weight_m2.data());

  // check compile-time spin values
  static_assert(decltype(spin_weight_0)::spin == 0,
                "assert failed for the spin of a spin-weight 0");
  static_assert(decltype(spin_weight_1)::spin == 1,
                "assert failed for the spin of a spin-weight 1");
  static_assert(decltype(compatible_spin_weight_0 / spin_weight_m2)::spin == 2,
                "assert failed for the spin of a spin-weight ratio.");
  static_assert(decltype(compatible_spin_weight_1 * spin_weight_1)::spin == 2,
                "assert failed for the spin of a spin-weight product.");

  // check that valid spin combinations work
  CHECK(spin_weight_0 + spin_weight_0 ==
        SpinWeighted<SpinWeightedType, 0>{spin_weight_0.data() +
                                          spin_weight_0.data()});
  CHECK(spin_weight_0 - no_spin_weight ==
        SpinWeighted<decltype(std::declval<SpinWeightedType>() -
                              std::declval<SpinWeightedType>()),
                     0>{spin_weight_0.data() - no_spin_weight});
  CHECK(spin_weight_1 * spin_weight_m2 ==
        SpinWeighted<SpinWeightedType, -1>{spin_weight_1.data() *
                                           spin_weight_m2.data()});
  CHECK(
      compatible_spin_weight_1 / spin_weight_m2 ==
      SpinWeighted<decltype(std::declval<CompatibleType>() /
                            std::declval<SpinWeightedType>()),
                   3>{compatible_spin_weight_1.data() / spin_weight_m2.data()});

  // check that plain data types act as spin 0
  CHECK(
      spin_weight_0 + no_spin_weight ==
      SpinWeighted<SpinWeightedType, 0>{spin_weight_0.data() + no_spin_weight});
  CHECK(compatible_no_spin_weight - spin_weight_0 ==
        SpinWeighted<decltype(std::declval<CompatibleType>() -
                              std::declval<SpinWeightedType>()),
                     0>{compatible_no_spin_weight - spin_weight_0.data()});
  CHECK(
      spin_weight_1 * no_spin_weight ==
      SpinWeighted<SpinWeightedType, 1>{spin_weight_1.data() * no_spin_weight});
  CHECK(no_spin_weight / spin_weight_m2 ==
        SpinWeighted<decltype(std::declval<SpinWeightedType>() /
                              std::declval<SpinWeightedType>()),
                     2>{no_spin_weight / spin_weight_m2.data()});
  CHECK(-spin_weight_1 ==
        SpinWeighted<SpinWeightedType, 1>(-spin_weight_1.data()));
  CHECK(spin_weight_m2 ==
        SpinWeighted<SpinWeightedType, -2>(spin_weight_m2.data()));

  SpinWeighted<SpinWeightedType, 0> sum = spin_weight_0 + spin_weight_0;
  spin_weight_0 += spin_weight_0;
  CHECK(spin_weight_0 == sum);

  SpinWeighted<SpinWeightedType, 0> difference = spin_weight_0 - no_spin_weight;
  spin_weight_0 -= no_spin_weight;
  CHECK(spin_weight_0 == difference);
}

using SpinWeightedTypePairs =
    tmpl::list<tmpl::list<std::complex<double>, double>,
               tmpl::list<ComplexDataVector, std::complex<double>>>;

SPECTRE_TEST_CASE("Unit.DataStructures.SpinWeighted",
                  "[DataStructures][Unit]") {
  tmpl::for_each<SpinWeightedTypePairs>([](auto x) {
    using type_pair = typename decltype(x)::type;
    test_spinweights<tmpl::front<type_pair>, tmpl::back<type_pair>>();
  });

  const SpinWeighted<ComplexDataVector, 1> size_created_spin_weight_1{5};
  CHECK(size_created_spin_weight_1.data().size() == 5);
  CHECK(size_created_spin_weight_1.size() == 5);

  const SpinWeighted<ComplexDataVector, 1> const_view;
  make_const_view(make_not_null(&const_view), size_created_spin_weight_1, 2, 2);
  CHECK(const_view.size() == 2);
  CHECK(const_view.data().data() ==
        size_created_spin_weight_1.data().data() + 2);

  SpinWeighted<ComplexDataVector, -2> size_and_value_created_spin_weight_m2{
      5, 4.0};
  CHECK(size_and_value_created_spin_weight_m2.data() ==
        ComplexDataVector{5, 4.0});
  CHECK(size_and_value_created_spin_weight_m2.size() == 5);

  // test destructive resize for vector type
  SpinWeighted<ComplexDataVector, 2> destructive_resize_check{5, 4.0};
  const SpinWeighted<ComplexDataVector, 2> destructive_resize_copy =
      destructive_resize_check;
  // check unchanged if no resize
  destructive_resize_check.destructive_resize(5);
  CHECK(destructive_resize_check == destructive_resize_copy);
  // check resize occurs if appropriate
  destructive_resize_check.destructive_resize(6);
  CHECK(destructive_resize_check != destructive_resize_copy);
  CHECK(destructive_resize_check.size() == destructive_resize_copy.size() + 1);

  CHECK(make_with_value<SpinWeighted<double, 2>>(2_st, 1.1) ==
        SpinWeighted<double, 2>(1.1));
  CHECK(make_with_value<SpinWeighted<DataVector, 2>>(2_st, 1.1) ==
        SpinWeighted<DataVector, 2>(DataVector{1.1, 1.1}));
  CHECK(make_with_value<SpinWeighted<DataVector, 2>>(
            SpinWeighted<DataVector, 2>(DataVector{1.2, 2.1}), 1.1) ==
        SpinWeighted<DataVector, 2>(DataVector{1.1, 1.1}));

  {
    SpinWeighted<double, 2> spin_double(1.1);
    set_number_of_grid_points(make_not_null(&spin_double), 2_st);
    CHECK(spin_double == SpinWeighted<double, 2>(1.1));
    set_number_of_grid_points(make_not_null(&spin_double), 1.2);
    CHECK(spin_double == SpinWeighted<double, 2>(1.1));
  }
  {
    SpinWeighted<DataVector, 2> spin_vector(DataVector{1.1, 1.2});
    set_number_of_grid_points(make_not_null(&spin_vector), 2_st);
    CHECK(spin_vector == SpinWeighted<DataVector, 2>(DataVector{1.1, 1.2}));
    set_number_of_grid_points(make_not_null(&spin_vector), 3_st);
    CHECK(spin_vector.size() == 3);
  }
}

// A macro which will static_assert fail when LHSTYPE OP RHSTYPE succeeds during
// SFINAE. Used to make sure we can't violate spin addition rules.
// clang-tidy: wants parens around macro argument, but that breaks macro
#define CHECK_TYPE_OPERATION_FAIL(TAG, OP, LHSTYPE, RHSTYPE)                  \
  template <typename T1, typename T2, typename = std::void_t<>>               \
  struct TAG : std::true_type {};                                             \
  template <typename T1, typename T2>                                         \
  struct TAG<T1, T2,                                                          \
             std::void_t<decltype(std::declval<T1>() OP std::declval<T2>())>> \
      : std::false_type {};                                                   \
  static_assert(TAG<LHSTYPE, RHSTYPE>::value, /*NOLINT*/                      \
                "static_assert failed, " #LHSTYPE #OP #RHSTYPE " had a type")

using SpinZero = SpinWeighted<double, 0>;
using SpinOne = SpinWeighted<double, 1>;
using SpinTwo = SpinWeighted<double, 2>;

CHECK_TYPE_OPERATION_FAIL(spin_check_1, +, SpinZero, SpinOne);
CHECK_TYPE_OPERATION_FAIL(spin_check_2, +, SpinOne, SpinTwo);
CHECK_TYPE_OPERATION_FAIL(spin_check_3, +,
                          decltype(std::declval<SpinZero>() *
                                   std::declval<SpinTwo>()),
                          SpinOne);
}  // namespace
