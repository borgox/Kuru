#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace Kuru::SDK {

using FLOAT = float;
using INT = std::int32_t;
using DWORD = std::uint32_t;
using WORD = std::uint16_t;
using BYTE = std::uint8_t;
using SBYTE = std::int8_t;
using SWORD = std::int16_t;
using QWORD = std::uint64_t;
using SQWORD = std::int64_t;
using PTRINT = std::intptr_t;
using SIZE_T = std::size_t;
using UBOOL = std::uint32_t;
using ANSICHAR = char;
using WIDECHAR = wchar_t;
using TCHAR = wchar_t;

constexpr FLOAT PI = 3.1415926535897932f;
constexpr FLOAT RAD_TO_DEG = 57.295779513082321600f;
constexpr FLOAT DEG_TO_RAD = 0.017453292519943296f;
constexpr FLOAT UNR_ROT_TO_DEG = 0.00549316540360483f;
constexpr FLOAT DEG_TO_UNR_ROT = 182.0444f;
constexpr INT INDEX_NONE = -1;

struct FVector {
    FLOAT X = 0;
    FLOAT Y = 0;
    FLOAT Z = 0;

    constexpr FVector() = default;
    constexpr FVector(FLOAT x, FLOAT y, FLOAT z) : X(x), Y(y), Z(z) {}

    [[nodiscard]] constexpr FVector operator+(const FVector& other) const noexcept {
        return {X + other.X, Y + other.Y, Z + other.Z};
    }

    [[nodiscard]] constexpr FVector operator-(const FVector& other) const noexcept {
        return {X - other.X, Y - other.Y, Z - other.Z};
    }

    [[nodiscard]] constexpr FVector operator*(FLOAT scalar) const noexcept {
        return {X * scalar, Y * scalar, Z * scalar};
    }

    [[nodiscard]] constexpr FVector operator/(FLOAT scalar) const noexcept {
        return {X / scalar, Y / scalar, Z / scalar};
    }

    constexpr FVector& operator+=(const FVector& other) noexcept {
        X += other.X;
        Y += other.Y;
        Z += other.Z;
        return *this;
    }

    constexpr FVector& operator-=(const FVector& other) noexcept {
        X -= other.X;
        Y -= other.Y;
        Z -= other.Z;
        return *this;
    }

    constexpr FVector& operator*=(FLOAT scalar) noexcept {
        X *= scalar;
        Y *= scalar;
        Z *= scalar;
        return *this;
    }

    constexpr FVector& operator/=(FLOAT scalar) noexcept {
        X /= scalar;
        Y /= scalar;
        Z /= scalar;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const FVector& other) const noexcept {
        return X == other.X && Y == other.Y && Z == other.Z;
    }

    [[nodiscard]] constexpr FLOAT Dot(const FVector& other) const noexcept {
        return X * other.X + Y * other.Y + Z * other.Z;
    }

    [[nodiscard]] constexpr FVector Cross(const FVector& other) const noexcept {
        return {
            Y * other.Z - Z * other.Y,
            Z * other.X - X * other.Z,
            X * other.Y - Y * other.X,
        };
    }

    [[nodiscard]] FLOAT Size() const noexcept { return std::sqrt(Dot(*this)); }
    [[nodiscard]] FLOAT Size2D() const noexcept { return std::sqrt(X * X + Y * Y); }
    [[nodiscard]] constexpr FLOAT SizeSquared() const noexcept { return Dot(*this); }
    [[nodiscard]] constexpr FLOAT SizeSquared2D() const noexcept { return X * X + Y * Y; }
    [[nodiscard]] constexpr bool IsZero() const noexcept { return X == 0.0f && Y == 0.0f && Z == 0.0f; }
    [[nodiscard]] bool IsNearlyZero(FLOAT tolerance = 1.e-4f) const noexcept {
        return std::fabs(X) <= tolerance && std::fabs(Y) <= tolerance && std::fabs(Z) <= tolerance;
    }
    [[nodiscard]] bool Equals(const FVector& other, FLOAT tolerance = 1.e-4f) const noexcept {
        return std::fabs(X - other.X) <= tolerance && std::fabs(Y - other.Y) <= tolerance &&
            std::fabs(Z - other.Z) <= tolerance;
    }
    bool Normalize(FLOAT tolerance = 1.e-8f) noexcept {
        const FLOAT squareSum = SizeSquared();
        if (squareSum <= tolerance) {
            return false;
        }
        *this /= std::sqrt(squareSum);
        return true;
    }
    [[nodiscard]] FVector SafeNormal(FLOAT tolerance = 1.e-8f) const noexcept {
        const FLOAT squareSum = SizeSquared();
        return squareSum > tolerance ? (*this) / std::sqrt(squareSum) : FVector{};
    }
    [[nodiscard]] FVector SafeNormal2D(FLOAT tolerance = 1.e-8f) const noexcept {
        const FLOAT squareSum = SizeSquared2D();
        return squareSum > tolerance ? FVector{X, Y, 0.0f} / std::sqrt(squareSum) : FVector{};
    }
    [[nodiscard]] static FLOAT Dist(const FVector& a, const FVector& b) noexcept { return (a - b).Size(); }
    [[nodiscard]] static FLOAT DistSquared(const FVector& a, const FVector& b) noexcept { return (a - b).SizeSquared(); }
    [[nodiscard]] static FVector Lerp(const FVector& a, const FVector& b, FLOAT alpha) noexcept {
        return a + (b - a) * alpha;
    }
};

struct FVector2D {
    FLOAT X = 0;
    FLOAT Y = 0;

    constexpr FVector2D() = default;
    constexpr FVector2D(FLOAT x, FLOAT y) : X(x), Y(y) {}

    [[nodiscard]] constexpr FVector2D operator+(const FVector2D& other) const noexcept { return {X + other.X, Y + other.Y}; }
    [[nodiscard]] constexpr FVector2D operator-(const FVector2D& other) const noexcept { return {X - other.X, Y - other.Y}; }
    [[nodiscard]] constexpr FVector2D operator*(FLOAT scalar) const noexcept { return {X * scalar, Y * scalar}; }
    [[nodiscard]] constexpr FVector2D operator/(FLOAT scalar) const noexcept { return {X / scalar, Y / scalar}; }
    [[nodiscard]] constexpr FLOAT Dot(const FVector2D& other) const noexcept { return X * other.X + Y * other.Y; }
    [[nodiscard]] FLOAT Size() const noexcept { return std::sqrt(Dot(*this)); }
    bool Normalize(FLOAT tolerance = 1.e-8f) noexcept {
        const FLOAT squareSum = Dot(*this);
        if (squareSum <= tolerance) {
            return false;
        }
        const FLOAT scale = 1.0f / std::sqrt(squareSum);
        X *= scale;
        Y *= scale;
        return true;
    }
    [[nodiscard]] bool Equals(const FVector2D& other, FLOAT tolerance = 1.e-4f) const noexcept {
        return std::fabs(X - other.X) <= tolerance && std::fabs(Y - other.Y) <= tolerance;
    }
};

struct FVector4 {
    FLOAT X = 0;
    FLOAT Y = 0;
    FLOAT Z = 0;
    FLOAT W = 0;

    constexpr FVector4() = default;
    constexpr FVector4(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : X(x), Y(y), Z(z), W(w) {}
};

struct FRotator {
    INT Pitch = 0;
    INT Yaw = 0;
    INT Roll = 0;

    constexpr FRotator() = default;
    constexpr FRotator(INT pitch, INT yaw, INT roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}

    [[nodiscard]] constexpr FRotator operator+(const FRotator& other) const noexcept {
        return {Pitch + other.Pitch, Yaw + other.Yaw, Roll + other.Roll};
    }
    [[nodiscard]] constexpr FRotator operator-(const FRotator& other) const noexcept {
        return {Pitch - other.Pitch, Yaw - other.Yaw, Roll - other.Roll};
    }
    [[nodiscard]] constexpr FRotator operator*(FLOAT scalar) const noexcept {
        return {static_cast<INT>(Pitch * scalar), static_cast<INT>(Yaw * scalar), static_cast<INT>(Roll * scalar)};
    }
    [[nodiscard]] constexpr bool operator==(const FRotator& other) const noexcept {
        return Pitch == other.Pitch && Yaw == other.Yaw && Roll == other.Roll;
    }
    void Normalize() noexcept {
        Pitch = ClampAxis(Pitch);
        Yaw = ClampAxis(Yaw);
        Roll = ClampAxis(Roll);
    }
    [[nodiscard]] FRotator GetNormalized() const noexcept {
        FRotator result = *this;
        result.Normalize();
        return result;
    }
    [[nodiscard]] static INT ClampAxis(INT angle) noexcept {
        angle &= 0xFFFF;
        return angle > 32767 ? angle - 65536 : angle;
    }
    [[nodiscard]] FVector Vector() const noexcept {
        const FLOAT pitchRad = static_cast<FLOAT>(Pitch) * UNR_ROT_TO_DEG * DEG_TO_RAD;
        const FLOAT yawRad = static_cast<FLOAT>(Yaw) * UNR_ROT_TO_DEG * DEG_TO_RAD;
        const FLOAT cp = std::cos(pitchRad);
        return {cp * std::cos(yawRad), cp * std::sin(yawRad), std::sin(pitchRad)};
    }
};

struct FQuat {
    FLOAT X = 0;
    FLOAT Y = 0;
    FLOAT Z = 0;
    FLOAT W = 1;

    constexpr FQuat() = default;
    constexpr FQuat(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : X(x), Y(y), Z(z), W(w) {}

    [[nodiscard]] constexpr FQuat operator*(FLOAT scalar) const noexcept { return {X * scalar, Y * scalar, Z * scalar, W * scalar}; }
    bool Normalize(FLOAT tolerance = 1.e-8f) noexcept {
        const FLOAT squareSum = X * X + Y * Y + Z * Z + W * W;
        if (squareSum <= tolerance) {
            return false;
        }
        const FLOAT scale = 1.0f / std::sqrt(squareSum);
        X *= scale;
        Y *= scale;
        Z *= scale;
        W *= scale;
        return true;
    }
    [[nodiscard]] constexpr FQuat Inverse() const noexcept { return {-X, -Y, -Z, W}; }
};

struct FPlane {
    FLOAT X = 0;
    FLOAT Y = 0;
    FLOAT Z = 0;
    FLOAT W = 0;

    constexpr FPlane() = default;
    constexpr FPlane(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : X(x), Y(y), Z(z), W(w) {}
};

struct FMatrix {
    FPlane XPlane;
    FPlane YPlane;
    FPlane ZPlane;
    FPlane WPlane;

    [[nodiscard]] FVector GetOrigin() const noexcept { return {WPlane.X, WPlane.Y, WPlane.Z}; }
    [[nodiscard]] static FMatrix Identity() noexcept {
        return {
            FPlane{1, 0, 0, 0},
            FPlane{0, 1, 0, 0},
            FPlane{0, 0, 1, 0},
            FPlane{0, 0, 0, 1},
        };
    }
    [[nodiscard]] FVector TransformFVector(const FVector& value) const noexcept {
        return {
            value.X * XPlane.X + value.Y * YPlane.X + value.Z * ZPlane.X + WPlane.X,
            value.X * XPlane.Y + value.Y * YPlane.Y + value.Z * ZPlane.Y + WPlane.Y,
            value.X * XPlane.Z + value.Y * YPlane.Z + value.Z * ZPlane.Z + WPlane.Z,
        };
    }
    [[nodiscard]] FVector TransformNormal(const FVector& value) const noexcept {
        return {
            value.X * XPlane.X + value.Y * YPlane.X + value.Z * ZPlane.X,
            value.X * XPlane.Y + value.Y * YPlane.Y + value.Z * ZPlane.Y,
            value.X * XPlane.Z + value.Y * YPlane.Z + value.Z * ZPlane.Z,
        };
    }
    [[nodiscard]] FMatrix Transpose() const noexcept {
        return {
            FPlane{XPlane.X, YPlane.X, ZPlane.X, WPlane.X},
            FPlane{XPlane.Y, YPlane.Y, ZPlane.Y, WPlane.Y},
            FPlane{XPlane.Z, YPlane.Z, ZPlane.Z, WPlane.Z},
            FPlane{XPlane.W, YPlane.W, ZPlane.W, WPlane.W},
        };
    }
};

struct FColor {
    BYTE B = 0;
    BYTE G = 0;
    BYTE R = 0;
    BYTE A = 255;

    [[nodiscard]] constexpr DWORD ToDWORD() const noexcept {
        return (static_cast<DWORD>(A) << 24) | (static_cast<DWORD>(R) << 16) |
            (static_cast<DWORD>(G) << 8) | static_cast<DWORD>(B);
    }
    [[nodiscard]] static constexpr FColor FromDWORD(DWORD value) noexcept {
        return {
            static_cast<BYTE>(value & 0xFF),
            static_cast<BYTE>((value >> 8) & 0xFF),
            static_cast<BYTE>((value >> 16) & 0xFF),
            static_cast<BYTE>((value >> 24) & 0xFF),
        };
    }
    [[nodiscard]] constexpr FColor WithAlpha(BYTE alpha) const noexcept { return {B, G, R, alpha}; }

    [[nodiscard]] constexpr bool operator==(const FColor& other) const noexcept {
        return B == other.B && G == other.G && R == other.R && A == other.A;
    }
};

struct FLinearColor {
    FLOAT R = 0;
    FLOAT G = 0;
    FLOAT B = 0;
    FLOAT A = 1;

    constexpr FLinearColor() = default;
    constexpr FLinearColor(FLOAT r, FLOAT g, FLOAT b, FLOAT a = 1.0f) : R(r), G(g), B(b), A(a) {}
    [[nodiscard]] constexpr FLinearColor operator+(const FLinearColor& other) const noexcept {
        return {R + other.R, G + other.G, B + other.B, A + other.A};
    }
    [[nodiscard]] constexpr FLinearColor operator*(FLOAT scalar) const noexcept {
        return {R * scalar, G * scalar, B * scalar, A * scalar};
    }
    [[nodiscard]] FLinearColor Clamp(FLOAT min = 0.0f, FLOAT max = 1.0f) const noexcept {
        return {
            std::fmin(std::fmax(R, min), max),
            std::fmin(std::fmax(G, min), max),
            std::fmin(std::fmax(B, min), max),
            std::fmin(std::fmax(A, min), max),
        };
    }
};

struct FIntPoint {
    INT X = 0;
    INT Y = 0;

    constexpr FIntPoint() = default;
    constexpr FIntPoint(INT x, INT y) : X(x), Y(y) {}
};

struct FIntVector {
    INT X = 0;
    INT Y = 0;
    INT Z = 0;

    constexpr FIntVector() = default;
    constexpr FIntVector(INT x, INT y, INT z) : X(x), Y(y), Z(z) {}
};

struct FBox {
    FVector Min;
    FVector Max;
    std::uint8_t IsValid = 0;

    void Init() noexcept {
        Min = FVector{};
        Max = FVector{};
        IsValid = 0;
    }
    [[nodiscard]] FVector GetCenter() const noexcept { return (Min + Max) * 0.5f; }
    [[nodiscard]] FVector GetExtent() const noexcept { return (Max - Min) * 0.5f; }
    void AddPoint(const FVector& point) noexcept {
        if (!IsValid) {
            Min = point;
            Max = point;
            IsValid = 1;
            return;
        }
        Min.X = std::fmin(Min.X, point.X);
        Min.Y = std::fmin(Min.Y, point.Y);
        Min.Z = std::fmin(Min.Z, point.Z);
        Max.X = std::fmax(Max.X, point.X);
        Max.Y = std::fmax(Max.Y, point.Y);
        Max.Z = std::fmax(Max.Z, point.Z);
    }
    [[nodiscard]] FBox ExpandBy(FLOAT amount) const noexcept {
        return {Min - FVector{amount, amount, amount}, Max + FVector{amount, amount, amount}, IsValid};
    }
    [[nodiscard]] bool IsInside(const FVector& point) const noexcept {
        return IsValid && point.X >= Min.X && point.X <= Max.X && point.Y >= Min.Y && point.Y <= Max.Y &&
            point.Z >= Min.Z && point.Z <= Max.Z;
    }
    [[nodiscard]] bool Intersect(const FBox& other) const noexcept {
        return IsValid && other.IsValid && Min.X <= other.Max.X && Max.X >= other.Min.X && Min.Y <= other.Max.Y &&
            Max.Y >= other.Min.Y && Min.Z <= other.Max.Z && Max.Z >= other.Min.Z;
    }
};

struct FSphere {
    FVector Center;
    FLOAT W = 0;
};

struct FBoxSphereBounds {
    FVector Origin;
    FVector BoxExtent;
    FLOAT SphereRadius = 0;
};

struct FTwoVectors {
    FVector V1;
    FVector V2;
};

struct FBoneAtom {
    FQuat Rotation;
    FVector Translation;
    FLOAT Scale = 1;
};

using FTransform = FBoneAtom;

struct FInterpCurvePointFloat {
    FLOAT InVal = 0;
    FLOAT OutVal = 0;
    FLOAT ArriveTangent = 0;
    FLOAT LeaveTangent = 0;
};

struct FInterpCurvePointVector2D {
    FLOAT InVal = 0;
    FVector2D OutVal;
    FVector2D ArriveTangent;
    FVector2D LeaveTangent;
};

struct FInterpCurvePointVector {
    FLOAT InVal = 0;
    FVector OutVal;
    FVector ArriveTangent;
    FVector LeaveTangent;
};

static_assert(sizeof(FVector) == 0x0C);
static_assert(sizeof(FVector2D) == 0x08);
static_assert(sizeof(FVector4) == 0x10);
static_assert(sizeof(FRotator) == 0x0C);
static_assert(sizeof(FQuat) == 0x10);
static_assert(sizeof(FPlane) == 0x10);
static_assert(sizeof(FMatrix) == 0x40);
static_assert(sizeof(FColor) == 0x04);
static_assert(sizeof(FLinearColor) == 0x10);
static_assert(sizeof(FIntPoint) == 0x08);
static_assert(sizeof(FIntVector) == 0x0C);
static_assert(sizeof(FBox) == 0x1C);
static_assert(sizeof(FSphere) == 0x10);
static_assert(sizeof(FBoxSphereBounds) == 0x1C);
static_assert(sizeof(FTwoVectors) == 0x18);
static_assert(sizeof(FBoneAtom) == 0x20);
static_assert(sizeof(FInterpCurvePointFloat) == 0x10);
static_assert(sizeof(FInterpCurvePointVector2D) == 0x1C);
static_assert(sizeof(FInterpCurvePointVector) == 0x28);

} // namespace Kuru::SDK
