#include "common/engine_config.h"

#include <array>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "common/engine_config_toml.h"
#include "common/exception.h"

namespace minitub {

namespace {

// Each enum's TOML spelling. One table per enum drives both parsing and printing.
template <typename E, std::size_t N>
using EnumNames = std::array<std::pair<std::string_view, E>, N>;

constexpr EnumNames<IoMode, 2> kIoModes{{{"buffered", IoMode::Buffered}, {"direct", IoMode::Direct}}};
constexpr EnumNames<ReplacerType, 3> kReplacers{
    {{"lru", ReplacerType::Lru}, {"lru_k", ReplacerType::LruK}, {"clock", ReplacerType::Clock}}};
constexpr EnumNames<IndexType, 2> kIndexTypes{
    {{"bplus_tree", IndexType::BPlusTree}, {"extendible_hash", IndexType::ExtendibleHash}}};
constexpr EnumNames<IsolationLevel, 2> kIsolationLevels{
    {{"snapshot", IsolationLevel::Snapshot}, {"serializable", IsolationLevel::Serializable}}};
constexpr EnumNames<WalFlushPolicy, 2> kFlushPolicies{
    {{"every_commit", WalFlushPolicy::EveryCommit}, {"group_commit", WalFlushPolicy::GroupCommit}}};

template <typename E, std::size_t N>
auto NameOf(const EnumNames<E, N> &names, E value) -> std::string_view {
  for (const auto &[name, v] : names) {
    if (v == value) {
      return name;
    }
  }
  return "unknown";
}

[[noreturn]] void Fail(const std::string &message) { throw Exception(ExceptionType::Config, message); }

auto Where(const toml::node &node) -> std::string {
  return " (line " + std::to_string(node.source().begin.line) + ")";
}

/** Reads the keys of one [section] and remembers which keys it asked for. */
class SectionReader {
 public:
  SectionReader(const toml::table &root, std::string_view section) : section_(section) {
    if (const toml::node *node = root.get(section)) {
      table_ = node->as_table();
      if (table_ == nullptr) {
        Fail("'" + section_ + "' must be a table" + Where(*node));
      }
    }
  }

  void Read(std::string_view key, std::string &out) {
    if (const auto *v = Get<std::string>(key, "a string")) {
      out = v->get();
    }
  }

  void Read(std::string_view key, bool &out) {
    if (const auto *v = Get<bool>(key, "a boolean")) {
      out = v->get();
    }
  }

  // Integers in the config are counts or durations, so they must be >= 1.
  void Read(std::string_view key, std::int64_t &out) {
    if (const auto *v = Get<std::int64_t>(key, "an integer")) {
      if (v->get() < 1) {
        Fail(Path(key) + " must be >= 1, got " + std::to_string(v->get()) + Where(*v));
      }
      out = v->get();
    }
  }

  template <typename E, std::size_t N>
  void ReadEnum(std::string_view key, const EnumNames<E, N> &names, E &out) {
    const auto *v = Get<std::string>(key, "a string");
    if (v == nullptr) {
      return;
    }
    std::string expected;
    for (const auto &[name, value] : names) {
      if (name == v->get()) {
        out = value;
        return;
      }
      expected += (expected.empty() ? "" : ", ") + std::string(name);
    }
    Fail("invalid value '" + v->get() + "' for " + Path(key) + "; expected one of: " + expected + Where(*v));
  }

  // Call after all Read()s: any key we did not ask for is a typo or unsupported.
  void RejectUnknownKeys() const {
    if (table_ == nullptr) {
      return;
    }
    for (const auto &[key, node] : *table_) {
      bool known = false;
      for (const auto &k : known_keys_) {
        known = known || k == key.str();
      }
      if (!known) {
        Fail("unknown key '" + Path(key.str()) + "'" + Where(node));
      }
    }
  }

 private:
  // Returns nullptr if the key is absent; fails if it has the wrong type.
  template <typename T>
  auto Get(std::string_view key, std::string_view type_name) -> const toml::value<T> * {
    known_keys_.emplace_back(key);
    const toml::node *node = table_ == nullptr ? nullptr : table_->get(key);
    if (node == nullptr) {
      return nullptr;
    }
    const auto *value = node->as<T>();
    if (value == nullptr) {
      Fail(Path(key) + " must be " + std::string(type_name) + Where(*node));
    }
    return value;
  }

  auto Path(std::string_view key) const -> std::string { return section_ + "." + std::string(key); }

  std::string section_;
  const toml::table *table_{nullptr};
  std::vector<std::string> known_keys_;
};

auto FromTable(const toml::table &root) -> EngineConfig {
  static constexpr std::array<std::string_view, 5> kSections{"storage", "buffer_pool", "index", "transaction",
                                                              "wal"};
  for (const auto &[key, node] : root) {
    bool known = false;
    for (const auto section : kSections) {
      known = known || section == key.str();
    }
    if (!known) {
      Fail("unknown section '" + std::string(key.str()) + "'" + Where(node));
    }
  }

  EngineConfig c;

  SectionReader storage(root, "storage");
  storage.Read("db_file", c.storage.db_file);
  storage.Read("log_file", c.storage.log_file);
  storage.ReadEnum("io_mode", kIoModes, c.storage.io_mode);
  storage.Read("disk_scheduler_workers", c.storage.disk_scheduler_workers);
  storage.RejectUnknownKeys();

  SectionReader buffer_pool(root, "buffer_pool");
  buffer_pool.Read("pool_size", c.buffer_pool.pool_size);
  buffer_pool.ReadEnum("replacer", kReplacers, c.buffer_pool.replacer);
  buffer_pool.Read("lru_k", c.buffer_pool.lru_k);
  buffer_pool.RejectUnknownKeys();

  SectionReader index(root, "index");
  index.ReadEnum("default_type", kIndexTypes, c.index.default_type);
  index.RejectUnknownKeys();

  SectionReader transaction(root, "transaction");
  transaction.ReadEnum("isolation", kIsolationLevels, c.transaction.isolation);
  transaction.RejectUnknownKeys();

  SectionReader wal(root, "wal");
  wal.Read("enabled", c.wal.enabled);
  wal.ReadEnum("flush_policy", kFlushPolicies, c.wal.flush_policy);
  wal.Read("group_commit_interval_us", c.wal.group_commit_interval_us);
  wal.RejectUnknownKeys();

  return c;
}

auto ParseError(const toml::parse_error &e, const std::string &source) -> std::string {
  std::ostringstream out;
  out << "cannot parse " << source << ": " << e.description() << " (line " << e.source().begin.line << ")";
  return out.str();
}

}  // namespace

auto ToString(IoMode v) -> std::string_view { return NameOf(kIoModes, v); }
auto ToString(ReplacerType v) -> std::string_view { return NameOf(kReplacers, v); }
auto ToString(IndexType v) -> std::string_view { return NameOf(kIndexTypes, v); }
auto ToString(IsolationLevel v) -> std::string_view { return NameOf(kIsolationLevels, v); }
auto ToString(WalFlushPolicy v) -> std::string_view { return NameOf(kFlushPolicies, v); }

auto EngineConfig::FromString(std::string_view toml) -> EngineConfig {
  try {
    return FromTable(toml::parse(toml));
  } catch (const toml::parse_error &e) {
    Fail(ParseError(e, "config"));
  }
}

auto EngineConfig::FromFile(const std::filesystem::path &path) -> EngineConfig {
  try {
    return FromTable(toml::parse_file(path.string()));
  } catch (const toml::parse_error &e) {
    Fail(ParseError(e, "'" + path.string() + "'"));
  }
}

auto ToToml(const EngineConfig &c) -> toml::table {
  auto str = [](std::string_view s) { return std::string(s); };
  return toml::table{
      {"storage", toml::table{{"db_file", c.storage.db_file},
                              {"log_file", c.storage.log_file},
                              {"io_mode", str(ToString(c.storage.io_mode))},
                              {"disk_scheduler_workers", c.storage.disk_scheduler_workers}}},
      {"buffer_pool", toml::table{{"pool_size", c.buffer_pool.pool_size},
                                  {"replacer", str(ToString(c.buffer_pool.replacer))},
                                  {"lru_k", c.buffer_pool.lru_k}}},
      {"index", toml::table{{"default_type", str(ToString(c.index.default_type))}}},
      {"transaction", toml::table{{"isolation", str(ToString(c.transaction.isolation))}}},
      {"wal", toml::table{{"enabled", c.wal.enabled},
                          {"flush_policy", str(ToString(c.wal.flush_policy))},
                          {"group_commit_interval_us", c.wal.group_commit_interval_us}}},
  };
}

}  // namespace minitub
