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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Statement.h"
#include "Transaction.h"
#include "Attachment.h"
#include "Client.h"

using namespace fbcpp;
using namespace fbcpp::impl;


Statement::Statement(
	Attachment& attachment, Transaction& transaction, std::string_view sql, const StatementOptions& options)
	: attachment{attachment},
	  status{attachment.getClient().newStatus()},
	  statusWrapper{attachment.getClient(), status.get()},
	  calendarConverter{attachment.getClient(), &statusWrapper},
	  numericConverter{attachment.getClient(), &statusWrapper}
{
	assert(attachment.isValid());
	assert(transaction.isValid());

	unsigned flags = fb::IStatement::PREPARE_PREFETCH_METADATA;

	if (options.getPrefetchLegacyPlan())
		flags |= fb::IStatement::PREPARE_PREFETCH_LEGACY_PLAN;

	if (options.getPrefetchPlan())
		flags |= fb::IStatement::PREPARE_PREFETCH_DETAILED_PLAN;

	statementHandle.reset(attachment.getHandle()->prepare(&statusWrapper, transaction.getHandle().get(),
		static_cast<unsigned>(sql.length()), sql.data(), SQL_DIALECT_CURRENT, flags));

	type = static_cast<StatementType>(statementHandle->getType(&statusWrapper));

	switch (type)
	{
		case StatementType::START_TRANSACTION:
			free();
			throw FbCppException("Cannot use SET TRANSACTION command with Statement class. Use Transaction class");

		case StatementType::COMMIT:
			free();
			throw FbCppException(
				"Cannot use COMMIT command with Statement class. Use the commit method from the Transaction class");

		case StatementType::ROLLBACK:
			free();
			throw FbCppException(
				"Cannot use ROLLBACK command with Statement class. Use the rollback method from the Transaction class");

		case StatementType::GET_SEGMENT:
		case StatementType::PUT_SEGMENT:
			throw FbCppException("Unsupported statement type: BLOB segment operations");

		default:
			break;
	}

	const auto processMetadata = [&](FbRef<fb::IMessageMetadata>& metadata, std::vector<Descriptor>& descriptors,
									 std::vector<std::byte>& message)
	{
		if (!metadata)
			return;

		message.resize(metadata->getMessageLength(&statusWrapper));

		FbRef<fb::IMetadataBuilder> builder;

		const auto count = metadata->getCount(&statusWrapper);
		descriptors.reserve(count);

		for (unsigned index = 0u; index < count; ++index)
		{
			Descriptor descriptor{
				.originalType = static_cast<DescriptorOriginalType>(metadata->getType(&statusWrapper, index)),
				.adjustedType = static_cast<DescriptorAdjustedType>(metadata->getType(&statusWrapper, index)),
				.scale = metadata->getScale(&statusWrapper, index),
				.length = metadata->getLength(&statusWrapper, index),
				.offset = 0,
				.nullOffset = 0,
				.isNullable = static_cast<bool>(metadata->isNullable(&statusWrapper, index)),
			};

			switch (descriptor.originalType)
			{
				case DescriptorOriginalType::TEXT:
					if (!builder)
						builder.reset(metadata->getBuilder(&statusWrapper));

					builder->setType(&statusWrapper, index, SQL_VARYING);
					descriptor.adjustedType = DescriptorAdjustedType::STRING;
					break;

				case DescriptorOriginalType::TIME_TZ_EX:
					if (!builder)
						builder.reset(metadata->getBuilder(&statusWrapper));

					builder->setType(&statusWrapper, index, SQL_TIME_TZ);
					descriptor.adjustedType = DescriptorAdjustedType::TIME_TZ;
					break;

				case DescriptorOriginalType::TIMESTAMP_TZ_EX:
					if (!builder)
						builder.reset(metadata->getBuilder(&statusWrapper));

					builder->setType(&statusWrapper, index, SQL_TIMESTAMP_TZ);
					descriptor.adjustedType = DescriptorAdjustedType::TIMESTAMP_TZ;
					break;

				default:
					break;
			}

			if (!builder)
			{
				descriptor.offset = metadata->getOffset(&statusWrapper, index);
				descriptor.nullOffset = metadata->getNullOffset(&statusWrapper, index);

				*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_TRUE;
			}

			descriptors.push_back(descriptor);
		}

		if (builder)
		{
			metadata.reset(builder->getMetadata(&statusWrapper));
			message.resize(metadata->getMessageLength(&statusWrapper));

			for (unsigned index = 0u; index < count; ++index)
			{
				auto& descriptor = descriptors[index];
				descriptor.offset = metadata->getOffset(&statusWrapper, index);
				descriptor.nullOffset = metadata->getNullOffset(&statusWrapper, index);

				*reinterpret_cast<std::int16_t*>(&message[descriptor.nullOffset]) = FB_TRUE;
			}
		}
	};

	inMetadata.reset(statementHandle->getInputMetadata(&statusWrapper));
	processMetadata(inMetadata, inDescriptors, inMessage);

	outMetadata.reset(statementHandle->getOutputMetadata(&statusWrapper));
	processMetadata(outMetadata, outDescriptors, outMessage);
}

void Statement::free()
{
	assert(isValid());

	if (resultSetHandle)
	{
		resultSetHandle->close(&statusWrapper);
		resultSetHandle.reset();
	}

	statementHandle->free(&statusWrapper);
	statementHandle.reset();
}

std::string Statement::getLegacyPlan()
{
	assert(isValid());

	return statementHandle->getPlan(&statusWrapper, false);
}

std::string Statement::getPlan()
{
	assert(isValid());

	return statementHandle->getPlan(&statusWrapper, true);
}

bool Statement::execute(Transaction& transaction)
{
	assert(isValid());
	assert(transaction.isValid());

	if (resultSetHandle)
	{
		resultSetHandle->close(&statusWrapper);
		resultSetHandle.reset();
	}

	const auto outMessageData = outMessage.data();

	if (outMessageData)
	{
		for (const auto& descriptor : outDescriptors)
			*reinterpret_cast<std::int16_t*>(&outMessageData[descriptor.nullOffset]) = FB_TRUE;
	}

	switch (type)
	{
		case StatementType::SELECT:
		case StatementType::SELECT_FOR_UPDATE:
			resultSetHandle.reset(statementHandle->openCursor(&statusWrapper, transaction.getHandle().get(),
				inMetadata.get(), inMessage.data(), outMetadata.get(), 0));
			return resultSetHandle->fetchNext(&statusWrapper, outMessageData) == fb::IStatus::RESULT_OK;

		default:
			statementHandle->execute(&statusWrapper, transaction.getHandle().get(), inMetadata.get(), inMessage.data(),
				outMetadata.get(), outMessageData);
			return true;
	}
}

bool Statement::fetchNext()
{
	assert(isValid());

	return resultSetHandle && resultSetHandle->fetchNext(&statusWrapper, outMessage.data()) == fb::IStatus::RESULT_OK;
}

bool Statement::fetchPrior()
{
	assert(isValid());

	return resultSetHandle && resultSetHandle->fetchPrior(&statusWrapper, outMessage.data()) == fb::IStatus::RESULT_OK;
}

bool Statement::fetchFirst()
{
	assert(isValid());

	return resultSetHandle && resultSetHandle->fetchFirst(&statusWrapper, outMessage.data()) == fb::IStatus::RESULT_OK;
}

bool Statement::fetchLast()
{
	assert(isValid());

	return resultSetHandle && resultSetHandle->fetchLast(&statusWrapper, outMessage.data()) == fb::IStatus::RESULT_OK;
}

bool Statement::fetchAbsolute(unsigned position)
{
	assert(isValid());

	return resultSetHandle &&
		resultSetHandle->fetchAbsolute(&statusWrapper, static_cast<int>(position), outMessage.data()) ==
		fb::IStatus::RESULT_OK;
}

bool Statement::fetchRelative(int offset)
{
	assert(isValid());

	return resultSetHandle &&
		resultSetHandle->fetchRelative(&statusWrapper, offset, outMessage.data()) == fb::IStatus::RESULT_OK;
}
