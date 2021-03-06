
#ifndef INITIAL3D_FLOAT3_HPP
#define INITIAL3D_FLOAT3_HPP

#if defined(_MSC_VER)
#define INITIAL3D_ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
#define INITIAL3D_ALIGN(x) __attribute__((aligned(x)))
#else
#error unable to align
#endif

#include <iostream>
#include <cmath>

#include <xmmintrin.h>
#include <emmintrin.h>

#include "Initial3D.hpp"

namespace initial3d {

	// sse vector
	class float3 {
	private:
		__m128 m_data;

	public:
		float3(__m128 data_) : m_data(data_) { }

		float3() : m_data(_mm_setzero_ps()) { }

		float3(float v) : m_data(_mm_set1_ps(v)) { }

		float3(float x, float y, float z) : m_data(_mm_set_ps(0, z, y, x)) { }

		float3(const vec3f &v) : m_data(_mm_set_ps(0, v.z(), v.y(), v.x())) { }

		__m128 data() const {
			return m_data;
		}

		float x() const {
			float r;
			_mm_store_ss(&r, m_data);
			return r;
		}

		float y() const {
			float r;
			_mm_store_ss(&r, _mm_shuffle_ps(m_data, m_data, _MM_SHUFFLE(0, 0, 0, 1)));
			return r;
		}

		float z() const {
			float r;
			_mm_store_ss(&r, _mm_shuffle_ps(m_data, m_data, _MM_SHUFFLE(0, 0, 0, 2)));
			return r;
		}

		operator vec3f() const {
			INITIAL3D_ALIGN(16) float f[4];
			_mm_store_ps(f, m_data);
			return vec3f(f[0], f[1], f[2]);
		}

		float3 operator-() const {
			return float3(_mm_sub_ps(_mm_setzero_ps(), m_data));
		}

		float3 & operator+=(const float3 &rhs) {
			m_data = _mm_add_ps(m_data, rhs.m_data);
			return *this;
		}

		float3 & operator+=(float rhs) {
			m_data = _mm_add_ps(m_data, _mm_set1_ps(rhs));
			return *this;
		}

		float3 operator+(const float3 &rhs) const {
			return float3(*this) += rhs;
		}

		float3 operator+(float rhs) const {
			return float3(*this) += rhs;
		}

		inline friend float3 operator+(float lhs, const float3 &rhs) {
			return float3(rhs) + lhs;
		}

		float3 & operator-=(const float3 &rhs) {
			m_data = _mm_sub_ps(m_data, rhs.m_data);
			return *this;
		}

		float3 & operator-=(float rhs) {
			m_data = _mm_sub_ps(m_data, _mm_set1_ps(rhs));
			return *this;
		}

		float3 operator-(const float3 &rhs) const {
			return float3(*this) -= rhs;
		}

		float3 operator-(float rhs) const {
			return float3(*this) -= rhs;
		}

		inline friend float3 operator-(float lhs, const float3 &rhs) {
			return float3(_mm_set1_ps(lhs)) -= rhs;
		}

		float3 & operator*=(const float3 &rhs) {
			m_data = _mm_mul_ps(m_data, rhs.m_data);
			return *this;
		}

		float3 & operator*=(float rhs) {
			m_data = _mm_mul_ps(m_data, _mm_set1_ps(rhs));
			return *this;
		}

		float3 operator*(const float3 &rhs) const {
			return float3(*this) *= rhs;
		}

		float3 operator*(float rhs) const {
			return float3(*this) *= rhs;
		}

		inline friend float3 operator*(float lhs, const float3 &rhs) {
			return float3(rhs) * lhs;
		}

		float3 & operator/=(const float3 &rhs) {
			m_data = _mm_div_ps(m_data, rhs.m_data);
			return *this;
		}

		float3 & operator/=(float rhs) {
			m_data = _mm_div_ps(m_data, _mm_set1_ps(rhs));
			return *this;
		}

		float3 operator/(const float3 &rhs) const {
			return float3(*this) /= rhs;
		}

		float3 operator/(float rhs) const {
			return float3(*this) /= rhs;
		}

		inline friend float3 operator/(float lhs, const float3 &rhs) {
			return float3(_mm_set1_ps(lhs)) /= rhs;
		}

		float3 operator<(const float3 &rhs) const {
			return float3(_mm_cmplt_ps(m_data, rhs.m_data));
		}

		float3 operator<=(const float3 &rhs) const {
			return float3(_mm_cmple_ps(m_data, rhs.m_data));
		}

		float3 operator>(const float3 &rhs) const {
			return float3(_mm_cmpgt_ps(m_data, rhs.m_data));
		}

		float3 operator>=(const float3 &rhs) const {
			return float3(_mm_cmpge_ps(m_data, rhs.m_data));
		}

		float3 operator==(const float3 &rhs) const {
			return float3(_mm_cmpeq_ps(m_data, rhs.m_data));
		}

		float3 operator!=(const float3 &rhs) const {
			return float3(_mm_cmpneq_ps(m_data, rhs.m_data));
		}

		inline friend std::ostream & operator<<(std::ostream &out, const float3 &v) {
			out << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
			return out;
		}

		static float3 abs(const float3 &x) {
			return float3(_mm_andnot_ps(_mm_set1_ps(-0.f), x.m_data));
		}

		static float3 max(const float3 &x, const float3 &y) {
			return float3(_mm_max_ps(x.m_data, y.m_data));
		}

		static float3 min(const float3 &x, const float3 &y) {
			return float3(_mm_min_ps(x.m_data, y.m_data));
		}

		static float3 mixf(const float3 &a, const float3 &b, const float3 &t) {
			return a * (1.f - t) + b * t;
		}

		static float3 mixf(const float3 &a, const float3 &b, float t) {
			return a * (1.f - t) + b * t;
		}

		static float3 mixb(const float3 &a, const float3 &b, const float3 &t) {
			return float3(_mm_or_ps(_mm_andnot_ps(t.data(), a.data()), _mm_and_ps(t.data(), b.data())));
		}

		static float3 mixb(const float3 &a, const float3 &b, bool t) {
			float3 tt(_mm_cmpneq_ps(_mm_setzero_ps(), _mm_castsi128_ps(_mm_set1_epi8(t))));
			return mixb(a, b, tt);
		}

		static float dot(const float3 &lhs, const float3 &rhs) {
			__m128 r1 = _mm_mul_ps(lhs.m_data, rhs.m_data);
			__m128 r2 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 r3 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 2));
			__m128 r4 = _mm_add_ps(r3, _mm_add_ps(r2, r1));
			float r;
			_mm_store_ss(&r, r4);
			return r;
		}
		
		static float3 cross(const float3 &lhs, const float3 &rhs) {
			static const unsigned shuf_xzy = _MM_SHUFFLE(0, 0, 2, 1);
			static const unsigned shuf_yxz = _MM_SHUFFLE(0, 1, 0, 2);
			__m128 r1 = _mm_mul_ps(_mm_shuffle_ps(lhs.m_data, lhs.m_data, shuf_xzy), _mm_shuffle_ps(rhs.m_data, rhs.m_data, shuf_yxz));
			__m128 r2 = _mm_mul_ps(_mm_shuffle_ps(lhs.m_data, lhs.m_data, shuf_yxz), _mm_shuffle_ps(rhs.m_data, rhs.m_data, shuf_xzy));
			return float3(_mm_sub_ps(r1, r2));
		}
		
		static bool all(const float3 &x) {
			__m128 r1 = x.m_data;
			__m128 r2 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 r3 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 2));
			__m128 r4 = _mm_and_ps(r3, _mm_and_ps(r2, r1));
			float r;
			_mm_store_ss(&r, r4);
			return r != 0.f;
		}

		static bool any(const float3 &x) {
			__m128 r1 = x.m_data;
			__m128 r2 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 r3 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 2));
			__m128 r4 = _mm_or_ps(r3, _mm_or_ps(r2, r1));
			float r;
			_mm_store_ss(&r, r4);
			return r != 0.f;
		}

		float min() const {
			__m128 r1 = m_data;
			__m128 r2 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 r3 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 2));
			__m128 r4 = _mm_min_ss(r3, _mm_min_ss(r2, r1));
			float r;
			_mm_store_ss(&r, r4);
			return r;
		}

		float max() const {
			__m128 r1 = m_data;
			__m128 r2 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 r3 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 2));
			__m128 r4 = _mm_max_ss(r3, _mm_max_ss(r2, r1));
			float r;
			_mm_store_ss(&r, r4);
			return r;
		}

		bool isnan() const {
			return any(float3(_mm_cmpunord_ps(m_data, m_data)));
		}

		float mag() const {
			return sqrt(dot(*this, *this));
		}

		float3 unit() const {
			float a = dot(*this, *this);
			__m128 r1 = _mm_rsqrt_ss(_mm_load_ss(&a));
			__m128 r2 = _mm_mul_ps(m_data, _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0, 0, 0, 0)));
			return float3(r2);
		}

	};

	// axis-aligned bounding box
	class aabb {
	private:
		float3 m_center;
		float3 m_halfsize;

	public:
		aabb() { }

		aabb(const float3 &center_) : m_center(center_) { }

		aabb(const float3 &center_, const float3 &halfsize_) : m_center(center_), m_halfsize(float3::abs(halfsize_)) { }

		float3 center() const {
			return m_center;
		}

		float3 halfsize() const {
			return m_halfsize;
		}

		float3 min() const {
			return m_center - m_halfsize;
		}

		float3 max() const {
			return m_center + m_halfsize;
		}

		bool contains(const float3 &p) const {
			return float3::all(float3::abs(p - m_center) <= m_halfsize);
		}

		bool contains(const aabb &a) const {
			return float3::all(float3::abs(a.m_center - m_center) <= (m_halfsize - a.m_halfsize));
		}

		bool contains_partial(const aabb &a) const {
			// intersects + contains in 1 dimension
			return float3::any(float3::abs(a.m_center - m_center) <= (m_halfsize - a.m_halfsize)) && intersects(a);
		}

		bool intersects(const aabb &a) const {
			return float3::all(float3::abs(a.m_center - m_center) <= (m_halfsize + a.m_halfsize));
		}

		inline friend std::ostream & operator<<(std::ostream &out, const aabb &a) {
			out << "aabb[" << a.min() << " <= x <= " << a.max() << "]";
			return out;
		}

		static aabb fromPoints(const float3 &p0, const float3 &p1) {
			float3 minv = float3::min(p0, p1);
			float3 maxv = float3::max(p0, p1);
			float3 hs = 0.5 * (maxv - minv);
			return aabb(minv + hs, hs);
		}

	};

}

#endif
