#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace minitub {

enum class IoMode { Buffered, Direct };
enum class ReplacerType { Lru, LruK, Clock };
enum class IndexType { BPlusTree, ExtendibleHash };
enum class IsolationLevel { Snapshot, Serializable };
enum class WalFlushPolicy { EveryCommit, GroupCommit };

// The names used in TOML files, e.g. ToString(ReplacerType::LruK) == "lru_k".
auto ToString(IoMode v) -> std::string_view;
auto ToString(ReplacerType v) -> std::string_view;
auto ToString(IndexType v) -> std::string_view;
auto ToString(IsolationLevel v) -> std::string_view;
auto ToString(WalFlushPolicy v) -> std::string_view;

/**
 * Runtime options for the whole engine (see docs/design.md section 6).
 * Design choices are options here, not #ifdefs, so one binary can compare them.
 * A default-constructed EngineConfig holds the documented defaults.
 */
struct EngineConfig {
  struct Storage {
    std::string db_file{"minitub.db"};
    std::string log_file{"minitub.log"};
    IoMode io_mode{IoMode::Buffered};
    std::int64_t disk_scheduler_workers{1};
    friend auto operator==(const Storage &, const Storage &) -> bool = default;
  };
  struct BufferPool {
    std::int64_t pool_size{1024};  // number of frames
    ReplacerType replacer{ReplacerType::LruK};
    std::int64_t lru_k{2};
    friend auto operator==(const BufferPool &, const BufferPool &) -> bool = default;
  };
  struct Index {
    IndexType default_type{IndexType::BPlusTree};
    friend auto operator==(const Index &, const Index &) -> bool = default;
  };
  struct Transaction {
    IsolationLevel isolation{IsolationLevel::Snapshot};
    friend auto operator==(const Transaction &, const Transaction &) -> bool = default;
  };
  struct Wal {
    bool enabled{false};
    WalFlushPolicy flush_policy{WalFlushPolicy::GroupCommit};
    std::int64_t group_commit_interval_us{1000};
    friend auto operator==(const Wal &, const Wal &) -> bool = default;
  };

  Storage storage;
  BufferPool buffer_pool;
  Index index;
  Transaction transaction;
  Wal wal;

  friend auto operator==(const EngineConfig &, const EngineConfig &) -> bool = default;

  // Both throw Exception(ExceptionType::Config) on syntax errors, unknown
  // sections/keys, wrong value types, bad enum values, or out-of-range numbers.
  // Missing keys keep their defaults.
  static auto FromFile(const std::filesystem::path &path) -> EngineConfig;
  static auto FromString(std::string_view toml) -> EngineConfig;
};

}  // namespace minitub
