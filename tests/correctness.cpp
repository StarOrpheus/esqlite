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
  ASSERT_FALSE(Conn->exec("CREATE TABLE KEK (str1 TEXT, str2 TEXT)"));
}

struct KekPod {
  std::string Str;
  int N1;
  double N2;
};

TEST(correctness_simple, read_write) {
  auto Conn = open("file.sqlite");
  ASSERT_EXPECTED(Conn);
  ASSERT_EXPECTED(Conn->exec("DROP TABLE IF EXISTS KEK"));
  ASSERT_EXPECTED(Conn->exec("CREATE TABLE KEK (str TEXT, n1 INT, n2 REAL)"));
  std::string_view TextSample = "Hello world!";
  ASSERT_EXPECTED(Conn->exec("INSERT INTO KEK (str, n1, n2) VALUES (?, ?, ?)", TextSample, 1, 2.51));
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