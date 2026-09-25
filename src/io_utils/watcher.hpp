#pragma once
// API
// Struct watcher;
// init_watch(watcher);
// watchedh = watch(filepath, actions, callback)
// pull_watch_events();
// unwtach(watchedh)

#include <sys/inotify.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <map>
#include <string_view>

struct Watcher {
    int wd;
    std::function<void(void)> callback;
};

enum class WatchEvents {
    CREATE = IN_CREATE,
    MODFY = IN_MODIFY,
    DELETE = IN_DELETE,
};

static int itf_inst = -1;
static std::map<std::filesystem::path, int> path2wd;
static std::map<int, Watcher> watchers;

inline void init_watch() {
    if (itf_inst != -1) return;
    itf_inst = inotify_init1(IN_NONBLOCK);
    if (itf_inst == -1) {
        fprintf(stderr, "Filed to initialize inotify");
        exit(EXIT_FAILURE);
    }
}

template <typename F>
inline void watch(std::vector<std::filesystem::path> filepaths,
                  WatchEvents events, const F& callback) {
    for (auto& filepath : filepaths) {
        if (itf_inst == -1) {
            fprintf(stderr, "init_watch needs to be called first!");
            exit(EXIT_FAILURE);
        }
        Watcher wtch;
        int wd = inotify_add_watch(itf_inst, filepath.c_str(), (int)events);
        wtch.callback = callback;
        watchers.insert({wd, wtch});
        path2wd.insert({filepath, wd});
    }
}

inline void poll_watchers() {
    if (itf_inst == -1) {
        fprintf(stderr, "init_watch needs to be called first!");
        exit(EXIT_FAILURE);
    }
    int len;
    inotify_event events[5];
    while ((len = read(itf_inst, events, sizeof(events))) > 0) {
        for (int i = 0; i < (len / sizeof(inotify_event)); i++) {
            watchers[events[i].wd].callback();
        }
    }
}

inline void unwatch(std::string_view filepath) {
    if (itf_inst == -1) {
        fprintf(stderr, "init_watch needs to be called first!");
        exit(EXIT_FAILURE);
    }
    inotify_rm_watch(itf_inst, path2wd.at(filepath));
}
