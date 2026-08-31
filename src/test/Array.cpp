/*
 * MIT License
 *
 * Copyright (c) 2026 Adriano dos Santos Fernandes
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
#include "fb-cpp/Array.h"
#include "fb-cpp/Attachment.h"
#include "fb-cpp/Statement.h"
#include "fb-cpp/Transaction.h"
#include <array>
#include <cstdint>
#include <variant>


BOOST_AUTO_TEST_SUITE(ArraySuite)

BOOST_AUTO_TEST_CASE(readWriteAndBind)
{
	const auto database = getTempFile("Array-readWriteAndBind.fdb");

	Attachment attachment{getClient(), database,
		AttachmentOptions().setCreateDatabase(true).setForcedWrites(false).setConnectionCharSet("UTF8")};
	FbDropDatabase attachmentDrop{attachment};

	{  // scope
		Transaction transaction{attachment};
		Statement statement{attachment, transaction,
			"recreate table array_test (id integer, numbers integer[1:4], grid integer[0:1, 5:6])"};
		BOOST_REQUIRE(statement.execute(transaction));
		transaction.commit();
	}

	const ArrayDescriptor numbersDescriptor{
		.relation = "ARRAY_TEST",
		.field = "NUMBERS",
		.elementType = DescriptorOriginalType::LONG,
		.scale = 0,
		.elementLength = sizeof(std::int32_t),
		.bounds = {{1, 4}},
	};
	const ArrayDescriptor gridDescriptor{
		.relation = "ARRAY_TEST",
		.field = "GRID",
		.elementType = DescriptorOriginalType::LONG,
		.scale = 0,
		.elementLength = sizeof(std::int32_t),
		.bounds = {{0, 1}, {5, 6}},
	};
	const std::array<std::int32_t, 4> numbers = {10, 20, 30, 40};
	const std::array<std::array<std::int32_t, 2>, 2> grid = {{{1, 2}, {3, 4}}};

	{  // scope
		Transaction transaction{attachment};
		Array numbersArray{attachment, transaction, numbersDescriptor};
		Array gridArray{attachment, transaction, gridDescriptor};
		numbersArray.write(std::span{numbers});
		gridArray.write(std::span{grid});

		Statement insert{attachment, transaction, "insert into array_test (id, numbers, grid) values (?, ?, ?)"};
		insert.set(0, 1);
		insert.set(1, numbersArray.getId());
		insert.set(2, gridArray.getId());
		BOOST_REQUIRE(insert.execute(transaction));

		Statement insertNull{attachment, transaction, "insert into array_test (id, numbers) values (?, ?)"};
		insertNull.set(0, 2);
		insertNull.set(1, std::optional<ArrayId>{});
		BOOST_REQUIRE(insertNull.execute(transaction));
		transaction.commit();
	}

	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select numbers, grid from array_test where id = ?"};
		select.set(0, 1);
		BOOST_REQUIRE(select.execute(transaction));

		const auto numbersId = select.getArrayId(0);
		const auto gridId = select.get<std::optional<ArrayId>>(1);
		BOOST_REQUIRE(numbersId.has_value());
		BOOST_REQUIRE(gridId.has_value());

		Array numbersArray{attachment, transaction, numbersDescriptor, numbersId.value()};
		Array gridArray{attachment, transaction, gridDescriptor, gridId.value()};
		std::array<std::int32_t, 4> receivedNumbers{};
		std::array<std::array<std::int32_t, 2>, 2> receivedGrid{};
		BOOST_CHECK_EQUAL(numbersArray.getSliceLength(), sizeof(receivedNumbers));
		BOOST_CHECK_EQUAL(gridArray.getSliceLength(), sizeof(receivedGrid));
		BOOST_CHECK_EQUAL(numbersArray.read(std::span{receivedNumbers}), sizeof(receivedNumbers));
		BOOST_CHECK_EQUAL(gridArray.read(std::span{receivedGrid}), sizeof(receivedGrid));
		BOOST_CHECK_EQUAL_COLLECTIONS(receivedNumbers.begin(), receivedNumbers.end(), numbers.begin(), numbers.end());
		for (std::size_t row = 0; row < receivedGrid.size(); ++row)
		{
			BOOST_CHECK_EQUAL_COLLECTIONS(
				receivedGrid[row].begin(), receivedGrid[row].end(), grid[row].begin(), grid[row].end());
		}

		Statement elementSelect{attachment, transaction,
			"select numbers[1], numbers[2], numbers[3], numbers[4], "
			"grid[0, 5], grid[0, 6], grid[1, 5], grid[1, 6] "
			"from array_test where id = ?"};
		elementSelect.set(0, 1);
		BOOST_REQUIRE(elementSelect.execute(transaction));
		for (std::size_t index = 0; index < numbers.size(); ++index)
			BOOST_CHECK_EQUAL(elementSelect.getInt32(static_cast<unsigned>(index)).value(), numbers[index]);
		for (std::size_t row = 0; row < grid.size(); ++row)
		{
			for (std::size_t column = 0; column < grid[row].size(); ++column)
			{
				const auto index = static_cast<unsigned>(numbers.size() + row * grid[row].size() + column);
				BOOST_CHECK_EQUAL(elementSelect.getInt32(index).value(), grid[row][column]);
			}
		}

		Statement variantSelect{attachment, transaction, "select numbers from array_test where id = 1"};
		BOOST_REQUIRE(variantSelect.execute(transaction));
		const auto value = variantSelect.get<std::variant<std::monostate, ArrayId>>(0);
		BOOST_CHECK(std::holds_alternative<ArrayId>(value));
	}

	{  // scope
		Transaction transaction{attachment};
		Statement select{attachment, transaction, "select numbers from array_test where id = ?"};
		select.set(0, 2);
		BOOST_REQUIRE(select.execute(transaction));
		BOOST_CHECK(select.getArrayId(0) == std::nullopt);
	}
}

BOOST_AUTO_TEST_SUITE_END()
