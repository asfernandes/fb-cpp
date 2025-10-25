# fb-cpp

A modern C++ wrapper for the Firebird database API.

[Documentation](https://asfernandes.github.io/fb-cpp) | [Repository](https://github.com/asfernandes/fb-cpp)

## Overview

`fb-cpp` provides a clean, modern C++ interface to the Firebird database engine.
It wraps the Firebird C++ API with RAII principles, smart pointers, and modern C++ features.

## Features

- **Modern C++**: Uses C++20 features for type safety and performance
- **RAII**: Automatic resource management with smart pointers
- **Type Safety**: Strong typing for database operations
- **Exception Safety**: Proper error handling with exceptions
- **Boost Integration**: Optional Boost.DLL for loading fbclient and Boost.Multiprecision support for large numbers

## Quick Start

```cpp
#include "fb-cpp/fb-cpp.h"

using namespace fbcpp;

// Create a client
Client client{"fbclient"};

// Connect to a database
const auto attachmentOptions = AttachmentOptions()
    .setConnectionCharSet("UTF8");
Attachment attachment{client, "localhost:database.fdb", attachmentOptions};

// Start a transaction
const auto transactionOptions = TransactionOptions()
    .setIsolationLevel(TransactionIsolationLevel::READ_COMMITTED);
Transaction transaction{attachment, transactionOptions};

// Prepare a statement
Statement statement{attachment, transaction, "select id, name from users where id = ?"};

// Set parameters
statement.setInt32(0, 42);
/* Or
statement.set(0, 42);
*/

// Execute and get results
statement.execute(transaction);

// Process results...
do
{
    const std::optional<std::int32_t> id = statement.getInt32(0);
    const std::optional<std::string> name = statement.getString(1);

    /* Or
    const auto id = statement.get<std::int32_t>(0);
    const auto name = statement.get<std::string>(1);
    */
} while (statement.fetchNext());

// Commit transaction
transaction.commit();
```

## Building

This project uses CMake with vcpkg for dependency management.

```bash
# Clone the repository
git clone --recursive-submodules https://github.com/asfernandes/fb-cpp.git

# Configure
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build/Release

# Install
cmake --install build/Release

# Generate documentation
cmake --build build/Release --target docs
```

## Documentation

The complete API documentation is available in the build `doc/docs/` directory after building with the `docs` target.

## License

MIT License - see LICENSE.md for details.
