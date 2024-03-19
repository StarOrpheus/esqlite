#ifndef ESQLITE_ESQLITE_H
#define ESQLITE_ESQLITE_H

#pragma once

#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <sqlite3.h>

#include "type_traits.h"

namespace esqlite {

template <typename ReturnT>
using ExpectedT = std::expected<ReturnT, std::string_view>;

struct Connection;

struct Statement final {
  // TODO: Create the prepared statement object using sqlite3_prepare_v2().
  // TODO: Bind values to parameters using the sqlite3_bind_*() interfaces.
  // TODO: Run the SQL by calling sqlite3_step() one or more times.
  // TODO: Reset the prepared statement using sqlite3_reset() then go back to
  // step 2. Do this zero or more times. Destroy the object using
  // sqlite3_finalize().

  constexpr Statement() noexcept = default;

  constexpr Statement(Statement const &) = delete;
  constexpr Statement &operator=(Statement const &) = delete;

  constexpr Statement(Statement &&Other) noexcept
      : Handle(std::exchange(Other.Handle, nullptr)) {}

  constexpr Statement &operator=(Statement &&Other) noexcept {
    if (this == &Other) [[unlikely]]
      return *this;
    this->~Statement();
    Handle = std::exchange(Other.Handle, nullptr);
    return *this;
  }

  ~Statement() noexcept { sqlite3_finalize(Handle); }

  auto bindNumeric(unsigned Ind, double D) noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_bind_double(Handle, Ind, D);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {};
  }

  auto bindNumeric(unsigned Ind, int D) noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_bind_int(Handle, Ind, D);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {};
  }

  auto bindNumeric(unsigned Ind, int64_t D) noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_bind_int64(Handle, Ind, D);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {};
  }

  auto bindNull(unsigned Ind) noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_bind_null(Handle, Ind);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {};
  }

  auto bindText(unsigned Ind, std::string_view S, bool IsStatic) noexcept
      -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_bind_text(Handle, Ind, S.data(), S.size(),
                              IsStatic ? SQLITE_STATIC : SQLITE_TRANSIENT);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {};
  }

  // TODO: bindText16, bindText64 clones

  auto bindBlob(unsigned Ind, std::span<uint8_t> S, bool IsStatic) noexcept
      -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_bind_blob(Handle, Ind, S.data(), S.size(),
                              IsStatic ? SQLITE_STATIC : SQLITE_TRANSIENT);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {};
  }

  template <typename T>
  auto bindParam(size_t Idx, T &&Param) noexcept -> ExpectedT<void> {
    if constexpr (is_sqlite_numeric_v<T>) {
      return bindNumeric(Idx, Param);
    } else if constexpr (is_sqlite_text<T>) {
      return bindText(Idx, std::forward<T>(Param), false);
    } else if constexpr (is_sqlite_blob<T>) {
      return bindBlob(Idx, std::forward<T>(Param), false);
    } else if constexpr (is_sqlite_null<T>) {
      return bindNull(Idx);
    }
    std::unreachable();
  }

  template <typename T, typename... Ts>
  auto bindParams(size_t FirstIdx, T &&First, Ts &&...Params) noexcept
      -> ExpectedT<void> {
    auto E = bindParam(FirstIdx, std::forward<T>(First));
    if (!E) [[unlikely]]
      return E;
    return bindParams(FirstIdx + 1, std::forward<Ts>(Params)...);
  }

  auto bindParams(size_t FirstIdx) noexcept -> ExpectedT<void> { return {}; }

  // TODO: bindBlob64

  enum class StepOk {
    STEP_ROW,
    STEP_BUSY,
    STEP_DONE,
  };

  auto step() noexcept -> ExpectedT<StepOk> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_step(Handle);
    if (E == SQLITE_OK || E == SQLITE_ROW)
      return {StepOk::STEP_ROW};
    if (E == SQLITE_ERROR) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    if (E == SQLITE_MISUSE) [[unlikely]]
      return std::unexpected(
          "SQLITE_MISUSE: routine was called inappropriately. Perhaps it was "
          "called on a prepared statement that has already been finalized or "
          "on one that had previously returned SQLITE_ERROR or SQLITE_DONE. Or "
          "it could be the case that the same database connection is being "
          "used by two or more threads at the same moment in time.");
    if (E == SQLITE_DONE)
      return {StepOk::STEP_DONE};
    if (E == SQLITE_BUSY)
      return {StepOk::STEP_BUSY};
    std::unreachable();
  }

  auto reset() noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    int E = sqlite3_reset(Handle);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {};
  }

  auto readNumeric(int Idx, int &X) noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    X = sqlite3_column_int(Handle, Idx);
    return {};
  }

  auto readNumeric(int Idx, std::int64_t &X) noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    X = sqlite3_column_int64(Handle, Idx);
    return {};
  }

  auto readNumeric(int Idx, double &X) noexcept -> ExpectedT<void> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    X = sqlite3_column_double(Handle, Idx);
    return {};
  }

  auto readText(int Idx) noexcept -> ExpectedT<std::string_view> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    auto const *Text =
        reinterpret_cast<char const *>(sqlite3_column_text(Handle, Idx));
    if (!Text)
      return {{}};

    return std::string_view(Text, sqlite3_column_bytes(Handle, Idx));
  }

  auto readBlob(int Idx) noexcept -> ExpectedT<std::span<const uint8_t>> {
    if (!Handle) [[unlikely]]
      return std::unexpected("Statement handle is null");

    auto const *Data =
        reinterpret_cast<uint8_t const *>(sqlite3_column_blob(Handle, Idx));
    if (!Data)
      return {{}};

    return std::span<const uint8_t>(Data, sqlite3_column_bytes(Handle, Idx));
  }

  template <class T>
  auto readColumn(int Idx, T &Param) noexcept -> ExpectedT<void> {
    if constexpr (is_sqlite_numeric_v<T>) {
      return readNumeric(Idx, Param);
    } else if constexpr (std::is_assignable_v<T, std::string_view>) {
      auto E = readText(Idx);
      if (!E) [[unlikely]]
        return std::unexpected(E.error());
      Param = *E;
    } else if constexpr (std::is_assignable_v<T, std::span<uint8_t const>>) {
      auto E = readBlob(Idx);
      if (!E) [[unlikely]]
        return std::unexpected(E.error());
      Param = *E;
    } else {
      std::unreachable();
    }
    return {};
  }

  template <class T, class... Ts>
  auto readColumns(int FirstIdx, T &Param, Ts &...Others) noexcept
      -> ExpectedT<void> {
    auto E = readColumn(FirstIdx, Param);
    if (!E) [[unlikely]]
      return E;
    return readColumns(FirstIdx + 1, Others...);
  }

  auto readColumns(int FirstIdx) noexcept -> ExpectedT<void> { return {}; }

  template <typename T> auto readPod() noexcept -> ExpectedT<T> {
    ExpectedT<T> Result{T()};
    auto TupledPod = asRefTuple(*Result);
    using TupleT = decltype(TupledPod);

    auto ReadFromZero = [this](auto &&...Ts) {
      return readColumns(0, std::forward<decltype(Ts)>(Ts)...);
    };

    auto E = std::apply(ReadFromZero, TupledPod);
    if (!E) [[unlikely]]
      Result = std::unexpected(E.error());
    return Result;
  }

private:
  constexpr Statement(sqlite3_stmt *Handle) noexcept : Handle(Handle) {}

private:
  friend struct Connection;

private:
  sqlite3_stmt *Handle{nullptr};
};

struct Connection final {

  constexpr Connection() noexcept = default;

  constexpr Connection(Connection const &) = delete;
  constexpr Connection &operator=(Connection const &) = delete;

  constexpr Connection(Connection &&Other) noexcept
      : RawHandle(std::exchange(Other.RawHandle, nullptr)) {}

  constexpr Connection &operator=(Connection &&Other) noexcept {
    if (this == &Other) [[unlikely]]
      return *this;
    this->~Connection();

    RawHandle = std::exchange(Other.RawHandle, nullptr);
    return *this;
  }

  ~Connection() noexcept { sqlite3_close(RawHandle); }

  auto prepare(std::string_view Sql) noexcept -> ExpectedT<Statement> {
    if (!RawHandle) [[unlikely]]
      return std::unexpected("DB handle is null");

    sqlite3_stmt *Handle = nullptr;

    int E =
        sqlite3_prepare_v2(RawHandle, Sql.data(), Sql.size(), &Handle, nullptr);
    if (E != SQLITE_OK) [[unlikely]] {
      return std::unexpected(sqlite3_errstr(E));
    }

    return {Statement(Handle)};
  }

  template <typename... Ts>
  auto exec(std::string_view Sql, Ts &&...BindParams) noexcept
      -> ExpectedT<void> {
    auto Stmt = prepare(Sql);
    if (!Stmt) [[unlikely]]
      return std::unexpected(Stmt.error());

    if (auto E = Stmt->bindParams(1, std::forward<Ts>(BindParams)...); !E) {
      return E;
    }

    if (auto E = Stmt->step(); !E) [[unlikely]] {
      return std::unexpected(Stmt.error());
    }

    return {};
  }

  auto exec(std::string_view Sql) noexcept -> ExpectedT<void> {
    auto Stmt = prepare(Sql);
    if (!Stmt) [[unlikely]]
      return std::unexpected(Stmt.error());

    if (auto E = Stmt->step(); !E) [[unlikely]] {
      return std::unexpected(Stmt.error());
    }

    return {};
  }

private:
  constexpr Connection(sqlite3 *RawHandle) noexcept : RawHandle(RawHandle) {}

  friend auto open(std::string_view Path) noexcept -> ExpectedT<Connection> {
    sqlite3 *Handle = nullptr;
    auto E = sqlite3_open(Path.data(), &Handle);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {Connection(Handle)};
  }

  friend auto open16(std::string_view Path) noexcept -> ExpectedT<Connection> {
    sqlite3 *Handle = nullptr;
    auto E = sqlite3_open16(Path.data(), &Handle);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {Connection(Handle)};
  }

  friend auto open_v2(std::string_view Path, int Flags) noexcept
      -> ExpectedT<Connection> {
    sqlite3 *Handle = nullptr;
    auto E = sqlite3_open_v2(Path.data(), &Handle, Flags, nullptr);
    if (E != SQLITE_OK) [[unlikely]]
      return std::unexpected(sqlite3_errstr(E));
    return {Connection(Handle)};
  }

private:
  sqlite3 *RawHandle{nullptr};
};

ExpectedT<Connection> open_v2(std::string_view Path, int Flags) noexcept;
ExpectedT<Connection> open16(std::string_view Path) noexcept;
ExpectedT<Connection> open(std::string_view Path) noexcept;

} // namespace esqlite

#endif // ESQLITE_ESQLITE_H
