#include "esqlite.h"

#include <gtest/gtest.h>

#include <filesystem>

using namespace esqlite;

#define ASSERT_EXPECTED(expr)                                                  \
  do {                                                                         \
    auto &&E = (expr);                                                         \
    if (!E) {                                                                  \
      std::cerr << "Error: " << E.error() << std::endl;                        \
      GTEST_FAIL();                                                            \
    }                                                                          \
  } while (false)

#define ASSERT_EXPECTED_V(expr, value)                                         \
  do {                                                                         \
    auto &&E = (expr);                                                         \
    if (!E) {                                                                  \
      std::cerr << "Error: " << E.error() << std::endl;                        \
      GTEST_FAIL();                                                            \
    }                                                                          \
    ASSERT_EQ(*E, (value));                                                    \
  } while (false)

TEST(correctness_simple, open_creates_file) {
  ASSERT_EXPECTED(open("file.sqlite"));
  ASSERT_TRUE(std::filesystem::exists("file.sqlite"));
}

TEST(correctness_simple, check_insert) {
  auto Conn = open("file.sqlite");
  ASSERT_EXPECTED(Conn);
  auto InsertStmt = Conn->prepare("CREATE TABLE IF NOT EXISTS KEK (str1 TEXT "
                                  "NOT NULL, str2 TEXT NOT NULL)");
  ASSERT_EXPECTED(InsertStmt);
  ASSERT_EXPECTED_V(InsertStmt->step(), Statement::StepOk::STEP_DONE);
  ASSERT_FALSE(Conn->run("CREATE TABLE KEK (str1 TEXT, str2 TEXT)"));
}

struct KekPod {
  std::string Str;
  int N1;
  double N2;
};

TEST(correctness_simple, read_write) {
  auto Conn = open("file.sqlite");
  ASSERT_EXPECTED(Conn);
  ASSERT_EXPECTED(Conn->run("DROP TABLE IF EXISTS KEK"));
  ASSERT_EXPECTED(Conn->run("CREATE TABLE KEK (str TEXT, n1 INT, n2 REAL)"));
  std::string_view TextSample = "Hello world!";
  ASSERT_EXPECTED(Conn->run("INSERT INTO KEK (str, n1, n2) VALUES (?, ?, ?)", TextSample, 1, 2.51));
  {
    auto Stmt = Conn->prepare("SELECT * FROM KEK");
    ASSERT_EXPECTED(Stmt);
    ASSERT_EXPECTED_V(Stmt->step(), Statement::StepOk::STEP_ROW);
    auto Pod = Stmt->readPod<KekPod>();
    ASSERT_EXPECTED(Pod);
    ASSERT_EQ(Pod->Str, TextSample);
    ASSERT_EQ(Pod->N1, 1);
    ASSERT_EQ(Pod->N2, 2.51);
  }
}

TEST(correctness_simple, read_generator_iterable) {
  auto Conn = open_v2("file.sqlite", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY);
  ASSERT_EXPECTED(Conn);
  ASSERT_EXPECTED(Conn->run("DROP TABLE IF EXISTS KEK"));
  ASSERT_EXPECTED(Conn->run("CREATE TABLE KEK (str TEXT, n1 INT, n2 REAL)"));
  std::string_view TextSample = "Hello world!";
  ASSERT_EXPECTED(Conn->run("INSERT INTO KEK (str, n1, n2) VALUES (?, ?, ?), (?, ?, ?), (?, ?, ?)", TextSample, 1, 2.51, TextSample, 2, 3.51, TextSample, 3, 4.51));

  auto ReadRange = Conn->runReading<std::string_view, int, double>("SELECT * FROM KEK");
  auto First = ReadRange.begin();
  auto End = ReadRange.end();

  ASSERT_NE(First, End);
  ASSERT_EXPECTED(*First);
  ASSERT_EQ(**First, std::tuple(TextSample, 1, 2.51));
  ++First;

  ASSERT_NE(First, End);
  ASSERT_EXPECTED(*First);
  ASSERT_EQ(**First, std::tuple(TextSample, 2, 3.51));
  ++First;

  ASSERT_NE(First, End);
  ASSERT_EXPECTED(*First);
  ASSERT_EQ(**First, std::tuple(TextSample, 3, 4.51));
  ++First;

  ASSERT_EQ(First, End);
}
