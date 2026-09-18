#include <cassert>

#include "DependencyTree.hpp"
#include "DependencyTreeFunctions.hpp"
#include "extras/File.hpp"

enum FileFlags : gbg::DependencyMask { FILE_M = 1 << 2 };

const gbg::ResourceType FILE_RT = 10;

struct WatchedFile {
    WatchedFile(const gbg::fsys::path& path, gbg::FileManager* f_m,
                gbg::DependencyTreeManager* dep_m)
        : f_m(f_m), d_m(dep_m) {
        assert(f_m != nullptr and d_m != nullptr);
        h = f_m->create(path.string());
        gbg::createRepresentative(*dep_m, h, *f_m, FILE_RT);
        representative = f_m->get(h).representative;
    }
    gbg::DependencyTreeNodeHandle representative;
    gbg::FileHandle h;
    gbg::FileManager* f_m;
    gbg::DependencyTreeManager* d_m;
    void operator()() {
        assert(f_m != nullptr and d_m != nullptr);

        gbg::DependencyTreeNodeHandle dh = f_m->get(h).representative;
        d_m->propagateChange(dh, FileFlags::FILE_M);
    }
};
