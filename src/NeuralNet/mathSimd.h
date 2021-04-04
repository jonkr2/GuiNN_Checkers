//
// Math SIMD routines for neural nets
// by Jonathan Kreuzer
//
#pragma once

#include <assert.h>
#include <algorithm>
#include <nmmintrin.h>
#ifndef __GNUC__
#include <intrin.h>
#else
#include <immintrin.h>
#endif

#include "../defines.h"

class SIMD
{
public:
	// Horizontal sum (add 8 adjacently stored 32-bit integers and return that value)
	static inline int32_t horizontalSum_8x32(__m256i v)
	{
		__m128i hSum = _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
		hSum = _mm_add_epi32(hSum, _mm_shuffle_epi32(hSum, _MM_SHUFFLE(1, 0, 3, 2)));
		hSum = _mm_add_epi32(hSum, _mm_shuffle_epi32(hSum, _MM_SHUFFLE(2, 3, 0, 1)));
		return _mm_cvtsi128_si32(hSum);
	}

	static inline int32_t horizontalSum_4x32(__m128i v)
	{
		v = _mm_add_epi32(v, _mm_shuffle_epi32(v, _MM_SHUFFLE(1, 0, 3, 2))); // 0+1 in 0, 2+3 in 2
		v = _mm_add_epi32(v, _mm_shuffle_epi32(v, _MM_SHUFFLE(2, 3, 0, 1))); // 0+1+2+3 in 0
		return _mm_cvtsi128_si32(v);
	}

	// (32-bit) returns the dot-product of v1 and v2 
	static inline int dotProductInt32(const int32_t* v1, const int32_t* v2, size_t count)
	{
		assert((count & 7) == 0);
		const int32_t* const v1End = v1 + count;

#if defined(USE_AVX2)
		__m256i dotSum = _mm256_setzero_si256();

		for (; v1 < v1End; v1 += 8, v2 += 8)
		{
			const __m256i temp_1 = _mm256_load_si256((__m256i*)v1);		// Load the 8 values
			const __m256i temp_2 = _mm256_load_si256((__m256i*)v2); 	// Load the 8 values
			__m256i temp_products = _mm256_mullo_epi32(temp_1, temp_2);
			// multiply the lo half of the weights... could we do mullo and mullhi with 16-bit weights?
			dotSum = _mm256_add_epi32(dotSum, temp_products);		// sum all 8
		}

		// take horizontal sum of the 8 dotSums
		return horizontalSum_8x32(dotSum);
#elif defined(USE_SSE2)  
		__m128i dotSum = _mm_setzero_si128();
		for (; v1 < v1End; v1 += 4, v2 += 4)
		{
			const __m128i temp_1 = _mm_load_si128((__m128i*)v1);		// Load the 4 values
			const __m128i temp_2 = _mm_load_si128((__m128i*)v2); 		// Load the 4 values
			__m128i temp_products = _mm_mullo_epi32(temp_1, temp_2); // multiply the values
			dotSum = _mm_add_epi32(dotSum, temp_products);		// sum all 4
		}

		// take horizontal sum of the 4 dotSums
		return horizontalSum_4x32(dotSum);
#else
		int32_t dotSum = 0;
		for (; v1 < v1End; v1++, v2++)
		{
			dotSum += (*v1) * (*v2);
		}
		return dotSum;
#endif
	}

	// (16-bit) returns the dot-product of v1 and v2 
	static inline int dotProductInt16(const int16_t* v1, const int16_t* v2, size_t count)
	{
		assert((count & 15) == 0);
		assert(((int64_t)v1 & 31) == 0);
		assert(((int64_t)v2 & 31) == 0);
		const int16_t* const v1End = v1 + count;

#if defined(USE_AVX2)
		__m256i dotSum = _mm256_setzero_si256();
		for (; v1 < v1End; v1 += 16, v2 += 16)
		{
			const __m256i temp_1 = _mm256_load_si256((__m256i*)v1);	// Load the 16 values
			const __m256i temp_2 = _mm256_load_si256((__m256i*)v2); // Load the 16 values
			dotSum = _mm256_add_epi32(dotSum, _mm256_madd_epi16(temp_1, temp_2)); // Do the mulitply add, and add result to dotSum
		}

		// madd16 stores in 8 32-bit values (does adjacent sums) so we have to horizontal sum those : https://software.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compiler-reference/intrinsics/intrinsics-for-intel-advanced-vector-extensions-2/intrinsics-for-arithmetic-operations-2/mm256-madd-epi16.html
		return horizontalSum_8x32(dotSum);
#elif defined(USE_SSE2)  
		__m128i dotSum = _mm_setzero_si128();
		for (; v1 < v1End; v1 += 8, v2 += 8)
		{
			const __m128i temp_1 = _mm_load_si128((__m128i*)v1); // Load the 8 values
			const __m128i temp_2 = _mm_load_si128((__m128i*)v2); // Load the 8 values
			dotSum = _mm_add_epi32(dotSum, _mm_madd_epi16(temp_1, temp_2)); // Do the mulitply add, and add result to dotSum
		}

		// madd16 stores in 4 32-bit values (does adjacent sums) so we have to horizontal sum those : https://software.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compiler-reference/intrinsics/intrinsics-for-intel-advanced-vector-extensions-2/intrinsics-for-arithmetic-operations-2/mm256-madd-epi16.html
		return horizontalSum_4x32(dotSum);
#else
		int32_t dotSum = 0;

		for (; v1 < v1End; v1++, v2++)
		{
			dotSum += (*v1) * (*v2);
		}
		return dotSum;
#endif
	}

	static inline float dotProductFloat(const float* v1, const float* v2, size_t count)
	{
		__m256 acc = _mm256_setzero_ps();
		const float* const v1End = v1 + (count - (count & 7));
		const float* const v1End2 = v1 + count;
		for (; v1 < v1End; v1 += 8, v2 += 8)
		{
			// Load 8 floats( 256 = 8 * 32-bit ) into a and b.
			const __m256 a = _mm256_load_ps(v1); // if aligned don't need the u
			const __m256 b = _mm256_load_ps(v2);
			// vdpps AVX instruction computes 2 independent dot products of 4-wide vectors, so add them up
			const __m256 dp = _mm256_dp_ps(a, b, 0xFF);
			acc = _mm256_add_ps(acc, dp);
		}

		// Add the 2 results into a single float.
		const __m128 low = _mm256_castps256_ps128(acc);
		const __m128 high = _mm256_extractf128_ps(acc, 1); // move data from high half of a register into low half with extractf128 instruction
		const __m128 result = _mm_add_ss(low, high);
		float ret = _mm_cvtss_f32(result);

		// Clean up any leftovers if not multiple of 8... For speed could ensure multiple of 8 and remove this
		if (v1 != v1End2)
		{
			for (; v1 < v1End2; v1++, v2++)
			{
				ret += (*v1) * (*v2);
			}
		}

		return ret;
	}

	// v1 = v1 + v2
	static inline void addVec16(int16_t* v1, const int16_t* v2, size_t count)
	{
		assert((count & 15) == 0);
		assert(((int64_t)v1 & 31) == 0);
		const int16_t* v1End = v1 + count;

#if defined(USE_AVX2)
		for (; v1 < v1End; v1 += 16, v2 += 16)
		{
			const __m256i temp1 = _mm256_load_si256((__m256i*)v1);	// Load the 16 values from v1
			const __m256i temp2 = _mm256_load_si256((__m256i*)v2); 	// Load the 16 values from v2
			_mm256_store_si256((__m256i*)v1, _mm256_add_epi16(temp1, temp2)); // Sum all 16 and store in v1
		}
#elif defined(USE_SSE2)
		for (; v1 < v1End; v1 += 8, v2 += 8)
		{
			const __m128i temp1 = _mm_load_si128((__m128i*)v1);
			const __m128i temp2 = _mm_load_si128((__m128i*)v2);
			_mm_store_si128((__m128i*)v1, _mm_add_epi16(temp1, temp2)); // Sum all 8 and store in v1
		}
#else
		for (; v1 < v1End; v1++, v2++)
		{
			*v1 += *v2;
		}
#endif
	}

	// v1 = v1 + v2
	static inline void addVec32(int32_t* v1, const int32_t* v2, size_t count)
	{
		assert((count & 7) == 0);
		const int32_t* v1End = v1 + count;

#if defined(USE_AVX2)
		for (; v1 < v1End; v1 += 8, v2 += 8)
		{
			const __m256i temp1 = _mm256_load_si256((__m256i*)v1);	// Load the 8 values from v1
			const __m256i temp2 = _mm256_load_si256((__m256i*)v2); 	// Load the 8 values from v2
			const __m256i tempSum = _mm256_add_epi32(temp1, temp2);
			_mm256_store_si256((__m256i*)v1, tempSum); // Sum all 8 and store in v1
		}
#elif defined(USE_SSE2)
		for (; v1 < v1End; v1 += 4, v2 += 4)
		{
			const __m128i temp1 = _mm_load_si128((__m128i*)v1);
			const __m128i temp2 = _mm_load_si128((__m128i*)v2);
			_mm_store_si128((__m128i*)v1, _mm_add_epi32(temp1, temp2)); // Sum all 8 and store in v1
		}
#else
		for (; v1 < v1End; v1++, v2++)
		{
			*v1 += *v2;
		}
#endif
	}

	// v1 = v1 - v2
	static inline void subVec16(int16_t* v1, const int16_t* v2, size_t count)
	{
		assert((count & 15) == 0);
		const int16_t* v1End = v1 + count;

#if defined(USE_AVX2)
		for (; v1 < v1End; v1 += 16, v2 += 16)
		{
			const __m256i temp1 = _mm256_load_si256((__m256i*)v1);	// Load the 16 values from v1
			const __m256i temp2 = _mm256_load_si256((__m256i*)v2); 	// Load the 16 values from v2
			_mm256_store_si256((__m256i*)v1, _mm256_sub_epi16(temp1, temp2)); // Sum all 16 and store in v1
		}
#elif defined(USE_SSE2)
		for (; v1 < v1End; v1 += 8, v2 += 8)
		{
			const __m128i temp1 = _mm_load_si128((__m128i*)v1);
			const __m128i temp2 = _mm_load_si128((__m128i*)v2);
			_mm_store_si128((__m128i*)v1, _mm_sub_epi16(temp1, temp2)); // Sum all 8 and store in v1
		}
#else
		for (; v1 < v1End; v1++, v2++)
		{
			*v1 -= *v2;
		}
#endif
	}

	// This is used for full transform... which isn't used right now
	static inline void addMulVec32(int32_t* o, const int32_t* weights, const int32_t val, size_t count)
	{
		assert((count & 7) == 0);
		const int32_t* oEnd = o + count;

#if defined(USE_AVX2)
		__m256i iVal = _mm256_set_epi32(val, val, val, val, val, val, val, val);

		for (; o < oEnd; o += 8, weights += 8)
		{
			__m256i wVal = _mm256_load_si256((__m256i*)weights);	// Load the values of 8 weights
			wVal = _mm256_mullo_epi32(wVal, iVal); // multiply weights with the input value... I think this only multiplies lo 16-bit?
			_mm256_store_si256((__m256i*)o, _mm256_add_epi32(_mm256_load_si256((__m256i*)o), wVal)); // Sum all 8 and store in o
		}
#else
		for (; o < oEnd; o++, weights++)
		{
			*o += val * (*weights);
		}
#endif 
	}


	static inline void clamp0Vec16(int16_t* v1, size_t count)
	{
		const int16_t* v1End = v1 + count;

#if defined(USE_AVX2)
		__m256i zero = _mm256_setzero_si256();
		for (; v1 < v1End; v1 += 16)
		{
			const __m256i temp1 = _mm256_load_si256((__m256i*)v1);	// Load the 16 values from v1
			_mm256_store_si256((__m256i*)v1, _mm256_max_epi16(temp1, zero)); // clamp to >= 0
		}
#elif defined(USE_SSE2)
		__m128i zero = _mm_setzero_si128();
		for (; v1 < v1End; v1 += 8)
		{
			const __m128i temp1 = _mm_load_si128((__m128i*)v1);	// Load the 8 values from v1
			_mm_store_si128((__m128i*)v1, _mm_max_epi16(temp1, zero)); // clamp to >= 0
		}
#else
		for (uint32_t o = 0; o < count; o++)
		{
			v1[o] = std::max(v1[o], (int16_t)0);
		}
#endif 
	}

	// This is not used anymore, need to check it over if used
	static inline void clamp0Vec32(int32_t* v1, size_t count)
	{
		const int32_t* v1End = v1 + count;

#if defined(USE_AVX2)
		__m256i zero = _mm256_setzero_si256();
		for (; v1 < v1End; v1 += 8)
		{
			const __m256i temp1 = _mm256_load_si256((__m256i*)v1);	// Load the 8 values from v1
			_mm256_store_si256((__m256i*)v1, _mm256_max_epi32(temp1, zero)); // clamp to >= 0
		}
#elif defined(USE_SSE2)
		__m128i zero = _mm_setzero_si128();
		for (; v1 < v1End; v1 += 4)
		{
			const __m128i temp1 = _mm_load_si128((__m128i*)v1);	// Load the 4 values from v1
			// _mm_max_epi32 is SSE4
			_mm_store_si128((__m128i*)v1, _mm_max_epi32(temp1, zero)); // clamp to >= 0
		}
#else
		for (; v1 < v1End; v1++)
		{
			*v1 = std::max(*v1, (int32_t)0);
		}
#endif 
	}

	// Converts to 32-bit v1 to 16-bit v2, including doing a right-shift 
	static inline void convertVec32to16clamp0(int32_t* v1, int16_t* v2, const int shift, size_t count)
	{
		assert((count & 15) == 0);
		const int32_t* v1End = v1 + count;

#if defined(USE_AVX2)
		__m256i zero = _mm256_setzero_si256();
		for (; v1 < v1End; v1 += 16, v2 += 16)
		{
			__m256i a = _mm256_load_si256((__m256i*)v1); // load 16 values (32-bit)
			__m256i b = _mm256_load_si256((__m256i*)(v1 + 8));
			a = _mm256_srai_epi32(a, shift); // shift right 
			b = _mm256_srai_epi32(b, shift);
			__m256i result = _mm256_packs_epi32(a, b); // pack into 16 values (16-bit)
			result = _mm256_permute4x64_epi64(result, 0xD8);
			_mm256_store_si256((__m256i*)v2, _mm256_max_epi16(result, zero)); // clamp to >= 0 and write into v2
		}
#elif defined(USE_SSE2)
		__m128i zero = _mm_setzero_si128();
		for (; v1 < v1End; v1 += 8, v2 += 8)
		{
			__m128i a = _mm_load_si128((__m128i*)v1); // load 8 values (32-bit)
			__m128i b = _mm_load_si128((__m128i*)(v1 + 4));
			a = _mm_srai_epi32(a, shift); // shift right 
			b = _mm_srai_epi32(b, shift);
			// pack into 8 values (16-bit) in v2, and also clamp to >= 0
			_mm_store_si128((__m128i*)v2, _mm_max_epi16(_mm_packs_epi32(a, b), zero));
		}
#else
		for (; v1 < v1End; v1++, v2++)
			*v2 = (int16_t)(*v1 >> shift);
#endif
	}
};