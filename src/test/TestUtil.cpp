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
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;


namespace fbcpp::test
{
	Client CLIENT{"fbclient"};

	namespace
	{
		class TempDir
		{
		public:
			explicit TempDir()
			{
				fs::path tempPath = fs::temp_directory_path();

				auto now = std::chrono::system_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

				std::ostringstream oss;
				oss << "fbcpp-test-" << time;

				path = tempPath / oss.str();

				fs::create_directory(path);
			}

			~TempDir()
			{
				std::error_code ec;
				fs::remove(path, ec);
			}

		public:
			auto get() const
			{
				return path;
			}

		private:
			fs::path path;
		} tempDir;

		struct ClientCleanup final
		{
			~ClientCleanup()
			{
				CLIENT.shutdown();
			}
		} clientCleanup;
	}  // namespace

	std::string getTempFile(const std::string_view name)
	{
		return (tempDir.get() / name).string();
	}
}  // namespace fbcpp::test
