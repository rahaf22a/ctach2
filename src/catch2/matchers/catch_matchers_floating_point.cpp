
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/internal/catch_enforce.hpp>
#include <catch2/internal/catch_polyfills.hpp>
#include <catch2/internal/catch_to_string.hpp>
#include <catch2/catch_tostring.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <limits>


namespace Catch {
namespace {

    int32_t convert(float f) {
        static_assert(sizeof(float) == sizeof(int32_t), "Important ULP matcher assumption violated");
        int32_t i;
        std::memcpy(&i, &f, sizeof(f));
        return i;
    }

    int64_t convert(double d) {
        static_assert(sizeof(double) == sizeof(int64_t), "Important ULP matcher assumption violated");
        int64_t i;
        std::memcpy(&i, &d, sizeof(d));
        return i;
    }

    // Calculates the ULP distance between two floating point numbers
    // That is, the number of valid IEEE-754 floating point representations
    // between the two values. In the general case we can say:
    //  * if nextafter(a, INFINITY) == b, then ulpDistance(a, b) == 1
    //  * if a == nextafter(b, INFINITY), then ulpDistance(a, b) == -1
    // however, as an exception, the distance between positive and negative
    // zero is considered to always have a value of zero
    // There is an argument to be made that this distance should be one, since
    //     nextafter(-0f, INFINITY) == +0f
    //     nextafter(+0f, -INFINITY) == -0f
    // However, the above exception was chosen to ensure that a == b implies
    //     ulpDistance(a, b) == 0
    // and that
    //     ulpDistance(-x, x) == ulpDistance(0, x)*2
    // Denormalized normals are counted normally in distance calculations.
    // See also: boost/math/special_functions/next.hpp
    template <typename FP>
    int64_t ulpDistance(FP a, FP b) {
        // Smallest value greater than zero
        constexpr FP EPSILON_0 = std::numeric_limits<FP>::denorm_min();
        // Largest possible distance we can return
        constexpr int64_t INFINITE_DISTANCE = std::numeric_limits<int64_t>::max();
        if (Catch::isnan(a) || Catch::isnan(b)) // Early out for NaNs
          return INFINITE_DISTANCE;
        if (!std::isfinite(a) || !std::isfinite(b)) // Early out for infinity
          return INFINITE_DISTANCE;
        if(a > b) // Ensure a < b
            return -ulpDistance(b, a);
        if(a == b) // This also ensure ulpDistance(-0f, +0f) == 0
            return 0;
        if(a == 0) // Ensure a != 0
            return 1 + std::abs(ulpDistance((b < 0) ? -EPSILON_0 : EPSILON_0, b));
        if(b == 0) // Ensure b != 0
            return 1 + std::abs(ulpDistance((a < 0) ? -EPSILON_0 : EPSILON_0, a));
        if((a < 0) != (b < 0)) // Ensure a and b have the same sign
            return 2 + std::abs(ulpDistance((b < 0) ? -EPSILON_0 : EPSILON_0, b))
                     + std::abs(ulpDistance((a < 0) ? -EPSILON_0 : EPSILON_0, a));
        if(a < 0) // Ensure a and b are positive
            return ulpDistance(-b, -a);
        assert(a >= 0);
        assert(a < b);
        int64_t ac = convert(a);
        int64_t bc = convert(b);
        return bc - ac; // ULP distance, assuming IEEE-754 floating point numbers
    }

    template <typename FP>
    bool almostEqualUlps(FP lhs, FP rhs, uint64_t maxUlpDiff) {
        // Comparison with NaN should always be false.
        // This way we can rule it out before getting into the ugly details
        if (Catch::isnan(lhs) || Catch::isnan(rhs))
            return false;
        if (!std::isfinite(lhs) || !std::isfinite(rhs))
            return lhs == rhs;

        auto ulpDiff = std::abs(ulpDistance(lhs, rhs));
        return static_cast<uint64_t>(ulpDiff) <= maxUlpDiff;
    }

#if defined(CATCH_CONFIG_GLOBAL_NEXTAFTER)

    float nextafter(float x, float y) {
        return ::nextafterf(x, y);
    }

    double nextafter(double x, double y) {
        return ::nextafter(x, y);
    }

#endif // ^^^ CATCH_CONFIG_GLOBAL_NEXTAFTER ^^^

template <typename FP>
FP step(FP start, FP direction, uint64_t steps) {
    for (uint64_t i = 0; i < steps; ++i) {
#if defined(CATCH_CONFIG_GLOBAL_NEXTAFTER)
        start = Catch::nextafter(start, direction);
#else
        start = std::nextafter(start, direction);
#endif
    }
    return start;
}

// Performs equivalent check of std::fabs(lhs - rhs) <= margin
// But without the subtraction to allow for INFINITY in comparison
bool marginComparison(double lhs, double rhs, double margin) {
    return (lhs + margin >= rhs) && (rhs + margin >= lhs);
}

template <typename FloatingPoint>
void write(std::ostream& out, FloatingPoint num) {
    out << std::scientific
        << std::setprecision(std::numeric_limits<FloatingPoint>::max_digits10 - 1)
        << num;
}

} // end anonymous namespace

namespace Matchers {
namespace Detail {

    enum class FloatingPointKind : uint8_t {
        Float,
        Double
    };

} // end namespace Detail


    WithinAbsMatcher::WithinAbsMatcher(double target, double margin)
        :m_target{ target }, m_margin{ margin } {
        CATCH_ENFORCE(margin >= 0, "Invalid margin: " << margin << '.'
            << " Margin has to be non-negative.");
    }

    // Performs equivalent check of std::fabs(lhs - rhs) <= margin
    // But without the subtraction to allow for INFINITY in comparison
    bool WithinAbsMatcher::match(double const& matchee) const {
        return (matchee + m_margin >= m_target) && (m_target + m_margin >= matchee);
    }

    std::string WithinAbsMatcher::describe() const {
        return "is within " + ::Catch::Detail::stringify(m_margin) + " of " + ::Catch::Detail::stringify(m_target);
    }


    WithinUlpsMatcher::WithinUlpsMatcher(double target, uint64_t ulps, Detail::FloatingPointKind baseType)
        :m_target{ target }, m_ulps{ ulps }, m_type{ baseType } {
        CATCH_ENFORCE(m_type == Detail::FloatingPointKind::Double
                   || m_ulps <= (std::numeric_limits<uint32_t>::max)(),
            "Provided ULP is impossibly large for a float comparison.");
    }

#if defined(__clang__)
#pragma clang diagnostic push
// Clang <3.5 reports on the default branch in the switch below
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif

    bool WithinUlpsMatcher::match(double const& matchee) const {
        switch (m_type) {
        case Detail::FloatingPointKind::Float:
            return almostEqualUlps<float>(static_cast<float>(matchee), static_cast<float>(m_target), m_ulps);
        case Detail::FloatingPointKind::Double:
            return almostEqualUlps<double>(matchee, m_target, m_ulps);
        default:
            CATCH_INTERNAL_ERROR( "Unknown Detail::FloatingPointKind value" );
        }
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    std::string WithinUlpsMatcher::describe() const {
        std::stringstream ret;

        ret << "is within " << m_ulps << " ULPs of ";

        if (m_type == Detail::FloatingPointKind::Float) {
            write(ret, static_cast<float>(m_target));
            ret << 'f';
        } else {
            write(ret, m_target);
        }

        ret << " ([";
        if (m_type == Detail::FloatingPointKind::Double) {
            write(ret, step(m_target, static_cast<double>(-INFINITY), m_ulps));
            ret << ", ";
            write(ret, step(m_target, static_cast<double>( INFINITY), m_ulps));
        } else {
            // We have to cast INFINITY to float because of MinGW, see #1782
            write(ret, step(static_cast<float>(m_target), static_cast<float>(-INFINITY), m_ulps));
            ret << ", ";
            write(ret, step(static_cast<float>(m_target), static_cast<float>( INFINITY), m_ulps));
        }
        ret << "])";

        return ret.str();
    }

    WithinRelMatcher::WithinRelMatcher(double target, double epsilon):
        m_target(target),
        m_epsilon(epsilon){
        CATCH_ENFORCE(m_epsilon >= 0., "Relative comparison with epsilon <  0 does not make sense.");
        CATCH_ENFORCE(m_epsilon  < 1., "Relative comparison with epsilon >= 1 does not make sense.");
    }

    bool WithinRelMatcher::match(double const& matchee) const {
        const auto relMargin = m_epsilon * (std::max)(std::fabs(matchee), std::fabs(m_target));
        return marginComparison(matchee, m_target,
                                std::isinf(relMargin)? 0 : relMargin);
    }

    std::string WithinRelMatcher::describe() const {
        Catch::ReusableStringStream sstr;
        sstr << "and " << m_target << " are within " << m_epsilon * 100. << "% of each other";
        return sstr.str();
    }


WithinUlpsMatcher WithinULP(double target, uint64_t maxUlpDiff) {
    return WithinUlpsMatcher(target, maxUlpDiff, Detail::FloatingPointKind::Double);
}

WithinUlpsMatcher WithinULP(float target, uint64_t maxUlpDiff) {
    return WithinUlpsMatcher(target, maxUlpDiff, Detail::FloatingPointKind::Float);
}

WithinAbsMatcher WithinAbs(double target, double margin) {
    return WithinAbsMatcher(target, margin);
}

WithinRelMatcher WithinRel(double target, double eps) {
    return WithinRelMatcher(target, eps);
}

WithinRelMatcher WithinRel(double target) {
    return WithinRelMatcher(target, std::numeric_limits<double>::epsilon() * 100);
}

WithinRelMatcher WithinRel(float target, float eps) {
    return WithinRelMatcher(target, eps);
}

WithinRelMatcher WithinRel(float target) {
    return WithinRelMatcher(target, std::numeric_limits<float>::epsilon() * 100);
}


} // namespace Matchers
} // namespace Catch
