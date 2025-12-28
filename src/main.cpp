#include <gtk/gtk.h>
#include "window.hpp"
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

static const char* VERSION = "1.0.0";
static LauncherWindow* launcher = nullptr;
static std::atomic<bool> running{true};

std::string get_socket_path() {
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    fs::path base = xdg_runtime ? xdg_runtime : "/tmp";
    return (base / "lawnch.sock").string();
}

std::string get_log_path() {
    const char* xdg_cache = getenv("XDG_CACHE_HOME");
    fs::path base = xdg_cache ? fs::path(getenv("HOME")) / ".cache" : xdg_cache;
    fs::path log_dir = base / "lawnch";
    fs::create_directories(log_dir);
    return (log_dir / "lawnch.log").string();
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    int log_fd = open(get_log_path().c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);
    close(STDIN_FILENO);
}

static gboolean toggle_idle(gpointer) {
    if (launcher) launcher->toggle();
    return G_SOURCE_REMOVE;
}

static gboolean quit_idle(gpointer app) {
    running = false;
    g_application_quit(G_APPLICATION(app));
    return G_SOURCE_REMOVE;
}

static void fifo_thread(GtkApplication* app) {
    std::string path = get_socket_path();
    mkfifo(path.c_str(), 0600);

    while (running) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) continue;

        char buf[64] = {0};
        read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (strncmp(buf, "toggle", 6) == 0) {
            g_idle_add(toggle_idle, nullptr);
        } else if (strncmp(buf, "quit", 4) == 0) {
            g_idle_add(quit_idle, app);
        }
    }
    unlink(path.c_str());
}

static void on_activate(GtkApplication* app, gpointer) {
    if (!launcher) {
        launcher = new LauncherWindow(app);

        GtkCssProvider* provider = gtk_css_provider_new();
        gtk_css_provider_load_from_path(provider, "/home/hoppxi/devspace/lawnch/build/style.css");
        gtk_style_context_add_provider_for_display(
            gdk_display_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );

        g_object_unref(provider);
        std::thread(fifo_thread, app).detach();
    }
}

void send_command(const std::string& cmd) {
    int fd = open(get_socket_path().c_str(), O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Lawnch daemon not running." << std::endl;
        exit(1);
    }
    write(fd, cmd.c_str(), cmd.length());
    close(fd);
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -d, --daemonize  Run as a background daemon\n"
              << "  -t, --toggle     Toggle the launcher window\n"
              << "  -q, --quit       Kill the running daemon\n"
              << "  -v, --version    Show version info\n"
              << "  -h, --help       Show this help\n";
}

int main(int argc, char* argv[]) {
    bool do_daemon = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Lawnch version " << VERSION << std::endl;
            return 0;
        } else if (arg == "-t" || arg == "--toggle") {
            send_command("toggle");
            return 0;
        } else if (arg == "-q" || arg == "--quit") {
            send_command("quit");
            return 0;
        } else if (arg == "-d" || arg == "--daemonize") {
            do_daemon = true;
        }
    }

    if (do_daemon) {
        daemonize();
    }

    GtkApplication* app = gtk_application_new("org.hoppxi.lawnch", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);
    
    int status = g_application_run(G_APPLICATION(app), 0, nullptr);
    g_object_unref(app);
    return status;
}