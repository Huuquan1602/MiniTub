#pragma once

// Kept apart from engine_config.h so only code that needs toml++ pays for it.
#include <toml++/toml.hpp>

#include "common/engine_config.h"

namespace minitub {

/** The full config as a TOML table (same layout as configs/default.toml). */
auto ToToml(const EngineConfig &config) -> toml::table;

}  // namespace minitub
