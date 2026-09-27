#include "common/engine_config.h"

#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "common/engine_config_toml.h"
#include "common/exception.h"

namespace minitub {

namespace {

// Expects FromString(toml) to throw a Config exception whose message contains `needle`.
void ExpectConfigError(const std::string &toml, const std::string &needle) {
  try {
    EngineConfig::FromString(toml);
    FAIL() << "expected an error containing: " << needle;
  } catch (const Exception &e) {
    EXPECT_EQ(e.GetType(), ExceptionType::Config);
    EXPECT_NE(std::string(e.what()).find(needle), std::string::npos) << "actual message: " << e.what();
  }
}

}  // namespace

TEST(EngineConfigTest, Defaults) {
  EngineConfig c;
  EXPECT_EQ(c.storage.db_file, "minitub.db");
  EXPECT_EQ(c.storage.log_file, "minitub.log");
  EXPECT_EQ(c.storage.io_mode, IoMode::Buffered);
  EXPECT_EQ(c.storage.disk_scheduler_workers, 1);
  EXPECT_EQ(c.buffer_pool.pool_size, 1024);
  EXPECT_EQ(c.buffer_pool.replacer, ReplacerType::LruK);
  EXPECT_EQ(c.buffer_pool.lru_k, 2);
  EXPECT_EQ(c.index.default_type, IndexType::BPlusTree);
  EXPECT_EQ(c.transaction.isolation, IsolationLevel::Snapshot);
  EXPECT_FALSE(c.wal.enabled);
  EXPECT_EQ(c.wal.flush_policy, WalFlushPolicy::GroupCommit);
  EXPECT_EQ(c.wal.group_commit_interval_us, 1000);
}

TEST(EngineConfigTest, EmptyInputGivesDefaults) { EXPECT_EQ(EngineConfig::FromString(""), EngineConfig{}); }

TEST(EngineConfigTest, DefaultTomlFileMatchesDefaults) {
  EXPECT_EQ(EngineConfig::FromFile(MINITUB_SOURCE_DIR "/configs/default.toml"), EngineConfig{});
}

TEST(EngineConfigTest, FullLoad) {
  auto c = EngineConfig::FromString(R"(
    [storage]
    db_file = "a.db"
    log_file = "a.log"
    io_mode = "direct"
    disk_scheduler_workers = 4
    [buffer_pool]
    pool_size = 64
    replacer = "clock"
    lru_k = 3
    [index]
    default_type = "extendible_hash"
    [transaction]
    isolation = "serializable"
    [wal]
    enabled = true
    flush_policy = "every_commit"
    group_commit_interval_us = 50
  )");
  EXPECT_EQ(c.storage.db_file, "a.db");
  EXPECT_EQ(c.storage.log_file, "a.log");
  EXPECT_EQ(c.storage.io_mode, IoMode::Direct);
  EXPECT_EQ(c.storage.disk_scheduler_workers, 4);
  EXPECT_EQ(c.buffer_pool.pool_size, 64);
  EXPECT_EQ(c.buffer_pool.replacer, ReplacerType::Clock);
  EXPECT_EQ(c.buffer_pool.lru_k, 3);
  EXPECT_EQ(c.index.default_type, IndexType::ExtendibleHash);
  EXPECT_EQ(c.transaction.isolation, IsolationLevel::Serializable);
  EXPECT_TRUE(c.wal.enabled);
  EXPECT_EQ(c.wal.flush_policy, WalFlushPolicy::EveryCommit);
  EXPECT_EQ(c.wal.group_commit_interval_us, 50);
}

TEST(EngineConfigTest, PartialLoadKeepsOtherDefaults) {
  auto c = EngineConfig::FromString("[buffer_pool]\nreplacer = \"lru\"\n");
  EngineConfig expected;
  expected.buffer_pool.replacer = ReplacerType::Lru;
  EXPECT_EQ(c, expected);
}

TEST(EngineConfigTest, ToTomlRoundTrips) {
  EngineConfig c;
  c.buffer_pool.replacer = ReplacerType::Clock;
  c.wal.enabled = true;
  std::ostringstream out;
  out << ToToml(c);
  EXPECT_EQ(EngineConfig::FromString(out.str()), c);
}

TEST(EngineConfigTest, RejectsUnknownSection) { ExpectConfigError("[bufferpool]\n", "unknown section 'bufferpool'"); }

TEST(EngineConfigTest, RejectsUnknownKey) {
  ExpectConfigError("[buffer_pool]\npool_sz = 8\n", "unknown key 'buffer_pool.pool_sz' (line 2)");
}

TEST(EngineConfigTest, RejectsBadEnumValue) {
  ExpectConfigError("[buffer_pool]\nreplacer = \"fifo\"\n",
                    "invalid value 'fifo' for buffer_pool.replacer; expected one of: lru, lru_k, clock");
}

TEST(EngineConfigTest, RejectsWrongType) {
  ExpectConfigError("[buffer_pool]\npool_size = \"big\"\n", "buffer_pool.pool_size must be an integer");
  ExpectConfigError("[wal]\nenabled = 1\n", "wal.enabled must be a boolean");
  ExpectConfigError("storage = 3\n", "'storage' must be a table");
}

TEST(EngineConfigTest, RejectsNonPositiveNumbers) {
  ExpectConfigError("[buffer_pool]\npool_size = 0\n", "buffer_pool.pool_size must be >= 1, got 0");
}

TEST(EngineConfigTest, RejectsSyntaxErrors) { ExpectConfigError("[storage\n", "cannot parse config"); }

TEST(EngineConfigTest, RejectsMissingFile) { EXPECT_THROW(EngineConfig::FromFile("does/not/exist.toml"), Exception); }

}  // namespace minitub
