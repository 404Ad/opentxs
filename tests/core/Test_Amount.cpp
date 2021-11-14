// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/exception/exception.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/Amount.hpp"
#include "opentxs/core/Amount.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/util/Bytes.hpp"

using namespace opentxs;

namespace bmp = boost::multiprecision;

constexpr auto int_max = std::numeric_limits<int>::max();
constexpr auto int_min = std::numeric_limits<int>::min();
constexpr auto long_max = std::numeric_limits<long int>::max();
constexpr auto long_min = std::numeric_limits<long int>::min();
constexpr auto longlong_max = std::numeric_limits<long long int>::max();
constexpr auto longlong_min = std::numeric_limits<long long int>::min();
constexpr auto uint_max = std::numeric_limits<unsigned int>::max();
constexpr auto uint_min = std::numeric_limits<unsigned int>::min();
constexpr auto ulong_max = std::numeric_limits<unsigned long int>::max();
constexpr auto ulong_min = std::numeric_limits<unsigned long int>::min();
constexpr auto ulonglong_max =
    std::numeric_limits<unsigned long long int>::max();
constexpr auto ulonglong_min =
    std::numeric_limits<unsigned long long int>::min();
// constexpr auto int64_max = std::numeric_limits<int64_t>::max();

TEST(Amount, default_constructor)
{
    const auto amount = Amount();

    ASSERT_TRUE(amount == 0ll);
}

TEST(Amount, int_constructor)
{
    const auto max_amount = Amount(int_max);

    ASSERT_TRUE(max_amount == int_max);

    const auto min_amount = Amount(int_min);

    ASSERT_TRUE(min_amount == int_min);
}

TEST(Amount, long_constructor)
{
    const auto max_amount = Amount(long_max);

    ASSERT_TRUE(max_amount == long_max);

    const auto min_amount = Amount(long_min);

    ASSERT_TRUE(min_amount == long_min);
}

TEST(Amount, longlong_constructor)
{
    const auto max_amount = Amount(longlong_max);

    ASSERT_TRUE(max_amount == longlong_max);

    const auto min_amount = Amount(longlong_min);

    ASSERT_TRUE(min_amount == longlong_min);
}

TEST(Amount, uint_constructor)
{
    const auto max_amount = Amount(uint_max);

    ASSERT_TRUE(max_amount == uint_max);

    const auto min_amount = Amount(uint_min);

    ASSERT_TRUE(min_amount == uint_min);
}

TEST(Amount, ulong_constructor)
{
    const auto max_amount = Amount(ulong_max);

    ASSERT_TRUE(max_amount == ulong_max);

    const auto min_amount = Amount(ulong_min);

    ASSERT_TRUE(min_amount == ulong_min);
}

TEST(Amount, ulonglong_constructor)
{
    const auto max_amount = Amount(ulonglong_max);

    ASSERT_TRUE(max_amount == ulonglong_max);

    const auto min_amount = Amount(ulonglong_min);

    ASSERT_TRUE(min_amount == ulonglong_min);
}

TEST(Amount, string_constructor)
{
    const auto max_amount = ulonglong_max;
    const auto amount = Amount(std::to_string(max_amount));

    ASSERT_TRUE(amount == max_amount);

    try {
        const auto overflow =
            std::numeric_limits<bmp::checked_int1024_t>::max();
        const auto overflow_amount = Amount(overflow.str());
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto underflow =
            std::numeric_limits<bmp::checked_int1024_t>::min();
        const auto underflow_amount = Amount(underflow.str());
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}

TEST(Amount, zmqframe_constructor)
{
    opentxs::OTZMQMessage message{opentxs::network::zeromq::Message::Factory()};

    const auto max = std::to_string(ulonglong_max);
    message->AddFrame(max);

    const auto amount = Amount(message->at(0));
    ASSERT_TRUE(amount == ulonglong_max);
}

TEST(Amount, copy_constructor)
{
    const auto amount = Amount(ulonglong_max);

    const auto copy = Amount(amount);

    ASSERT_TRUE(copy == ulonglong_max);
}

TEST(Amount, move_constructor)
{
    const auto amount = std::make_unique<Amount>(ulonglong_max);

    const auto moved = Amount(std::move(*amount));

    ASSERT_TRUE(moved == ulonglong_max);
}

TEST(Amount, copy_assignment)
{
    const auto amount = Amount(ulonglong_max);

    auto copy = Amount();

    copy = amount;

    ASSERT_TRUE(copy == ulonglong_max);
}

TEST(Amount, move_assignment)
{
    const auto amount = std::make_unique<Amount>(ulonglong_max);

    auto moved = Amount();

    moved = std::move(*amount);

    ASSERT_TRUE(moved == ulonglong_max);
}

TEST(Amount, str)
{
    const auto str = std::to_string(longlong_max);

    const auto amount = Amount(str);

    ASSERT_EQ(str, amount.str());
}

TEST(Amount, string_operator)
{
    const auto amount = Amount(ulonglong_max);

    const std::string conversion_amount = amount;

    ASSERT_EQ(conversion_amount, std::to_string(ulonglong_max));
}

TEST(Amount, unary_minus_operator)
{
    auto amount = Amount{1};

    ASSERT_TRUE(-amount == -1);
}

// TEST(Amount, SerializeBitcoin)
//{
//    auto bytes = Space{};
//
//    auto amount = Amount(-1);
//
//    ASSERT_FALSE(amount.SerializeBitcoin(writer(bytes)));
//
//    amount = ulonglong_max;
//
//    ASSERT_FALSE(amount.SerializeBitcoin(writer(bytes)));
//
//    amount = int64_max;
//
//    ASSERT_TRUE(amount.SerializeBitcoin(writer(bytes)));
//
//    const auto buffer = boost::endian::little_int64_buf_t(int64_max);
//    const auto view =
//        ReadView(reinterpret_cast<const char*>(&buffer), sizeof(buffer));
//
//    ASSERT_EQ(view, reader(bytes));
//}

// TEST(Amount, SerializeBitcoinSize)
//{
//    ASSERT_EQ(
//        Amount::SerializeBitcoinSize(),
//        sizeof(boost::endian::little_int64_buf_t));
//}

TEST(Amount, less_than)
{
    const Amount int_amount{int_max - 1};

    ASSERT_TRUE(int_amount < int_max);
    ASSERT_FALSE(int_amount < int_max - 2);

    ASSERT_TRUE(int_max - 2 < int_amount);
    ASSERT_FALSE(int_max < int_amount);

    const Amount long_amount{long_max - 1};

    ASSERT_TRUE(long_amount < long_max);
    ASSERT_FALSE(long_amount < long_max - 2);

    ASSERT_TRUE(long_max - 2 < long_amount);
    ASSERT_FALSE(long_max < long_amount);

    const Amount longlong_amount{longlong_max - 1};

    ASSERT_TRUE(longlong_amount < longlong_max);
    ASSERT_FALSE(longlong_amount < longlong_max - 2);

    ASSERT_TRUE(longlong_max - 2 < longlong_amount);
    ASSERT_FALSE(longlong_max < longlong_amount);

    const Amount ulonglong_amount{ulonglong_max - 1};

    ASSERT_TRUE(ulonglong_amount < ulonglong_max);
    ASSERT_FALSE(ulonglong_amount < ulonglong_max - 2);

    ASSERT_TRUE(ulonglong_max - 2 < ulonglong_amount);
    ASSERT_FALSE(ulonglong_max < ulonglong_amount);

    ASSERT_TRUE(Amount{0} < ulonglong_amount);
    ASSERT_FALSE(ulonglong_amount < Amount{0});
}

TEST(Amount, greater_than)
{
    const Amount int_amount{int_max - 1};

    ASSERT_TRUE(int_amount > int_max - 2);
    ASSERT_FALSE(int_amount > int_max);

    ASSERT_TRUE(int_max > int_amount);
    ASSERT_FALSE(int_max - 2 > int_amount);

    const Amount long_amount{long_max - 1};

    ASSERT_TRUE(long_amount > long_max - 2);
    ASSERT_FALSE(long_amount > long_max);

    ASSERT_TRUE(long_max > long_amount);
    ASSERT_FALSE(long_max - 2 > long_amount);

    const Amount longlong_amount{longlong_max - 1};

    ASSERT_TRUE(longlong_amount > longlong_max - 2);
    ASSERT_FALSE(longlong_amount > longlong_max);

    ASSERT_TRUE(longlong_max > longlong_amount);
    ASSERT_FALSE(longlong_max - 2 > longlong_amount);

    const Amount ulonglong_amount{ulonglong_max - 1};

    ASSERT_TRUE(ulonglong_amount > ulonglong_max - 2);
    ASSERT_FALSE(ulonglong_amount > ulonglong_max);

    ASSERT_TRUE(ulonglong_max > ulonglong_amount);
    ASSERT_FALSE(ulonglong_max - 2 > ulonglong_amount);

    ASSERT_TRUE(Amount{ulonglong_max} > ulonglong_amount);
    ASSERT_FALSE(Amount{0} > ulonglong_amount);
}

TEST(Amount, equal_to)
{
    const Amount int_amount{int_max};

    ASSERT_TRUE(int_amount == int_max);
    ASSERT_FALSE(int_amount == int_max - 1);

    ASSERT_TRUE(int_max == int_amount);
    ASSERT_FALSE(int_max - 1 == int_amount);

    const Amount long_amount{long_max};

    ASSERT_TRUE(long_amount == long_max);
    ASSERT_FALSE(long_amount == long_max - 1);

    ASSERT_TRUE(long_max == long_amount);
    ASSERT_FALSE(long_max - 1 == long_amount);

    const Amount longlong_amount{longlong_max};

    ASSERT_TRUE(longlong_amount == longlong_max);
    ASSERT_FALSE(longlong_amount == longlong_max - 1);

    ASSERT_TRUE(longlong_max == longlong_amount);
    ASSERT_FALSE(longlong_max - 1 == longlong_amount);

    const Amount uint_amount{uint_max};

    ASSERT_TRUE(uint_amount == uint_max);
    ASSERT_FALSE(uint_amount == uint_max - 1);

    const Amount ulong_amount{ulong_max};

    ASSERT_TRUE(ulong_amount == ulong_max);
    ASSERT_FALSE(ulong_amount == ulong_max - 1);

    const Amount ulonglong_amount{ulonglong_max};

    ASSERT_TRUE(ulonglong_amount == ulonglong_max);
    ASSERT_FALSE(ulonglong_amount == ulonglong_max - 1);

    ASSERT_TRUE(ulonglong_max == ulonglong_amount);
    ASSERT_FALSE(ulonglong_max - 1 == ulonglong_amount);

    ASSERT_TRUE(ulong_amount == ulong_max);
    ASSERT_FALSE(ulong_amount == ulong_max - 1);

    ASSERT_TRUE(Amount{ulonglong_max} == ulonglong_amount);
    ASSERT_FALSE(Amount{0} == ulonglong_amount);
}

TEST(Amount, not_equal_to)
{
    const Amount int_amount{int_max};

    ASSERT_TRUE(int_amount != int_max - 1);
    ASSERT_FALSE(int_amount != int_max);

    ASSERT_TRUE(int_max - 1 != int_amount);
    ASSERT_FALSE(int_max != int_amount);

    const Amount long_amount{long_max};

    ASSERT_TRUE(long_amount != long_max - 1);
    ASSERT_FALSE(long_amount != long_max);

    const Amount longlong_amount{longlong_max};

    ASSERT_TRUE(longlong_amount != longlong_max - 1);
    ASSERT_FALSE(longlong_amount != longlong_max);

    ASSERT_TRUE(longlong_max - 1 != longlong_amount);
    ASSERT_FALSE(longlong_max != longlong_amount);

    const Amount ulonglong_amount{ulonglong_max};

    ASSERT_TRUE(ulonglong_amount != ulonglong_max - 1);
    ASSERT_FALSE(ulonglong_amount != ulonglong_max);

    ASSERT_TRUE(ulonglong_max - 1 != ulonglong_amount);
    ASSERT_FALSE(ulonglong_max != ulonglong_amount);

    ASSERT_TRUE(Amount{ulonglong_max - 1} != ulonglong_amount);
    ASSERT_FALSE(Amount{ulonglong_max} != ulonglong_amount);
}

TEST(Amount, less_than_or_equal_to)
{
    const Amount int_amount{int_max - 1};

    ASSERT_TRUE(int_amount <= int_max);
    ASSERT_TRUE(int_amount <= int_max - 1);
    ASSERT_FALSE(int_amount <= int_max - 2);

    const Amount long_amount{long_max - 1};

    ASSERT_TRUE(long_max - 2 <= long_amount);
    ASSERT_TRUE(long_max - 1 <= long_amount);
    ASSERT_FALSE(long_max <= long_amount);

    const Amount longlong_amount{longlong_max - 1};

    ASSERT_TRUE(longlong_amount <= longlong_max);
    ASSERT_TRUE(longlong_amount <= longlong_max - 1);
    ASSERT_FALSE(longlong_amount <= longlong_max - 2);

    ASSERT_TRUE(longlong_max - 2 <= longlong_amount);
    ASSERT_TRUE(longlong_max - 1 <= longlong_amount);
    ASSERT_FALSE(longlong_max <= longlong_amount);

    const Amount uint_amount{uint_max - 1};

    ASSERT_TRUE(uint_amount <= uint_max);
    ASSERT_TRUE(uint_amount <= uint_max - 1);
    ASSERT_FALSE(uint_amount <= uint_max - 2);

    const Amount ulong_amount{ulong_max - 1};

    ASSERT_TRUE(ulong_amount <= ulong_max);
    ASSERT_TRUE(ulong_amount <= ulong_max - 1);
    ASSERT_FALSE(ulong_amount <= ulong_max - 2);

    const Amount ulonglong_amount{ulonglong_max - 1};

    ASSERT_TRUE(ulonglong_amount <= ulonglong_max);
    ASSERT_TRUE(ulonglong_amount <= ulonglong_max - 1);
    ASSERT_FALSE(ulonglong_amount <= ulonglong_max - 2);

    ASSERT_TRUE(ulonglong_max - 2 <= ulonglong_amount);
    ASSERT_TRUE(ulonglong_max - 1 <= ulonglong_amount);
    ASSERT_FALSE(ulonglong_max <= ulonglong_amount);

    ASSERT_TRUE(Amount{ulonglong_max - 2} <= ulonglong_amount);
    ASSERT_TRUE(Amount{ulonglong_max - 1} <= ulonglong_amount);
    ASSERT_FALSE(Amount{ulonglong_max} <= ulonglong_amount);
}

TEST(Amount, greater_than_or_equal_to)
{
    const Amount int_amount{int_max - 1};

    ASSERT_TRUE(int_max >= int_amount);
    ASSERT_TRUE(int_max - 1 >= int_amount);
    ASSERT_FALSE(int_max - 2 >= int_amount);

    const Amount long_amount{long_max - 1};

    ASSERT_TRUE(long_amount >= long_max - 2);
    ASSERT_TRUE(long_amount >= long_max - 1);
    ASSERT_FALSE(long_amount >= long_max);

    const Amount longlong_amount{longlong_max - 1};

    ASSERT_TRUE(longlong_amount >= longlong_max - 2);
    ASSERT_TRUE(longlong_amount >= longlong_max - 1);
    ASSERT_FALSE(longlong_amount >= longlong_max);

    ASSERT_TRUE(longlong_max >= longlong_amount);
    ASSERT_TRUE(longlong_max - 1 >= longlong_amount);
    ASSERT_FALSE(longlong_max - 2 >= longlong_amount);

    const Amount ulonglong_amount{ulonglong_max - 1};

    ASSERT_TRUE(ulonglong_amount >= ulonglong_max - 2);
    ASSERT_TRUE(ulonglong_amount >= ulonglong_max - 1);
    ASSERT_FALSE(ulonglong_amount >= ulonglong_max);

    ASSERT_TRUE(ulonglong_max >= ulonglong_amount);
    ASSERT_TRUE(ulonglong_max - 1 >= ulonglong_amount);
    ASSERT_FALSE(ulonglong_max - 2 >= ulonglong_amount);

    ASSERT_TRUE(Amount{ulonglong_max} >= ulonglong_amount);
    ASSERT_TRUE(Amount{ulonglong_max - 1} >= ulonglong_amount);
    ASSERT_FALSE(Amount{ulonglong_max - 2} >= ulonglong_amount);
}

TEST(Amount, plus)
{
    const Amount amount;

    ASSERT_TRUE(1l + amount == 1);

    ASSERT_TRUE(1ll + amount == 1);

    ASSERT_TRUE(1ul + amount == 1);

    const auto long_total_amount = amount + 1l;

    ASSERT_TRUE(long_total_amount == 1l);

    const auto longlong_total_amount = amount + 1ll;

    ASSERT_TRUE(longlong_total_amount == 1ll);

    const auto uint_total_amount = amount + 1u;

    ASSERT_TRUE(uint_total_amount == 1);

    const auto ulong_total_amount = amount + 1ul;

    ASSERT_TRUE(ulong_total_amount == 1ul);

    const auto ulonglong_total_amount = amount + 1ull;

    ASSERT_TRUE(ulonglong_total_amount == 1ull);

    ASSERT_TRUE(Amount{1} + amount == 1);

    auto ulonglong_amount = Amount{1};
    ulonglong_amount += 1ul;

    ASSERT_TRUE(ulonglong_amount == 2);

    ulonglong_amount += Amount{1};

    ASSERT_TRUE(ulonglong_amount == 3);

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        auto overflow = amount_max + 1l;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        auto overflow = amount_max + 1ll;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        auto overflow = amount_max + 1u;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        auto overflow = amount_max + 1ul;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        auto overflow = amount_max + 1ull;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        auto overflow = amount_max + Amount{1};
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        amount_max += 1ul;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount_max = Amount{max.str()};
        amount_max += Amount{1};
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}

TEST(Amount, minus)
{
    const Amount amount;

    ASSERT_TRUE(1l - amount == 1);

    const Amount long_amount{long_max};

    const auto long_result_amount = long_amount - 1l;

    ASSERT_TRUE(long_result_amount == long_max - 1l);

    const Amount longlong_amount{longlong_max};

    const auto longlong_result_amount = longlong_amount - 1ll;

    ASSERT_TRUE(longlong_result_amount == longlong_max - 1ll);

    const Amount ulong_amount{ulong_max};

    const auto ulong_result_amount = ulong_amount - 1ul;

    ASSERT_TRUE(ulong_result_amount == ulong_max - 1ul);

    Amount ulonglong_amount{ulonglong_max};

    const auto ulonglong_result_amount = ulonglong_amount - 1ull;

    ASSERT_TRUE(ulonglong_result_amount == ulonglong_max - 1ull);

    ASSERT_TRUE(Amount{1} - amount == 1);

    ulonglong_amount = Amount{3};
    ulonglong_amount -= 1ul;

    ASSERT_TRUE(ulonglong_amount == 2);

    ulonglong_amount -= Amount{1};

    ASSERT_TRUE(ulonglong_amount == 1);

    try {
        const auto min = std::numeric_limits<Amount::Imp::Backend>::min();
        const auto amount = Amount{min.str()};
        const auto negative_result = amount - 1l;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto min = std::numeric_limits<Amount::Imp::Backend>::min();
        const auto amount = Amount{min.str()};
        const auto negative_result = amount - 1ll;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto min = std::numeric_limits<Amount::Imp::Backend>::min();
        const auto amount = Amount{min.str()};
        const auto negative_result = amount - 1ul;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto min = std::numeric_limits<Amount::Imp::Backend>::min();
        const auto amount = Amount{min.str()};
        const auto negative_result = amount - 1ull;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto min = std::numeric_limits<Amount::Imp::Backend>::min();
        auto amount_min = Amount{min.str()};
        auto underflow = amount_min - Amount{1};
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto min = std::numeric_limits<Amount::Imp::Backend>::min();
        auto amount_min = Amount{min.str()};
        amount_min -= 1ul;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto min = std::numeric_limits<Amount::Imp::Backend>::min();
        auto amount_min = Amount{min.str()};
        amount_min -= Amount{1};
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}

TEST(Amount, multiply)
{
    const Amount int_amount{int_max / 2};

    auto int_result = int_amount * 2;

    ASSERT_TRUE(int_result == ((int_max / 2) * 2));

    int_result = 2 * int_amount;

    ASSERT_TRUE(int_result == ((int_max / 2) * 2));

    const Amount long_amount{long_max / 2};

    auto long_result = long_amount * 2l;

    ASSERT_TRUE(long_result == ((long_max / 2) * 2));

    const Amount longlong_amount{longlong_max / 2};

    auto longlong_result = longlong_amount * 2ll;

    ASSERT_TRUE(longlong_result == ((longlong_max / 2) * 2));

    longlong_result = 2ll * longlong_amount;

    ASSERT_TRUE(longlong_result == ((longlong_max / 2) * 2));

    const Amount uint_amount{uint_max / 2};

    const auto uint_result = 2ul * uint_amount;

    ASSERT_TRUE(uint_result == ((uint_max / 2) * 2));

    const Amount ulong_amount{ulong_max / 2};

    const auto ulong_result = 2ul * ulong_amount;

    ASSERT_TRUE(ulong_result == ((ulong_max / 2) * 2));

    const Amount ulonglong_amount{ulonglong_max / 2};

    auto ulonglong_result = ulonglong_amount * 2ull;

    ASSERT_TRUE(ulonglong_result == ((ulonglong_max / 2) * 2));

    ulonglong_result = 2ull * ulonglong_amount;

    ASSERT_TRUE(ulonglong_result == ((ulonglong_max / 2) * 2));

    ASSERT_TRUE(Amount{2} * ulonglong_amount == ((ulonglong_max / 2) * 2));

    ulonglong_result = ulonglong_amount;
    ulonglong_result *= Amount{2};

    ASSERT_TRUE(ulonglong_result == ((ulonglong_max / 2) * 2));

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = amount * 2;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = 2 * amount;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = amount * 2l;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = amount * 2ll;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = 2ll * amount;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = amount * 2ull;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = 2ull * amount;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        const auto amount = Amount{max.str()};
        const auto overflow = amount * amount;
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto max = std::numeric_limits<Amount::Imp::Backend>::max();
        auto amount = Amount{max.str()};
        amount *= Amount{2};
        EXPECT_TRUE(false);
    } catch (std::overflow_error&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}

TEST(Amount, divide)
{
    const Amount int_amount{int_max};

    const auto int_result = int_amount / 2;

    ASSERT_TRUE(int_result == int_max / 2);

    const Amount longlong_amount{longlong_max};

    const auto longlong_result = longlong_amount / 2ll;

    ASSERT_TRUE(longlong_result == longlong_max / 2);

    const Amount ulonglong_amount{ulonglong_max};

    const auto ulonglong_result = ulonglong_amount / 2ull;

    ASSERT_TRUE(ulonglong_result == ulonglong_max / 2);

    ASSERT_TRUE(ulonglong_amount / Amount{2} == ulonglong_max / 2);

    try {
        const auto result = int_amount / 0;
        EXPECT_TRUE(false);
    } catch (std::exception&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto result = longlong_amount / 0ll;
        EXPECT_TRUE(false);
    } catch (std::exception&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto result = ulonglong_amount / 0ull;
        EXPECT_TRUE(false);
    } catch (std::exception&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    try {
        const auto result = ulonglong_amount / Amount{};
        EXPECT_TRUE(false);
    } catch (std::exception&) {
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}

TEST(Amount, modulo)
{
    const Amount int_amount{3};

    const auto int_result = int_amount % 2;

    ASSERT_TRUE(int_result == 1);

    const Amount longlong_amount{3ll};

    const auto longlong_result = longlong_amount % 2ll;

    ASSERT_TRUE(longlong_result == 1ll);

    const Amount ulonglong_amount{3ull};

    const auto ulonglong_result = ulonglong_amount % 2ull;

    ASSERT_TRUE(ulonglong_result == 1ull);

    ASSERT_TRUE(ulonglong_amount % Amount{2} == 1);

    //    try {
    //        const auto result = int_amount % 0;
    //        EXPECT_TRUE(false);
    //    } catch (std::exception&) {
    //        EXPECT_TRUE(true);
    //    } catch (...) {
    //        EXPECT_TRUE(false);
    //    }
    //
    //    try {
    //        const auto result = longlong_amount % 0ll;
    //        EXPECT_TRUE(false);
    //    } catch (std::exception&) {
    //        EXPECT_TRUE(true);
    //    } catch (...) {
    //        EXPECT_TRUE(false);
    //    }
    //
    //    try {
    //        const auto result = ulonglong_amount % 0ull;
    //        EXPECT_TRUE(false);
    //    } catch (std::exception&) {
    //        EXPECT_TRUE(true);
    //    } catch (...) {
    //        EXPECT_TRUE(false);
    //    }
    //
    //    try {
    //        const auto result = ulonglong_amount % Amount{};
    //        EXPECT_TRUE(false);
    //    } catch (std::exception&) {
    //        EXPECT_TRUE(true);
    //    } catch (...) {
    //        EXPECT_TRUE(false);
    //    }
}