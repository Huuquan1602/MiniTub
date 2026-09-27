#include "storage/disk/disk_scheduler.h"

#include "common/exception.h"

namespace minitub {

DiskScheduler::DiskScheduler(DiskManager * /*disk_manager*/, int /*num_workers*/) {
  throw Exception(ExceptionType::NotImplemented, "DiskScheduler::DiskScheduler");
}

DiskScheduler::~DiskScheduler() {
  // TODO(M1): send one stop signal per worker, then join them. (Destructors must not throw.)
}

void DiskScheduler::Schedule(DiskRequest /*request*/) {
  throw Exception(ExceptionType::NotImplemented, "DiskScheduler::Schedule");
}

}  // namespace minitub
