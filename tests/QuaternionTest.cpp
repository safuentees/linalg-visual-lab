//
// Quaternion unit tests using Google Test
//
// Run this test executable separately from the main app.
// In CLion: select "quaternion_tests" from the run configuration dropdown.
//

#include <gtest/gtest.h>
#include "math/Quaternion.h"
#include <cmath>

namespace {

// Helper to check if two floats are approximately equal
constexpr float kEpsilon = 1e-6f;

bool approxEqual(float a, float b) {
    return std::fabs(a - b) < kEpsilon;
}

bool quatEqual(const math::Quat& q, float w, float x, float y, float z) {
    return approxEqual(q.w, w) && approxEqual(q.x, x) &&
           approxEqual(q.y, y) && approxEqual(q.z, z);
}

} // namespace

// =============================================================================
// Quaternion Multiplication Tests
// =============================================================================
// These verify the Hamilton product identities: i²=j²=k²=ijk=-1

TEST(QuaternionMultiply, IdentityTimesIdentity) {
    // 1 * 1 = 1
    math::Quat identity = {1, 0, 0, 0};
    math::Quat result = math::multiply(identity, identity);
    EXPECT_TRUE(quatEqual(result, 1, 0, 0, 0));
}

TEST(QuaternionMultiply, I_squared_equals_minus1) {
    // i² = -1
    math::Quat i = {0, 1, 0, 0};
    math::Quat result = math::multiply(i, i);
    EXPECT_TRUE(quatEqual(result, -1, 0, 0, 0));
}

TEST(QuaternionMultiply, J_squared_equals_minus1) {
    // j² = -1
    math::Quat j = {0, 0, 1, 0};
    math::Quat result = math::multiply(j, j);
    EXPECT_TRUE(quatEqual(result, -1, 0, 0, 0));
}

TEST(QuaternionMultiply, K_squared_equals_minus1) {
    // k² = -1
    math::Quat k = {0, 0, 0, 1};
    math::Quat result = math::multiply(k, k);
    EXPECT_TRUE(quatEqual(result, -1, 0, 0, 0));
}

TEST(QuaternionMultiply, I_times_J_equals_K) {
    // i * j = k
    math::Quat i = {0, 1, 0, 0};
    math::Quat j = {0, 0, 1, 0};
    math::Quat result = math::multiply(i, j);
    EXPECT_TRUE(quatEqual(result, 0, 0, 0, 1));
}

TEST(QuaternionMultiply, J_times_I_equals_minusK) {
    // j * i = -k  (non-commutative!)
    math::Quat j = {0, 0, 1, 0};
    math::Quat i = {0, 1, 0, 0};
    math::Quat result = math::multiply(j, i);
    EXPECT_TRUE(quatEqual(result, 0, 0, 0, -1));
}

TEST(QuaternionMultiply, J_times_K_equals_I) {
    // j * k = i
    math::Quat j = {0, 0, 1, 0};
    math::Quat k = {0, 0, 0, 1};
    math::Quat result = math::multiply(j, k);
    EXPECT_TRUE(quatEqual(result, 0, 1, 0, 0));
}

TEST(QuaternionMultiply, K_times_J_equals_minusI) {
    // k * j = -i
    math::Quat k = {0, 0, 0, 1};
    math::Quat j = {0, 0, 1, 0};
    math::Quat result = math::multiply(k, j);
    EXPECT_TRUE(quatEqual(result, 0, -1, 0, 0));
}

TEST(QuaternionMultiply, K_times_I_equals_J) {
    // k * i = j
    math::Quat k = {0, 0, 0, 1};
    math::Quat i = {0, 1, 0, 0};
    math::Quat result = math::multiply(k, i);
    EXPECT_TRUE(quatEqual(result, 0, 0, 1, 0));
}

TEST(QuaternionMultiply, I_times_K_equals_minusJ) {
    // i * k = -j
    math::Quat i = {0, 1, 0, 0};
    math::Quat k = {0, 0, 0, 1};
    math::Quat result = math::multiply(i, k);
    EXPECT_TRUE(quatEqual(result, 0, 0, -1, 0));
}

// =============================================================================
// TODO: Add tests for normalize, conjugate, fromAxisAngle, quatToMat4
// as you implement them
// =============================================================================
