// tests2.cpp
//
// Unit tests for the in‑memory database using GoogleTest.  These tests
// mirror those in tests.cpp but use the GTest assertions.  The
// functionality covered is table creation, insertion, selection,
// updates, deletes and error handling.

#include <gtest/gtest.h>

#include "lib.hpp"

using namespace db;

TEST(DBTests, CreateInsertSelect) {
    Database db;
    auto tc = tokenize("CREATE TABLE t (name str, age int)");
    TokenStream tcs{tc};
    auto create_stmt = parse_statement(tcs);
    db.exec(*static_cast<CreateTable*>(create_stmt.get()));

    auto ti = tokenize("INSERT INTO t (name, age) VALUES (\"Ana\", 19), (\"Ben\", 25)");
    TokenStream tsi{ti};
    auto insert_stmt = parse_statement(tsi);
    db.exec(*static_cast<Insert*>(insert_stmt.get()));

    auto ts = tokenize("SELECT name, age FROM t");
    TokenStream tss{ts};
    auto select_stmt = parse_statement(tss);
    auto result = db.exec(*static_cast<Select*>(select_stmt.get()));

    ASSERT_EQ(result.rows.size(), 2u);
    ASSERT_EQ(result.headers.size(), 2u);
    EXPECT_EQ(std::get<Str>(result.rows[0][0]), "Ana");
    EXPECT_EQ(std::get<Int>(result.rows[0][1]), 19);
    EXPECT_EQ(std::get<Str>(result.rows[1][0]), "Ben");
    EXPECT_EQ(std::get<Int>(result.rows[1][1]), 25);
}

TEST(DBTests, UpdateDelete) {
    Database db;
    auto tc = tokenize("CREATE TABLE u (name str, age int)");
    TokenStream tcs{tc};
    auto create_stmt = parse_statement(tcs);
    db.exec(*static_cast<CreateTable*>(create_stmt.get()));

    auto ti = tokenize("INSERT INTO u (name, age) VALUES (\"Alice\", 30), (\"Bob\", 40), (\"Carol\", 50)");
    TokenStream tsi{ti};
    auto insert_stmt = parse_statement(tsi);
    db.exec(*static_cast<Insert*>(insert_stmt.get()));

    auto tu = tokenize("UPDATE u SET age = 35 WHERE name = \"Alice\"");
    TokenStream tsu{tu};
    auto update_stmt = parse_statement(tsu);
    auto updated = db.exec(*static_cast<Update*>(update_stmt.get()));
    ASSERT_EQ(updated, 1u);

    auto td = tokenize("DELETE FROM u WHERE name != \"Bob\"");
    TokenStream tsd{td};
    auto delete_stmt = parse_statement(tsd);
    auto deleted = db.exec(*static_cast<Delete*>(delete_stmt.get()));
    ASSERT_EQ(deleted, 2u);

    auto ts = tokenize("SELECT name, age FROM u");
    TokenStream tss{ts};
    auto select_stmt = parse_statement(tss);
    auto result = db.exec(*static_cast<Select*>(select_stmt.get()));
    ASSERT_EQ(result.rows.size(), 1u);
    EXPECT_EQ(std::get<Str>(result.rows[0][0]), "Bob");
    EXPECT_EQ(std::get<Int>(result.rows[0][1]), 40);
}

TEST(DBTests, ErrorCases) {
    Database db;
    // Duplicate table names should throw
    {
        auto tc1 = tokenize("CREATE TABLE x (id int)");
        TokenStream ts1{tc1};
        auto stmt1 = parse_statement(ts1);
        db.exec(*static_cast<CreateTable*>(stmt1.get()));
        auto tc2 = tokenize("CREATE TABLE x (id int)");
        TokenStream ts2{tc2};
        auto stmt2 = parse_statement(ts2);
        EXPECT_THROW(db.exec(*static_cast<CreateTable*>(stmt2.get())), std::exception);
    }
    // Unknown table on insert
    {
        auto ti = tokenize("INSERT INTO no_table (x) VALUES (1)");
        TokenStream ts{ti};
        auto stmt = parse_statement(ts);
        EXPECT_THROW(db.exec(*static_cast<Insert*>(stmt.get())), std::exception);
    }
    // Type mismatch on insert
    {
        auto tc = tokenize("CREATE TABLE y (a int)");
        TokenStream ts1{tc};
        auto stmt1 = parse_statement(ts1);
        db.exec(*static_cast<CreateTable*>(stmt1.get()));
        auto ti = tokenize("INSERT INTO y (a) VALUES (\"hello\")");
        TokenStream ts2{ti};
        auto stmt2 = parse_statement(ts2);
        EXPECT_THROW(db.exec(*static_cast<Insert*>(stmt2.get())), std::exception);
    }
}