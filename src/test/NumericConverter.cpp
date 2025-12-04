/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "TestUtil.h"
#include "fb-cpp/types.h"
#include "fb-cpp/NumericConverter.h"
#include "fb-cpp/Exception.h"
#include <limits>
#include <cstdint>

static constexpr float floatTolerance = 0.00001f;
static constexpr double doubleTolerance = 0.00000000000001;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
static const BoostDecFloat16 decFloat16Tolerance{"0.00000000000001"};
static const BoostDecFloat34 decFloat34Tolerance{"0.00000000000000000000000000000001"};
#endif


BOOST_AUTO_TEST_SUITE(NumericConverterSuite)

BOOST_AUTO_TEST_CASE(convertScaledInt16)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(ScaledInt16{12'3, -1}, -4), FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(ScaledInt16{-12'3, -1}, -4), FbCppException);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{3'276'7, -1}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{3'2767, -4}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{3'276'7, -1}, -1), 3'276'7);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{-3'276'8, -1}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{-3'2768, -4}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(ScaledInt16{-3'276'8, -1}, -1), -3'276'8);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{12'3, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{-12'3, -1}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{3'276'7, -1}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{3'2767, -4}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{3'276'7, -1}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{-3'276'8, -1}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{-3'2768, -4}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt16{-3'276'8, -1}, -1), -32768);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{12'3, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{-12'3, -1}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{3'276'7, -1}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{3'2767, -4}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{3'276'7, -1}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{-3'276'8, -1}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{-3'2768, -4}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt16{-3'276'8, -1}, -1), -32768);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{12'3, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{-12'3, -1}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{3'276'7, -1}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{3'2767, -4}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{3'276'7, -1}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{-3'276'8, -1}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{-3'2768, -4}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt16{-3'276'8, -1}, -1), -32768);
#endif

	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt16{12'3, -1}), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt16{-12'3, -1}), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt16{3'276'7, -1}), 3'276.7f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt16{3'2767, -4}), 3.2767f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt16{-3'276'8, -1}), -3'276.8f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt16{-3'2768, -4}), -3.2768f, floatTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt16{12'3, -1}), 12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt16{-12'3, -1}), -12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt16{3'276'7, -1}), 3'276.7, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt16{3'2767, -4}), 3.2767, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt16{-3'276'8, -1}), -3'276.8, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt16{-3'2768, -4}), -3.2768, doubleTolerance);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<BoostDecFloat16>(ScaledInt16{12'3, -1}), BoostDecFloat16{"12.3"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt16{-12'3, -1}), BoostDecFloat16{"-12.3"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt16{3'276'7, -1}), BoostDecFloat16{"3276.7"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt16{3'2767, -4}), BoostDecFloat16{"3.2767"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt16{-3'276'8, -1}), BoostDecFloat16{"-3276.8"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt16{-3'2768, -4}), BoostDecFloat16{"-3.2768"},
		decFloat16Tolerance);

	BOOST_CHECK_CLOSE(
		converter.numberToNumber<BoostDecFloat34>(ScaledInt16{12'3, -1}), BoostDecFloat34{"12.3"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt16{-12'3, -1}), BoostDecFloat34{"-12.3"},
		decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt16{3'276'7, -1}), BoostDecFloat34{"3276.7"},
		decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt16{3'2767, -4}), BoostDecFloat34{"3.2767"},
		decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt16{-3'276'8, -1}), BoostDecFloat34{"-3276.8"},
		decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt16{-3'2768, -4}), BoostDecFloat34{"-3.2768"},
		decFloat34Tolerance);
#endif

	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt16{32767, 0}), "32767");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt16{-32768, 0}), "-32768");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt16{3'276'7, -1}), "3276.7");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt16{-3'276'8, -1}), "-3276.8");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt16{3'2767, -4}), "3.2767");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt16{-3'2768, -4}), "-3.2768");
}

BOOST_AUTO_TEST_CASE(convertScaledInt32)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(ScaledInt32{12'3, -1}, -4), FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(ScaledInt32{214'748'364'7, -1}, -1), FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(ScaledInt32{-214'748'364'8, -1}, -1), FbCppException);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{12'3, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{-12'3, -1}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{214'748'364'7, -1}, 0), 214'748'365);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{214'748'3647, -4}, 0), 214'748);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{214'748'364'7, -1}, -1), 214'748'364'7);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{-214'748'364'8, -1}, 0), -214'748'365);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{-214'748'3648, -4}, 0), -214'748);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(ScaledInt32{-214'748'364'8, -1}, -1), -214'748'364'8);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{12'3, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{-12'3, -1}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{214'748'364'7, -1}, 0), 214'748'365);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{214'748'3647, -4}, 0), 214'748);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{214'748'364'7, -1}, -1), 214'748'364'7);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{-214'748'364'8, -1}, 0), -214'748'365);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{-214'748'3648, -4}, 0), -214'748);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt32{-214'748'364'8, -1}, -1), -214'748'364'8);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{12'3, -1}, -2) == BoostInt128{12'30});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{-12'3, -1}, -2) == BoostInt128{-12'30});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{12'3, -1}, -4) == BoostInt128{12'3000});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{-12'3, -1}, -4) == BoostInt128{-12'3000});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{214'748'364'7, -1}, 0) == BoostInt128{214'748'365});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{214'748'3647, -4}, 0) == BoostInt128{214'748});
	BOOST_CHECK(
		converter.numberToNumber<BoostInt128>(ScaledInt32{214'748'364'7, -1}, -1) == BoostInt128{214'748'364'7});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{-214'748'364'8, -1}, 0) == BoostInt128{-214'748'365});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt32{-214'748'3648, -4}, 0) == BoostInt128{-214'748});
	BOOST_CHECK(
		converter.numberToNumber<BoostInt128>(ScaledInt32{-214'748'364'8, -1}, -1) == BoostInt128{-2'147'483'648});
#endif

	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt32{12'3, -1}), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt32{-12'3, -1}), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt32{214'748'364'7, -1}), 214'748'364.7f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt32{214'748'3647, -4}), 214'748.3647f, floatTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<float>(ScaledInt32{-214'748'364'8, -1}), -214'748'364.8f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt32{-214'748'3648, -4}), -214'748.3648f, floatTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt32{12'3, -1}), 12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt32{-12'3, -1}), -12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt32{214'748'364'7, -1}), 214'748'364.7, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt32{214'748'3647, -4}), 214'748.3647, doubleTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<double>(ScaledInt32{-214'748'364'8, -1}), -214'748'364.8, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt32{-214'748'3648, -4}), -214'748.3648, doubleTolerance);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<BoostDecFloat16>(ScaledInt32{12'3, -1}), BoostDecFloat16{"12.3"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt32{-12'3, -1}), BoostDecFloat16{"-12.3"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt32{214'748'364'7, -1}),
		BoostDecFloat16{"214748364.7"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt32{214'748'3647, -4}),
		BoostDecFloat16{"214748.3647"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt32{-214'748'364'8, -1}),
		BoostDecFloat16{"-214748364.8"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt32{-214'748'3648, -4}),
		BoostDecFloat16{"-214748.3648"}, decFloat16Tolerance);

	BOOST_CHECK_CLOSE(
		converter.numberToNumber<BoostDecFloat34>(ScaledInt32{12'3, -1}), BoostDecFloat34{"12.3"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt32{-12'3, -1}), BoostDecFloat34{"-12.3"},
		decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt32{214'748'364'7, -1}),
		BoostDecFloat34{"214748364.7"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt32{214'748'3647, -4}),
		BoostDecFloat34{"214748.3647"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt32{-214'748'364'8, -1}),
		BoostDecFloat34{"-214748364.8"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt32{-214'748'3648, -4}),
		BoostDecFloat34{"-214748.3648"}, decFloat34Tolerance);
#endif

	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt32{2'147'483'647, 0}), "2147483647");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt32{-2'147'483'648, 0}), "-2147483648");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt32{214'748'364'7, -1}), "214748364.7");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt32{-214'748'364'8, -1}), "-214748364.8");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt32{214'748'3647, -4}), "214748.3647");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt32{-214'748'3648, -4}), "-214748.3648");
}

BOOST_AUTO_TEST_CASE(convertScaledInt64)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_THROW(
		converter.numberToNumber<std::int32_t>(ScaledInt64{922'337'203'685'477'580'7LL, -1}, -1), FbCppException);
	BOOST_CHECK_THROW(
		converter.numberToNumber<std::int32_t>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}, -1), FbCppException);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{12'3, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{-12'3, -1}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{922'337'203'685'477'580'7LL, -1}, 0),
		922'337'203'685'477'581LL);
	BOOST_CHECK_EQUAL(
		converter.numberToNumber<std::int64_t>(ScaledInt64{922'337'203'685'477'5807LL, -4}, 0), 922'337'203'685'478LL);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{922'337'203'685'477'580'7LL, -1}, -1),
		9'223'372'036'854'775'807LL);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}, 0),
		-922'337'203'685'477'581LL);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{-922'337'203'685'477'5807LL - 1, -4}, 0),
		-922'337'203'685'478LL);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}, -1),
		-922'337'203'685'477'580'7LL - 1);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt64{12'3, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt64{-12'3, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt64{12'3, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledInt64{-12'3, -1}, -4), -12'3000);
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt64{922'337'203'685'477'580'7LL, -1}, 0) ==
		BoostInt128{922'337'203'685'477'581});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt64{922'337'203'685'477'5807LL, -4}, 0) ==
		BoostInt128{922'337'203'685'478});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt64{922'337'203'685'477'580'7LL, -1}, -1) ==
		BoostInt128{9'223'372'036'854'775'807});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}, 0) ==
		BoostInt128{-922'337'203'685'477'581});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt64{-922'337'203'685'477'5807LL - 1, -4}, 0) ==
		BoostInt128{-922'337'203'685'478});
	BOOST_CHECK(converter.numberToNumber<BoostInt128>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}, -1) ==
		BoostInt128{-922'337'203'685'477'580'7LL - 1});
#endif

	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt64{12'3, -1}), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt64{-12'3, -1}), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt64{922'337'203'685'477'580'7LL, -1}),
		9.223372036854775807e17f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt64{922'337'203'685'477'5807LL, -4}),
		9.223372036854775807e14f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}),
		-9.223372036854775807e17f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledInt64{-922'337'203'685'477'5807LL - 1, -4}),
		-9.223372036854775807e14f, floatTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt64{12'3, -1}), 12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt64{-12'3, -1}), -12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt64{922'337'203'685'477'580'7LL, -1}),
		9.223372036854775807e17, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt64{922'337'203'685'477'5807LL, -4}),
		9.223372036854775807e14, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}),
		-9.223372036854775807e17, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(ScaledInt64{-922'337'203'685'477'5807LL - 1, -4}),
		-9.223372036854775807e14, doubleTolerance);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<BoostDecFloat16>(ScaledInt64{12'3, -1}), BoostDecFloat16{"12.3"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt64{-12'3, -1}), BoostDecFloat16{"-12.3"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt64{922'337'203'685'477'580'7LL, -1}),
		BoostDecFloat16{"922337203685477580.7"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt64{922'337'203'685'477'5807LL, -4}),
		BoostDecFloat16{"922337203685477.5807"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}),
		BoostDecFloat16{"-922337203685477580.8"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledInt64{-922'337'203'685'477'5807LL - 1, -4}),
		BoostDecFloat16{"-922337203685477.5808"}, decFloat16Tolerance);

	BOOST_CHECK_CLOSE(
		converter.numberToNumber<BoostDecFloat34>(ScaledInt64{12'3, -1}), BoostDecFloat34{"12.3"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt64{-12'3, -1}), BoostDecFloat34{"-12.3"},
		decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt64{922'337'203'685'477'580'7LL, -1}),
		BoostDecFloat34{"922337203685477580.7"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt64{922'337'203'685'477'5807LL, -4}),
		BoostDecFloat34{"922337203685477.5807"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}),
		BoostDecFloat34{"-922337203685477580.8"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledInt64{-922'337'203'685'477'5807LL - 1, -4}),
		BoostDecFloat34{"-922337203685477.5808"}, decFloat34Tolerance);
#endif

	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt64{922'337'203'685'477'5807LL, 0}), "9223372036854775807");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledInt64{-922'337'203'685'477'580'7LL - 1, 0}), "-9223372036854775808");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt64{922'337'203'685'477'580'7LL, -1}), "922337203685477580.7");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledInt64{-922'337'203'685'477'580'7LL - 1, -1}), "-922337203685477580.8");
	BOOST_CHECK_EQUAL(converter.numberToString(ScaledInt64{922'337'203'685'477'5807LL, -4}), "922337203685477.5807");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledInt64{-922'337'203'685'477'5807LL - 1, -4}), "-922337203685477.5808");
}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
BOOST_AUTO_TEST_CASE(convertScaledBoostInt128)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_THROW(converter.numberToNumber<std::int64_t>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}, -1),
		FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int64_t>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}, -1),
		FbCppException);
	BOOST_CHECK_THROW(
		converter.numberToNumber<std::int64_t>(ScaledBoostInt128{BoostInt128{"9223372036854775807"}, -1}, -4),
		FbCppException);

	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledBoostInt128{BoostInt128{12'3}, -1}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledBoostInt128{BoostInt128{-12'3}, -1}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledBoostInt128{BoostInt128{12'3}, -1}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(ScaledBoostInt128{BoostInt128{-12'3}, -1}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}, 0),
		BoostInt128{"17014118346046923173168730371588410573"});
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -4}, 0),
		BoostInt128{"17014118346046923173168730371588411"});
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}, -1),
		BoostInt128{"170141183460469231731687303715884105727"});
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}, 0),
		BoostInt128{"-17014118346046923173168730371588410573"});
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -4}, 0),
		BoostInt128{"-17014118346046923173168730371588411"});
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}, -1),
		BoostInt128{"-170141183460469231731687303715884105728"});

	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(ScaledBoostInt128{BoostInt128{12'3}, -1}), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<float>(ScaledBoostInt128{BoostInt128{-12'3}, -1}), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<float>(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}),
		1.7014118346046924e37f, floatTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<float>(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -4}),
		1.7014118346046924e34f, floatTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<float>(ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}),
		-1.7014118346046924e37f, floatTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<float>(ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -4}),
		-1.7014118346046924e34f, floatTolerance);

	BOOST_CHECK_CLOSE(
		converter.numberToNumber<double>(ScaledBoostInt128{BoostInt128{12'3}, -1}), 12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<double>(ScaledBoostInt128{BoostInt128{-12'3}, -1}), -12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<double>(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}),
		1.7014118346046924e37, doubleTolerance);
	BOOST_CHECK_CLOSE(
		converter.numberToNumber<double>(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -4}),
		1.7014118346046924e34, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}),
		-1.7014118346046924e37, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -4}),
		-1.7014118346046924e34, doubleTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledBoostInt128{BoostInt128{12'3}, -1}),
		BoostDecFloat16{"12.3"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(ScaledBoostInt128{BoostInt128{-12'3}, -1}),
		BoostDecFloat16{"-12.3"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}),
		BoostDecFloat16{"17014118346046923173168730371588410572.7"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -4}),
		BoostDecFloat16{"17014118346046923173168730371588410.5727"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}),
		BoostDecFloat16{"-17014118346046923173168730371588410572.8"}, decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -4}),
		BoostDecFloat16{"-17014118346046923173168730371588410.5728"}, decFloat16Tolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledBoostInt128{BoostInt128{12'3}, -1}),
		BoostDecFloat34{"12.3"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(ScaledBoostInt128{BoostInt128{-12'3}, -1}),
		BoostDecFloat34{"-12.3"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}),
		BoostDecFloat34{"17014118346046923173168730371588410572.7"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(
						  ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -4}),
		BoostDecFloat34{"17014118346046923173168730371588410.5727"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}),
		BoostDecFloat34{"-17014118346046923173168730371588410572.8"}, decFloat34Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(
						  ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -4}),
		BoostDecFloat34{"-17014118346046923173168730371588410.5728"}, decFloat34Tolerance);

	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, 0}),
		"170141183460469231731687303715884105727");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, 0}),
		"-170141183460469231731687303715884105728");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -1}),
		"17014118346046923173168730371588410572.7");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -1}),
		"-17014118346046923173168730371588410572.8");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledBoostInt128{BoostInt128{"170141183460469231731687303715884105727"}, -4}),
		"17014118346046923173168730371588410.5727");
	BOOST_CHECK_EQUAL(
		converter.numberToString(ScaledBoostInt128{BoostInt128{"-170141183460469231731687303715884105728"}, -4}),
		"-17014118346046923173168730371588410.5728");
}
#endif

BOOST_AUTO_TEST_CASE(convertFloat)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(12.3f, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-12.3f, -2), -12'30);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(12.3f, -4), FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(-12.3f, -4), FbCppException);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(3'276.7f, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(3.2767f, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(3'276.7f, -1), 3'276'7);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-3'276.8f, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-3.2768f, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-3'276.8f, -1), -3'276'8);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(12.3f, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-12.3f, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(12.3f, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-12.3f, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(3'276.7f, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(3.2767f, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(3'276.7f, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-3'276.8f, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-3.2768f, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-3'276.8f, -1), -32768);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(12.3f, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-12.3f, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(12.3f, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-12.3f, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(3'276.7f, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(3.2767f, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(3'276.7f, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-3'276.8f, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-3.2768f, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-3'276.8f, -1), -32768);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(12.3f, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-12.3f, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(12.3f, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-12.3f, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(3'276.7f, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(3.2767f, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(3'276.7f, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-3'276.8f, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-3.2768f, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-3'276.8f, -1), -32768);
#endif

	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(12.3f), 12.3, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(-12.3f), -12.3, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(3'276.7f), 3'276.7, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(3.2767f), 3.2767, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(-3'276.8f), -3'276.8, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(-3.2768f), -3.2768, floatTolerance);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(12.3f), BoostDecFloat16{"12.3"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(-12.3f), BoostDecFloat16{"-12.3"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(3276.7f), BoostDecFloat16{"3276.7"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(3.2767f), BoostDecFloat16{"3.2767"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(-3276.8f), BoostDecFloat16{"-3276.8"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(-3.2768f), BoostDecFloat16{"-3.2768"}, floatTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(12.3f), BoostDecFloat34{"12.3"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(-12.3f), BoostDecFloat34{"-12.3"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(3276.7f), BoostDecFloat34{"3276.7"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(3.2767f), BoostDecFloat34{"3.2767"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(-3276.8f), BoostDecFloat34{"-3276.8"}, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(-3.2768f), BoostDecFloat34{"-3.2768"}, floatTolerance);
#endif

	BOOST_CHECK_CLOSE(std::stof(converter.numberToString(12.3f)), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(std::stof(converter.numberToString(-12.3f)), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(std::stof(converter.numberToString(3276.7f)), 3276.7f, floatTolerance);
	BOOST_CHECK_CLOSE(std::stof(converter.numberToString(3.2767f)), 3.2767f, floatTolerance);
	BOOST_CHECK_CLOSE(std::stof(converter.numberToString(-3276.8f)), -3276.8f, floatTolerance);
	BOOST_CHECK_CLOSE(std::stof(converter.numberToString(-3.2768f)), -3.2768f, floatTolerance);
}

BOOST_AUTO_TEST_CASE(convertDouble)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(12.3, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-12.3, -2), -12'30);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(12.3, -4), FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(-12.3, -4), FbCppException);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(3'276.7, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(3.2767, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(3'276.7, -1), 3'276'7);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-3'276.8, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-3.2768, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(-3'276.8, -1), -3'276'8);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(12.3, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-12.3, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(12.3, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-12.3, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(3'276.7, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(3.2767, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(3'276.7, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-3'276.8, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-3.2768, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(-3'276.8, -1), -32768);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(12.3, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-12.3, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(12.3, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-12.3, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(3'276.7, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(3.2767, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(3'276.7, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-3'276.8, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-3.2768, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(-3'276.8, -1), -32768);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(12.3, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-12.3, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(12.3, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-12.3, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(3'276.7, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(3.2767, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(3'276.7, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-3'276.8, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-3.2768, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(-3'276.8, -1), -32768);
#endif

	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(12.3), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(-12.3), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(3'276.7), 3'276.7f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(3.2767), 3.2767f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(-3'276.8), -3'276.8f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(-3.2768), -3.2768f, floatTolerance);

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(12.3), BoostDecFloat16{"12.3"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(-12.3), BoostDecFloat16{"-12.3"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(3276.7), BoostDecFloat16{"3276.7"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(3.2767), BoostDecFloat16{"3.2767"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(-3276.8), BoostDecFloat16{"-3276.8"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(-3.2768), BoostDecFloat16{"-3.2768"}, doubleTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(12.3), BoostDecFloat34{"12.3"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(-12.3), BoostDecFloat34{"-12.3"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(3276.7), BoostDecFloat34{"3276.7"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(3.2767), BoostDecFloat34{"3.2767"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(-3276.8), BoostDecFloat34{"-3276.8"}, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(-3.2768), BoostDecFloat34{"-3.2768"}, doubleTolerance);
#endif

	BOOST_CHECK_CLOSE(std::stod(converter.numberToString(12.3)), 12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(std::stod(converter.numberToString(-12.3)), -12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(std::stod(converter.numberToString(3276.7)), 3276.7, doubleTolerance);
	BOOST_CHECK_CLOSE(std::stod(converter.numberToString(3.2767)), 3.2767, doubleTolerance);
	BOOST_CHECK_CLOSE(std::stod(converter.numberToString(-3276.8)), -3276.8, doubleTolerance);
	BOOST_CHECK_CLOSE(std::stod(converter.numberToString(-3.2768)), -3.2768, doubleTolerance);
}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0

BOOST_AUTO_TEST_CASE(convertDecFloat16)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"-12.3"}, -2), -12'30);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"12.3"}, -4), FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"-12.3"}, -4), FbCppException);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"3276.7"}, -1), 3'276'7);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat16{"-3276.8"}, -1), -3'276'8);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"-12.3"}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"12.3"}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"-12.3"}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"3276.7"}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat16{"-3276.8"}, -1), -32768);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"-12.3"}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"12.3"}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"-12.3"}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"3276.7"}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat16{"-3276.8"}, -1), -32768);

	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"-12.3"}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"12.3"}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"-12.3"}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"3276.7"}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat16{"-3276.8"}, -1), -32768);

	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat16{"12.3"}), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat16{"-12.3"}), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat16{"3276.7"}), 3'276.7f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat16{"3.2767"}), 3.2767f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat16{"-3276.8"}), -3'276.8f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat16{"-3.2768"}), -3.2768f, floatTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat16{"12.3"}), 12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat16{"-12.3"}), -12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat16{"3276.7"}), 3'276.7, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat16{"3.2767"}), 3.2767, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat16{"-3276.8"}), -3'276.8, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat16{"-3.2768"}), -3.2768, doubleTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(BoostDecFloat16{"12.3"}), BoostDecFloat34{"12.3"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(BoostDecFloat16{"-12.3"}), BoostDecFloat34{"-12.3"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(BoostDecFloat16{"3276.7"}), BoostDecFloat34{"3276.7"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(BoostDecFloat16{"3.2767"}), BoostDecFloat34{"3.2767"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(BoostDecFloat16{"-3276.8"}), BoostDecFloat34{"-3276.8"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat34>(BoostDecFloat16{"-3.2768"}), BoostDecFloat34{"-3.2768"},
		decFloat16Tolerance);

	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat16{"12.3"}), "12.3");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat16{"-12.3"}), "-12.3");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat16{"3276.7"}), "3276.7");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat16{"3.2767"}), "3.2767");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat16{"-3276.8"}), "-3276.8");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat16{"-3.2768"}), "-3.2768");
}

BOOST_AUTO_TEST_CASE(convertDecFloat34)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"-12.3"}, -2), -12'30);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"12.3"}, -4), FbCppException);
	BOOST_CHECK_THROW(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"-12.3"}, -4), FbCppException);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"3276.7"}, -1), 3'276'7);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int16_t>(BoostDecFloat34{"-3276.8"}, -1), -3'276'8);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"-12.3"}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"12.3"}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"-12.3"}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"3276.7"}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int32_t>(BoostDecFloat34{"-3276.8"}, -1), -32768);

	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"-12.3"}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"12.3"}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"-12.3"}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"3276.7"}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<std::int64_t>(BoostDecFloat34{"-3276.8"}, -1), -32768);

	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"12.3"}, -2), 12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"-12.3"}, -2), -12'30);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"12.3"}, -4), 12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"-12.3"}, -4), -12'3000);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"3276.7"}, 0), 3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"3.2767"}, 0), 3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"3276.7"}, -1), 32767);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"-3276.8"}, 0), -3'277);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"-3.2768"}, 0), -3);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostInt128>(BoostDecFloat34{"-3276.8"}, -1), -32768);

	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat34{"12.3"}), 12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat34{"-12.3"}), -12.3f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat34{"3276.7"}), 3'276.7f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat34{"3.2767"}), 3.2767f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat34{"-3276.8"}), -3'276.8f, floatTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<float>(BoostDecFloat34{"-3.2768"}), -3.2768f, floatTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat34{"12.3"}), 12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat34{"-12.3"}), -12.3, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat34{"3276.7"}), 3'276.7, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat34{"3.2767"}), 3.2767, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat34{"-3276.8"}), -3'276.8, doubleTolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<double>(BoostDecFloat34{"-3.2768"}), -3.2768, doubleTolerance);

	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(BoostDecFloat34{"12.3"}), BoostDecFloat16{"12.3"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(BoostDecFloat34{"-12.3"}), BoostDecFloat16{"-12.3"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(BoostDecFloat34{"3276.7"}), BoostDecFloat16{"3276.7"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(BoostDecFloat34{"3.2767"}), BoostDecFloat16{"3.2767"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(BoostDecFloat34{"-3276.8"}), BoostDecFloat16{"-3276.8"},
		decFloat16Tolerance);
	BOOST_CHECK_CLOSE(converter.numberToNumber<BoostDecFloat16>(BoostDecFloat34{"-3.2768"}), BoostDecFloat16{"-3.2768"},
		decFloat16Tolerance);

	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat34{"12.3"}), "12.3");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat34{"-12.3"}), "-12.3");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat34{"3276.7"}), "3276.7");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat34{"3.2767"}), "3.2767");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat34{"-3276.8"}), "-3276.8");
	BOOST_CHECK_EQUAL(converter.numberToString(BoostDecFloat34{"-3.2768"}), "-3.2768");
}

BOOST_AUTO_TEST_CASE(decFloat16NumberLimits)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	const auto maxValue = std::numeric_limits<BoostDecFloat16>::max();
	const auto minValue = std::numeric_limits<BoostDecFloat16>::min();
	const auto lowestValue = std::numeric_limits<BoostDecFloat16>::lowest();

	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat16>(maxValue), maxValue);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat16>(minValue), minValue);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat16>(lowestValue), lowestValue);

	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat34>(maxValue), BoostDecFloat34{maxValue});
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat34>(minValue), BoostDecFloat34{minValue});
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat34>(lowestValue), BoostDecFloat34{lowestValue});

	const auto maxString = converter.numberToString(maxValue);
	const auto minString = converter.numberToString(minValue);
	const auto lowestString = converter.numberToString(lowestValue);

	BOOST_CHECK_EQUAL(BoostDecFloat16{maxString}, maxValue);
	BOOST_CHECK_EQUAL(BoostDecFloat16{minString}, minValue);
	BOOST_CHECK_EQUAL(BoostDecFloat16{lowestString}, lowestValue);
}

BOOST_AUTO_TEST_CASE(decFloat34NumberLimits)
{
	const auto status = CLIENT.newStatus();
	impl::StatusWrapper statusWrapper{CLIENT, status.get()};

	impl::NumericConverter converter{CLIENT, &statusWrapper};

	const auto maxValue = std::numeric_limits<BoostDecFloat34>::max();
	const auto minValue = std::numeric_limits<BoostDecFloat34>::min();
	const auto lowestValue = std::numeric_limits<BoostDecFloat34>::lowest();

	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat34>(maxValue), maxValue);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat34>(minValue), minValue);
	BOOST_CHECK_EQUAL(converter.numberToNumber<BoostDecFloat34>(lowestValue), lowestValue);

	const auto maxString = converter.numberToString(maxValue);
	const auto minString = converter.numberToString(minValue);
	const auto lowestString = converter.numberToString(lowestValue);

	BOOST_CHECK_EQUAL(BoostDecFloat34{maxString}, maxValue);
	BOOST_CHECK_EQUAL(BoostDecFloat34{minString}, minValue);
	BOOST_CHECK_EQUAL(BoostDecFloat34{lowestString}, lowestValue);
}

#endif

BOOST_AUTO_TEST_SUITE_END()
