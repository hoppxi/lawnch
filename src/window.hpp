#pragma once
#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include "search.hpp"
#include <vector>
#include <string>

class LauncherWindow {
public:
    LauncherWindow(GtkApplication* app);
    void show();
    void hide();
    void toggle();

private:
    GtkWidget* window;
    GtkWidget* search_entry;
    GtkWidget* results_list_view;
    GtkWidget* scrolled_window;
    GtkStringList* string_list;
    GtkSelectionModel* selection_model;
    
    std::vector<SearchResult> current_results;

    void setup_ui();
    void setup_layer_shell();
    void on_search_changed(GtkEditable* editable);
    void update_results(const std::string& query);
    void on_list_item_setup(GtkListItem* list_item);
    void on_list_item_bind(GtkListItem* list_item);
    void on_list_item_activated(guint position);
    
    static void on_search_changed_cb(GtkEditable* editable, gpointer user_data);
    static void on_list_item_setup_cb(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data);
    static void on_list_item_bind_cb(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data);
    static void on_row_clicked_cb(GtkListItem* list_item, int n_press, double x, double y, gpointer user_data);
    static void on_list_item_activated_cb(GtkListView* list_view, guint position, gpointer user_data);
    static gboolean on_key_press_cb(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
};