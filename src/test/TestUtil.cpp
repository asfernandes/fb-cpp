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

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "TestUtil.h"
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;


namespace fbcpp::test
{
	// Client CLIENT{"fbclient"};
	Client CLIENT{fb::fb_get_master_interface()};

	namespace
	{
		fs::path tempDir;
		bool removeTempDir = false;
		std::string testServerPrefix;

		struct GlobalFixture
		{
			GlobalFixture()
			{
				const char* testDirEnv = std::getenv("FBCPP_TEST_DIR");
				const char* testServerEnv = std::getenv("FBCPP_TEST_SERVER");
				if (testServerEnv && *testServerEnv)
					testServerPrefix = std::string(testServerEnv) + ":";

				if (testDirEnv && *testDirEnv)
				{
					tempDir = testDirEnv;
					return;
				}

				const fs::path prefix = fs::temp_directory_path();

				auto now = std::chrono::system_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

				std::ostringstream oss;
				oss << "fbcpp-test-" << time;

				tempDir = prefix / oss.str();

				if (fs::create_directory(tempDir))
					removeTempDir = true;
			}

			~GlobalFixture()
			{
				if (removeTempDir)
				{
					std::error_code ec;
					fs::remove(tempDir, ec);
				}

				CLIENT.shutdown();
			}
		};
	}  // namespace

	std::string getTempFile(const std::string_view name)
	{
		return testServerPrefix + (tempDir / name).string();
	}
}  // namespace fbcpp::test


BOOST_GLOBAL_FIXTURE(GlobalFixture);
