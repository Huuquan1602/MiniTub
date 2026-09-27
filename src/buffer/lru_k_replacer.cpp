#include "buffer/lru_k_replacer.h"

#include "common/exception.h"

namespace minitub {

LRUKReplacer::LRUKReplacer(std::size_t /*num_frames*/, std::size_t /*k*/) {
  throw Exception(ExceptionType::NotImplemented, "LRUKReplacer::LRUKReplacer");
}

auto LRUKReplacer::Evict() -> std::optional<frame_id_t> {
  throw Exception(ExceptionType::NotImplemented, "LRUKReplacer::Evict");
}

void LRUKReplacer::RecordAccess(frame_id_t /*frame_id*/) {
  throw Exception(ExceptionType::NotImplemented, "LRUKReplacer::RecordAccess");
}

void LRUKReplacer::SetEvictable(frame_id_t /*frame_id*/, bool /*evictable*/) {
  throw Exception(ExceptionType::NotImplemented, "LRUKReplacer::SetEvictable");
}

void LRUKReplacer::Remove(frame_id_t /*frame_id*/) {
  throw Exception(ExceptionType::NotImplemented, "LRUKReplacer::Remove");
}

auto LRUKReplacer::Size() const -> std::size_t { throw Exception(ExceptionType::NotImplemented, "LRUKReplacer::Size"); }

}  // namespace minitub
