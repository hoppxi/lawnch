#include <sys/types.h>
#include <sys/wait.h>
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
#include <cstdlib>

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
    if (chdir("/") != 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    int log_fd = open(get_log_path().c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd >= 0) {
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
        close(log_fd);
    }
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
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (n < 0) {
            perror("read");
            continue;
        }
        buf[n] = '\0';

        if (strncmp(buf, "toggle", 6) == 0) {
            g_idle_add(toggle_idle, nullptr);
        } else if (strncmp(buf, "quit", 4) == 0) {
            g_idle_add(quit_idle, app);
        }
    }
    unlink(path.c_str());
}

static void on_css_error(GtkCssProvider *provider, GtkCssSection *section, const GError *error, gpointer data) {
    g_warning("CSS parsing error: %s", error->message);
}


static void on_activate(GtkApplication* app, gpointer) {
    if (!launcher) {
        launcher = new LauncherWindow(app);

        GtkCssProvider* provider = gtk_css_provider_new();
        g_signal_connect(provider, "parsing-error", G_CALLBACK(on_css_error), NULL);
        gtk_css_provider_load_from_path(provider, "/home/hoppxi/devspace/lawnch/build/style.css");
        gtk_style_context_add_provider_for_display(
            gdk_display_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );

        g_object_unref(provider);
        std::thread(fifo_thread, app).detach();
    }
}

bool send_command(const std::string& cmd) {
    int fd = open(get_socket_path().c_str(), O_WRONLY | O_NONBLOCK);
    if (fd < 0) return false;
    ssize_t n = write(fd, cmd.c_str(), cmd.length());
    if (n < 0) perror("write");
    close(fd);
    return true;
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -d, --daemon     Run The daemon\n"
              << "  -t, --toggle     Toggle the launcher window (auto-starts daemon if needed)\n"
              << "  -q, --quit       Kill the running daemon\n"
              << "  -r, --reload     Restart the daemon\n"
              << "  -v, --version    Show version info\n"
              << "  -h, --help       Show this help\n";
}

int main(int argc, char* argv[]) {
    bool do_daemon = false;
    bool do_toggle = false;
    bool do_quit = false;
    bool do_reload = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Lawnch version " << VERSION << std::endl;
            return 0;
        } else if (arg == "-t" || arg == "--toggle") {
            do_toggle = true;
        } else if (arg == "-q" || arg == "--quit") {
            do_quit = true;
        } else if (arg == "-r" || arg == "--reload") {
            do_reload = true;
        } else if (arg == "-d" || arg == "--daemon") {
            do_daemon = true;
        }
    }

    if (do_quit) {
        if (!send_command("quit")) {
            std::cerr << "No running daemon found." << std::endl;
        }
        return 0;
    }

    if (do_reload) {
        if (send_command("quit")) {
            usleep(200000);
        }
        daemonize();
    }

    if (do_toggle) {
        if (!send_command("toggle")) {
            std::cerr << "No running daemon found." << std::endl;
        }
        return 0;
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