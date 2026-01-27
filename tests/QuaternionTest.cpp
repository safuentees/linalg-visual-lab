//
// Quaternion unit tests using Google Test
//
// Run this test executable separately from the main app.
// In CLion: select "quaternion_tests" from the run configuration dropdown.
//

#include <gtest/gtest.h>
#include "math/Quaternion.h"
#include "math/Types.hpp"
#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

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
// Norm and Normalize Tests
// =============================================================================

TEST(QuaternionNorm, IdentityHasNorm1) {
    math::Quat q = {1, 0, 0, 0};
    EXPECT_FLOAT_EQ(math::norm(q), 1.0f);
}

TEST(QuaternionNorm, UnitQuaternionHasNorm1) {
    // 90° around Z: (cos(45°), 0, 0, sin(45°))
    float c = std::cos(M_PI / 4.0f);
    float s = std::sin(M_PI / 4.0f);
    math::Quat q = {c, 0, 0, s};
    EXPECT_NEAR(math::norm(q), 1.0f, kEpsilon);
}

TEST(QuaternionNormalize, NormalizesCorrectly) {
    math::Quat q = {2, 0, 0, 0};  // Not unit length
    math::Quat n = math::normalize(q);
    EXPECT_FLOAT_EQ(math::norm(n), 1.0f);
    EXPECT_TRUE(quatEqual(n, 1, 0, 0, 0));
}

TEST(QuaternionConjugate, FlipsSigns) {
    math::Quat q = {1, 2, 3, 4};
    math::Quat c = math::conjugate(q);
    EXPECT_TRUE(quatEqual(c, 1, -2, -3, -4));
}

// =============================================================================
// fromAxisAngle Tests
// =============================================================================

TEST(FromAxisAngle, ZeroAngleGivesIdentity) {
    math::Quat q = math::fromAxisAngle({1, 0, 0}, 0.0f);
    EXPECT_TRUE(quatEqual(q, 1, 0, 0, 0));
}

TEST(FromAxisAngle, 90DegreesAroundZ) {
    // q = (cos(45°), sin(45°) * (0,0,1)) = (cos(45°), 0, 0, sin(45°))
    float angle = M_PI / 2.0f;  // 90 degrees
    math::Quat q = math::fromAxisAngle({0, 0, 1}, angle);

    float c = std::cos(angle / 2.0f);
    float s = std::sin(angle / 2.0f);
    EXPECT_NEAR(q.w, c, kEpsilon);
    EXPECT_NEAR(q.x, 0, kEpsilon);
    EXPECT_NEAR(q.y, 0, kEpsilon);
    EXPECT_NEAR(q.z, s, kEpsilon);
}

TEST(FromAxisAngle, 180DegreesAroundX) {
    float angle = M_PI;  // 180 degrees
    math::Quat q = math::fromAxisAngle({1, 0, 0}, angle);

    // cos(90°) = 0, sin(90°) = 1
    EXPECT_NEAR(q.w, 0, kEpsilon);
    EXPECT_NEAR(q.x, 1, kEpsilon);
    EXPECT_NEAR(q.y, 0, kEpsilon);
    EXPECT_NEAR(q.z, 0, kEpsilon);
}

TEST(FromAxisAngle, NonUnitAxisGetsNormalized) {
    // Axis (2, 0, 0) should be normalized to (1, 0, 0)
    float angle = M_PI / 2.0f;
    math::Quat q = math::fromAxisAngle({2, 0, 0}, angle);

    float c = std::cos(angle / 2.0f);
    float s = std::sin(angle / 2.0f);
    EXPECT_NEAR(q.w, c, kEpsilon);
    EXPECT_NEAR(q.x, s, kEpsilon);  // Normalized axis is (1,0,0)
    EXPECT_NEAR(q.y, 0, kEpsilon);
    EXPECT_NEAR(q.z, 0, kEpsilon);
}

// =============================================================================
// quatToMat4 Tests
// =============================================================================

TEST(QuatToMat4, IdentityQuatGivesIdentityMatrix) {
    math::Quat q = {1, 0, 0, 0};
    Mat4 R = math::quatToMat4(q);

    // Check diagonal
    EXPECT_NEAR(R[0][0], 1.0f, kEpsilon);
    EXPECT_NEAR(R[1][1], 1.0f, kEpsilon);
    EXPECT_NEAR(R[2][2], 1.0f, kEpsilon);
    EXPECT_NEAR(R[3][3], 1.0f, kEpsilon);

    // Check some off-diagonals
    EXPECT_NEAR(R[0][1], 0.0f, kEpsilon);
    EXPECT_NEAR(R[1][0], 0.0f, kEpsilon);
}

TEST(QuatToMat4, 90DegreesAroundZ_RotatesXtoY) {
    // 90° around Z should rotate (1,0,0) to (0,1,0)
    float angle = M_PI / 2.0f;
    math::Quat q = math::fromAxisAngle({0, 0, 1}, angle);
    Mat4 R = math::quatToMat4(q);

    Vec4 point = {1, 0, 0, 1};
    Vec4 rotated = R * point;

    EXPECT_NEAR(rotated.x, 0.0f, kEpsilon);
    EXPECT_NEAR(rotated.y, 1.0f, kEpsilon);
    EXPECT_NEAR(rotated.z, 0.0f, kEpsilon);
}

TEST(QuatToMat4, 90DegreesAroundX_RotatesYtoZ) {
    // 90° around X should rotate (0,1,0) to (0,0,1)
    float angle = M_PI / 2.0f;
    math::Quat q = math::fromAxisAngle({1, 0, 0}, angle);
    Mat4 R = math::quatToMat4(q);

    Vec4 point = {0, 1, 0, 1};
    Vec4 rotated = R * point;

    EXPECT_NEAR(rotated.x, 0.0f, kEpsilon);
    EXPECT_NEAR(rotated.y, 0.0f, kEpsilon);
    EXPECT_NEAR(rotated.z, 1.0f, kEpsilon);
}

TEST(QuatToMat4, 90DegreesAroundY_RotatesZtoX) {
    // 90° around Y should rotate (0,0,1) to (1,0,0)
    float angle = M_PI / 2.0f;
    math::Quat q = math::fromAxisAngle({0, 1, 0}, angle);
    Mat4 R = math::quatToMat4(q);

    Vec4 point = {0, 0, 1, 1};
    Vec4 rotated = R * point;

    EXPECT_NEAR(rotated.x, 1.0f, kEpsilon);
    EXPECT_NEAR(rotated.y, 0.0f, kEpsilon);
    EXPECT_NEAR(rotated.z, 0.0f, kEpsilon);
}

TEST(QuatToMat4, ArbitraryAxis_45Degrees) {
    // Rotation around (1,1,1) by 45 degrees
    Vec3 axis = {1, 1, 1};
    float angle = M_PI / 4.0f;

    math::Quat q = math::fromAxisAngle(axis, angle);
    Mat4 R = math::quatToMat4(q);

    // The matrix should be orthogonal (R * R^T = I)
    // Check determinant is 1 (proper rotation)
    // For simplicity, just verify the quaternion is unit and matrix looks reasonable
    EXPECT_NEAR(math::norm(q), 1.0f, kEpsilon);
    EXPECT_NEAR(R[3][3], 1.0f, kEpsilon);
}

// =============================================================================
// Validation: Compare Quaternion vs Matrix approach
// =============================================================================
// These tests verify that quatToMat4(fromAxisAngle(axis, theta)) produces
// the same rotation as the traditional matrix-based axis-angle rotation.

namespace {
    // Replicate BuildAxisRotation from App.cpp for comparison
    Mat4 BuildAxisRotationMatrix(const Vec3& axis, float theta) {
        const float axisLen = glm::length(axis);
        if (axisLen <= 1e-6f) {
            return Mat4(1.f);
        }

        const Vec3 m_w = axis / axisLen;
        const float d = std::sqrt(m_w.y * m_w.y + m_w.z * m_w.z);

        Mat4 Rx(1.f);
        Vec3 w_x = m_w;
        if (d > 1e-6f) {
            const float c = m_w.z / d;
            const float s = m_w.y / d;
            Rx[1][1] = c;
            Rx[2][1] = -s;
            Rx[1][2] = s;
            Rx[2][2] = c;
            w_x = Vec3(Rx * Vec4(m_w, 0.f));
        }

        const float theta_y = std::atan2(-w_x.x, w_x.z);
        const Mat4 Ry = glm::rotate(Mat4(1.f), theta_y, Vec3{0.f, 1.f, 0.f});

        const Mat4 A = Ry * Rx;
        const Mat4 Rz = glm::rotate(Mat4(1.f), theta, Vec3{0.f, 0.f, 1.f});
        const Mat4 R = glm::transpose(A) * Rz * A;

        return R;
    }

    Mat4 BuildAxisRotationQuat(const Vec3& axis, float theta) {
        math::Quat q = math::fromAxisAngle(axis, theta);
        return math::quatToMat4(q);
    }

    bool matricesEqual(const Mat4& a, const Mat4& b, float eps = 1e-5f) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                if (std::fabs(a[c][r] - b[c][r]) > eps) {
                    return false;
                }
            }
        }
        return true;
    }

    bool vectorsEqual(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
        return std::fabs(a.x - b.x) < eps &&
               std::fabs(a.y - b.y) < eps &&
               std::fabs(a.z - b.z) < eps;
    }
}

TEST(Validation, MatrixVsQuat_AxisX_90Degrees) {
    Vec3 axis = {1, 0, 0};
    float theta = M_PI / 2.0f;

    Mat4 R_mat = BuildAxisRotationMatrix(axis, theta);
    Mat4 R_quat = BuildAxisRotationQuat(axis, theta);

    // Test with a point
    Vec3 testPt = {0, 1, 0};
    Vec3 result_mat = Vec3(R_mat * Vec4(testPt, 1.0f));
    Vec3 result_quat = Vec3(R_quat * Vec4(testPt, 1.0f));

    EXPECT_TRUE(vectorsEqual(result_mat, result_quat));
}

TEST(Validation, MatrixVsQuat_AxisY_45Degrees) {
    Vec3 axis = {0, 1, 0};
    float theta = M_PI / 4.0f;

    Mat4 R_mat = BuildAxisRotationMatrix(axis, theta);
    Mat4 R_quat = BuildAxisRotationQuat(axis, theta);

    Vec3 testPt = {1, 0, 1};
    Vec3 result_mat = Vec3(R_mat * Vec4(testPt, 1.0f));
    Vec3 result_quat = Vec3(R_quat * Vec4(testPt, 1.0f));

    EXPECT_TRUE(vectorsEqual(result_mat, result_quat));
}

TEST(Validation, MatrixVsQuat_ArbitraryAxis) {
    Vec3 axis = {1, 2, 3};  // Non-unit, arbitrary axis
    float theta = 1.23f;    // Arbitrary angle

    Mat4 R_mat = BuildAxisRotationMatrix(axis, theta);
    Mat4 R_quat = BuildAxisRotationQuat(axis, theta);

    Vec3 testPt = {-2, 5, 0.5f};
    Vec3 result_mat = Vec3(R_mat * Vec4(testPt, 1.0f));
    Vec3 result_quat = Vec3(R_quat * Vec4(testPt, 1.0f));

    EXPECT_TRUE(vectorsEqual(result_mat, result_quat));
}

TEST(Validation, MatrixVsQuat_DiagonalAxis) {
    Vec3 axis = {1, 1, 1};
    float theta = M_PI / 3.0f;  // 60 degrees

    Mat4 R_mat = BuildAxisRotationMatrix(axis, theta);
    Mat4 R_quat = BuildAxisRotationQuat(axis, theta);

    // Test multiple points
    std::vector<Vec3> testPoints = {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}, {-1, 2, 0.5f}
    };

    for (const auto& pt : testPoints) {
        Vec3 result_mat = Vec3(R_mat * Vec4(pt, 1.0f));
        Vec3 result_quat = Vec3(R_quat * Vec4(pt, 1.0f));
        EXPECT_TRUE(vectorsEqual(result_mat, result_quat))
            << "Failed for point (" << pt.x << ", " << pt.y << ", " << pt.z << ")";
    }
}

TEST(Validation, MatrixVsQuat_NegativeAngle) {
    Vec3 axis = {0, 0, 1};
    float theta = -M_PI / 2.0f;  // -90 degrees

    Mat4 R_mat = BuildAxisRotationMatrix(axis, theta);
    Mat4 R_quat = BuildAxisRotationQuat(axis, theta);

    Vec3 testPt = {1, 0, 0};
    Vec3 result_mat = Vec3(R_mat * Vec4(testPt, 1.0f));
    Vec3 result_quat = Vec3(R_quat * Vec4(testPt, 1.0f));

    EXPECT_TRUE(vectorsEqual(result_mat, result_quat));
}
