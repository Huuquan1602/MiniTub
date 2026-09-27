#include "workload.h"

#include "workloads/dummy_workload.h"

namespace minitub::bench {

// Register new workloads here.
auto MakeWorkload(std::string_view name) -> std::unique_ptr<Workload> {
  if (name == "dummy") {
    return std::make_unique<DummyWorkload>();
  }
  return nullptr;
}

auto WorkloadNames() -> std::vector<std::string> { return {"dummy"}; }

}  // namespace minitub::bench
