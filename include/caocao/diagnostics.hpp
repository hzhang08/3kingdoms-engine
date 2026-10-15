#pragma once
#include "caocao/engine.hpp"
#include <fstream>

namespace caocao {
// Local native diagnostics. Storage failures must never become game failures.
// Each process owns a distinct run directory, so concurrent games cannot
// overwrite one another's log or recovery snapshot.
class Diagnostics {
 std::filesystem::path directory_;
 std::ofstream stream_;
 size_t segment_bytes_=2*1024*1024, segments_=4, bytes_=0, segment_=0;
 uint64_t sequence_=0;
 std::string checkpoint_hash_;
 Json metadata_;
 bool active_=false;
 void rotate();
 void atomic_file(const std::filesystem::path&,const std::string&);
 void prune(const std::filesystem::path&,size_t);
public:
 bool open(const std::filesystem::path& root,size_t segment_bytes=2*1024*1024,size_t segments=4,size_t retained_inactive_runs=8) noexcept;
 void write(Json event) noexcept;
 bool checkpoint(const std::string& snapshot,const std::string& reason) noexcept;
 void failure_snapshot(const std::string& snapshot) noexcept;
 void close(int exit_code) noexcept;
 bool active()const{return active_;}
 const std::filesystem::path& directory()const{return directory_;}
};
}
