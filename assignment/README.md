# In-Memory Database (Simplified SQL)

This project implements a tiny in-memory database that parses a small SQL
subset and executes it against data stored entirely in memory. It is
intended as an educational exercise for parsing, data structure design and
code organization in C++. The database supports creating tables, inserting
data, deleting and updating rows, and selecting data with simple
conditions.
## Features

* **CREATE TABLE** with `int` and `str` column types.
* **INSERT INTO** accepts multiple rows at once and fills unspecified columns
  with sensible defaults (`0` for `int`, `""` for `str`).
* **DELETE FROM** with an optional `WHERE` clause removes matching rows.
* **UPDATE** sets column values on rows matching the `WHERE` clause.
* **SELECT** projects specific columns or all columns (`*`) and filters
  results with a `WHERE` clause. Results can be output as either an ASCII
  table or CSV.
* The parser treats all keywords as **case‑sensitive**; use uppercase
  (`SELECT`) and lowercase type names (`int`, `str`) as per the
  specification. Strings are delimited by double quotes; semicolons
  inside strings do not end a statement. Escaping of quotes within
  strings is not supported.
* Errors in parsing or execution throw exceptions; the CLI catches
  exceptions, prints an error message to `stderr` and exits with a
  non‑zero status.

## Building

This project uses CMake. To build on a Unix‑like system:

```bash
cd inmemdb
cmake -S . -B build
cmake --build build
```

The resulting executables are `inmemdb_cli` for interactive use and
`run_tests` for the optional unit tests (if enabled). On Windows, use
your preferred generator via CMake and an appropriate compiler.

### Dependencies

No external dependencies are required beyond a C++20 compiler and CMake
version 3.15 or newer. The test runner uses only the standard library.

## Usage

Run the CLI by piping a SQL file into it or by passing a file as an
argument. Use the `--csv` flag to output CSV instead of the default ASCII
tables.

```bash
./build/inmemdb_cli < samples/sample.sql          # ASCII output
./build/inmemdb_cli --csv < samples/sample.sql    # CSV output
```

The sample demonstrates creating a table, inserting rows, selecting,
updating and deleting. Feel free to modify `samples/sample.sql` or
pipe your own statements into the program.

## Tests (Extension 2)

As an optional extension, a simple unit test suite is provided in
`tests/test_db.cpp`. It exercises the core functionality of the database
including table creation, insertion, selection, updates, deletes, and
error handling. To build and run the tests:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

If all tests pass you should see the message “All tests passed” in the
output from the `run_tests` executable.

## Extending

The current implementation supports only equality and inequality
conditions in WHERE clauses. You can extend it to implement additional
features such as inequality operators (`<`, `>`, `<=`, `>=`) on integer
columns, logical operators (`AND`, `OR`) with proper precedence,
JOINs across tables, or persistence to disk. Tests can be expanded
accordingly.