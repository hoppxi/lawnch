#pragma once
#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include "search.hpp"

class LauncherWindow {
public:
    LauncherWindow(GtkApplication* app);
    void show();
    void hide();
    void toggle();

private:
    GtkWidget* window;
    GtkWidget* search_entry;
    GtkWidget* results_list_box;
    GtkWidget* scrolled_window;
    
    std::vector<SearchResult> current_results;

    void setup_ui();
    void setup_layer_shell();
    void on_search_changed(GtkEditable* editable);
    void on_result_activated(GtkListBox* box, GtkListBoxRow* row);
    void update_results(const std::string& query);
    
    static void on_search_changed_cb(GtkEditable* editable, gpointer user_data);
    static void on_row_activated_cb(GtkListBox* box, GtkListBoxRow* row, gpointer user_data);
    static gboolean on_key_press_cb(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
};