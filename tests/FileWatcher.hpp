#include <cassert>
#include <expected>
#include <filesystem>

#include "DependencyTree.hpp"
#include "DependencyTreeFunctions.hpp"
#include "extras/File.hpp"
#include "io_utils/watcher.hpp"

enum FileFlags : gbg::DependencyMask { FILE_M = 1 << 2 };

const gbg::ResourceType FILE_RT = 10;

class FileWatcher {
   public:
    FileWatcher(gbg::FileManager& f_m, gbg::DependencyTreeManager& dep_m)
        : _f_m(f_m), _d_m(dep_m) {}

    std::optional<gbg::File*> createFile(const std::string_view& path) {
        std::filesystem::path pt(path);
        if (not std::filesystem::exists(pt))
            return {};

        auto& f = _f_m.create(pt.relative_path());
        f.path = pt;
        auto rep = gbg::createRepresentative(_d_m, f, FILE_RT);

        watch({pt}, WatchEvents::MODFY,
              [this, rep]() { _d_m.propagateChange(rep, FileFlags::FILE_M); });

        return &f;
    }

    void poll() { poll_watchers(); }

   private:
    std::vector<gbg::FileHandle> _files;
    gbg::FileManager& _f_m;
    gbg::DependencyTreeManager& _d_m;
};
