#include "window.hpp"
#include "utils.hpp"
#include <iostream>

LauncherWindow::LauncherWindow(GtkApplication* app) {
    window = gtk_application_window_new(app);
    setup_layer_shell();
    setup_ui();
}

void LauncherWindow::setup_layer_shell() {
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_namespace(GTK_WINDOW(window), "lawnch");
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), -1);
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
    
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
}

void LauncherWindow::setup_ui() {
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(main_box, "main-window");
    gtk_widget_set_halign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_box, GTK_ALIGN_CENTER);
    
    GtkWidget* launcher_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(launcher_box, 450, -1);
    gtk_widget_add_css_class(launcher_box, "launcher-container");

    GtkWidget* input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(input_box, "input-container");
    
    GtkWidget* search_icon = gtk_image_new_from_icon_name("system-search-symbolic");
    
    search_entry = gtk_text_new();
    gtk_text_set_placeholder_text(GTK_TEXT(search_entry), "type :help for flags or start typing to search");
    gtk_widget_set_hexpand(search_entry, TRUE);
    gtk_widget_add_css_class(search_entry, "search-input");
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed_cb), this);

    gtk_box_append(GTK_BOX(input_box), search_icon);
    gtk_box_append(GTK_BOX(input_box), search_entry);

    string_list = gtk_string_list_new(NULL);
    selection_model = GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(string_list)));
    gtk_single_selection_set_autoselect(GTK_SINGLE_SELECTION(selection_model), TRUE);

    GtkListItemFactory* factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(on_list_item_setup_cb), this);
    g_signal_connect(factory, "bind", G_CALLBACK(on_list_item_bind_cb), this);

    results_list_view = gtk_list_view_new(GTK_SELECTION_MODEL(selection_model), factory);
    gtk_widget_add_css_class(results_list_view, "results-list");
    g_signal_connect(results_list_view, "activate", G_CALLBACK(on_list_item_activated_cb), this);

    scrolled_window = gtk_scrolled_window_new();
    gtk_widget_add_css_class(scrolled_window, "scroll-window");
    gtk_widget_set_size_request(scrolled_window, -1, 400);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), results_list_view);

    gtk_box_append(GTK_BOX(launcher_box), input_box);
    gtk_box_append(GTK_BOX(launcher_box), scrolled_window);
    gtk_box_append(GTK_BOX(main_box), launcher_box);
    
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkEventController* key_controller = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(key_controller, GTK_PHASE_CAPTURE); 
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_press_cb), this);
    gtk_widget_add_controller(window, key_controller);

    gtk_widget_grab_focus(search_entry);
}

void LauncherWindow::on_list_item_setup(GtkListItem* list_item) {
    GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(row_box, "list-row-box");

    GtkWidget* icon = gtk_image_new();
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 24);
    
    GtkWidget* text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget* title = gtk_label_new(NULL);
    gtk_widget_add_css_class(title, "result-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
    
    GtkWidget* comment = gtk_label_new(NULL);
    gtk_widget_add_css_class(comment, "result-comment");
    gtk_label_set_xalign(GTK_LABEL(comment), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(comment), PANGO_ELLIPSIZE_END);

    gtk_box_append(GTK_BOX(text_box), title);
    gtk_box_append(GTK_BOX(text_box), comment);
    gtk_box_append(GTK_BOX(row_box), icon);
    gtk_box_append(GTK_BOX(row_box), text_box);

    gtk_list_item_set_child(list_item, row_box);

    GtkGesture* click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    g_signal_connect_swapped(click, "pressed", G_CALLBACK(on_row_clicked_cb), list_item);
    gtk_widget_add_controller(row_box, GTK_EVENT_CONTROLLER(click));
}

void LauncherWindow::on_list_item_bind(GtkListItem* list_item) {
    guint position = gtk_list_item_get_position(list_item);
    if (position >= current_results.size()) return;

    const auto& res = current_results[position];
    GtkWidget* row_box = gtk_list_item_get_child(list_item);
    GtkWidget* icon = gtk_widget_get_first_child(row_box);
    GtkWidget* text_box = gtk_widget_get_next_sibling(icon);
    GtkWidget* title = gtk_widget_get_first_child(text_box);
    GtkWidget* comment = gtk_widget_get_next_sibling(title);

    if (res.type == "emoji") {
        gtk_image_clear(GTK_IMAGE(icon)); 
    } else {
        gtk_image_set_from_icon_name(GTK_IMAGE(icon), res.icon.c_str());
    }

    gtk_label_set_text(GTK_LABEL(title), res.name.c_str());
    gtk_label_set_text(GTK_LABEL(comment), res.comment.c_str());
}

void LauncherWindow::on_row_clicked_cb(GtkListItem* list_item, int n_press, double x, double y, gpointer user_data) {
    guint pos = gtk_list_item_get_position(list_item);
    GtkWidget* list_view = gtk_widget_get_ancestor(gtk_list_item_get_child(list_item), GTK_TYPE_LIST_VIEW);
    if (list_view) {
        g_signal_emit_by_name(list_view, "activate", pos);
    }
}

void LauncherWindow::update_results(const std::string& query) {
    current_results.clear();
    gtk_string_list_splice(string_list, 0, g_list_model_get_n_items(G_LIST_MODEL(string_list)), NULL);

    if (query.empty()) return;

    current_results = SearchEngine::get().query(query);

    for (const auto& res : current_results) {
        gtk_string_list_append(string_list, res.name.c_str());
    }

    if (current_results.size() > 0) {
        gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(selection_model), 0);
    }
}

void LauncherWindow::on_list_item_activated(guint position) {
    if (position < current_results.size()) {
        const auto& res = current_results[position];
        if (!res.command.empty()) {
            Utils::exec_detached(res.command);
            hide();
        }
    }
}

void LauncherWindow::on_search_changed_cb(GtkEditable* editable, gpointer user_data) {
    static_cast<LauncherWindow*>(user_data)->on_search_changed(GTK_EDITABLE(editable));
}

void LauncherWindow::on_list_item_setup_cb(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data) {
    static_cast<LauncherWindow*>(user_data)->on_list_item_setup(list_item);
}

void LauncherWindow::on_list_item_bind_cb(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data) {
    static_cast<LauncherWindow*>(user_data)->on_list_item_bind(list_item);
}

void LauncherWindow::on_list_item_activated_cb(GtkListView* list_view, guint position, gpointer user_data) {
    static_cast<LauncherWindow*>(user_data)->on_list_item_activated(position);
}

gboolean LauncherWindow::on_key_press_cb(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    LauncherWindow* self = static_cast<LauncherWindow*>(user_data);
    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(self->selection_model));

    if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down) {
        guint selected = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(self->selection_model));
        
        if (n_items > 0) {
            if (keyval == GDK_KEY_Down) {
                selected = (selected + 1) % n_items;
            } else {
                selected = (selected == 0) ? (n_items - 1) : (selected - 1);
            }
            gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(self->selection_model), selected);
            gtk_widget_activate_action(GTK_WIDGET(self->results_list_view), "list.scroll-to-item", "u", selected);
        }
        return TRUE; 
    }

    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        guint selected = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(self->selection_model));
        if (selected != GTK_INVALID_LIST_POSITION) {
            self->on_list_item_activated(selected);
        }
        return TRUE;
    }

    if (keyval == GDK_KEY_Escape) {
        self->hide();
        return TRUE;
    }

    return FALSE; 
}

void LauncherWindow::on_search_changed(GtkEditable* editable) {
    update_results(gtk_editable_get_text(editable));
}

void LauncherWindow::show() {
    gtk_widget_set_visible(window, TRUE);
}

void LauncherWindow::hide() {
    gtk_widget_set_visible(window, FALSE);
}

void LauncherWindow::toggle() {
    if (gtk_widget_get_visible(window)) hide();
    else show();
}