// API
// init_watch();
// watchedh = watch(filepath, actions, callback)
// pull_watch_events();
// unwtach(watchedh)


#include <sys/inotify.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string_view>

struct Watcher {
   int fd; 
   void(*callback)();
};

enum class WatchEvents {
    CREATE = IN_CREATE,
    MODFY = IN_MODIFY,
    DELETE = IN_DELETE,
};

static int itf_inst = -1;
static std::map<std::string_view, Watcher> watchers;

inline void init_watch() {
    itf_inst = inotify_init1(IN_NONBLOCK);
    if(itf_inst == -1) {
        fprintf(stderr, "Filed to initialize inotify");
        exit(EXIT_FAILURE);
    }
}

inline void watch(std::string_view filepath, uint32_t events, void(*callback)()) {
    if(itf_inst == -1) {
        fprintf(stderr, "init_watch needs to be called first!");
        exit(EXIT_FAILURE);
    }
    Watcher wtch;
    wtch.fd = inotify_add_watch(itf_inst, filepath.data(), events);
    wtch.callback = callback;
    watchers.insert({filepath, wtch});
}

inline void poll_watchers() {
    if(itf_inst == -1) {
        fprintf(stderr, "init_watch needs to be called first!");
        exit(EXIT_FAILURE);
    }
    int len;
    inotify_event events[5];
    while((len = read(itf_inst, events, sizeof(events))) > 0) {
        for(int i = 0; i < (len/sizeof(inotify_event)); i++) {
            printf("%s", events[i].name);
        }
    }
}

inline void unwatch(std::string_view filepath) {
    if(itf_inst == -1) {
        fprintf(stderr, "init_watch needs to be called first!");
        exit(EXIT_FAILURE);
    }
    inotify_rm_watch(itf_inst, watchers.at(filepath).fd);
}
