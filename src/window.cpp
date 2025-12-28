#include "window.hpp"
#include "utils.hpp"
#include <iostream>

struct RowData {
    SearchResult result;
};

void row_data_free(gpointer data) {
    delete static_cast<RowData*>(data);
}

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
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
    
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

    gtk_widget_remove_css_class(GTK_WIDGET(window), "background");
    gtk_widget_remove_css_class(GTK_WIDGET(window), "fullscreen");
}

void LauncherWindow::setup_ui() {
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(main_box, "main-window");
    gtk_widget_set_halign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_box, GTK_ALIGN_CENTER);
    
    GtkWidget* launcher_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(launcher_box, 500, -1);
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

    scrolled_window = gtk_scrolled_window_new();
    gtk_widget_add_css_class(scrolled_window, "scroll-window");
    gtk_widget_set_size_request(scrolled_window, -1, 400);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    results_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(results_list_box, "results-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(results_list_box), GTK_SELECTION_SINGLE);
    g_signal_connect(results_list_box, "row-activated", G_CALLBACK(on_row_activated_cb), this);

    gtk_widget_set_can_focus(results_list_box, FALSE); // Prevent list from stealing focus
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(results_list_box), TRUE);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), results_list_box);

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

void LauncherWindow::on_search_changed(GtkEditable* editable) {
    const char* text = gtk_editable_get_text(editable);
    update_results(text);
}

void LauncherWindow::update_results(const std::string& query) {
    // Clear list
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(results_list_box)) != NULL) {
        gtk_list_box_remove(GTK_LIST_BOX(results_list_box), child);
    }

    if (query.empty()) return;

    current_results = SearchEngine::get().query(query);

    for (const auto& res : current_results) {
        GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_add_css_class(row_box, "row-box");

        GtkWidget* icon;
        if (res.type == "emoji") {
             // For emoji, the "name" contains the char, but lets just use text for icon if needed
             // Or valid gtk icon if available.
             icon = gtk_label_new(res.name.substr(0, res.name.find(' ')).c_str()); 
             gtk_widget_add_css_class(icon, "emoji-icon");
        } else {
            icon = gtk_image_new_from_icon_name(res.icon.c_str());
            gtk_image_set_pixel_size(GTK_IMAGE(icon), 24);
        }
        
        // Text Box
        GtkWidget* text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_box_set_homogeneous(GTK_BOX(text_box), FALSE);
        gtk_widget_set_hexpand(text_box, TRUE);
        gtk_widget_set_halign(text_box, GTK_ALIGN_START);

        GtkWidget* title = gtk_label_new(res.name.c_str());
        gtk_widget_set_halign(title, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_LEFT);
        gtk_label_set_max_width_chars(GTK_LABEL(title), 50);
        gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
        gtk_widget_set_size_request(title, 300, -1);
        gtk_widget_add_css_class(title, "result-title");
        
        GtkWidget* comment = gtk_label_new(res.comment.c_str());
        gtk_widget_set_halign(comment, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(comment), 0.0);
        gtk_label_set_justify(GTK_LABEL(comment), GTK_JUSTIFY_LEFT);
        gtk_label_set_max_width_chars(GTK_LABEL(comment), 50);
        gtk_label_set_ellipsize(GTK_LABEL(comment), PANGO_ELLIPSIZE_END);
        gtk_widget_set_size_request(comment, 300, -1);
        gtk_widget_add_css_class(comment, "result-comment");

        gtk_box_append(GTK_BOX(text_box), title);
        gtk_box_append(GTK_BOX(text_box), comment);

        gtk_box_append(GTK_BOX(row_box), icon);
        gtk_box_append(GTK_BOX(row_box), text_box);

        gtk_list_box_append(GTK_LIST_BOX(results_list_box), row_box);
        
        GtkWidget* row_widget = gtk_widget_get_last_child(results_list_box);
        RowData* data = new RowData{res};
        g_object_set_data_full(G_OBJECT(row_widget), "res_data", data, row_data_free);
    }

    GtkListBoxRow* first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(results_list_box), 0);
    if (first_row) {
        gtk_list_box_select_row(GTK_LIST_BOX(results_list_box), first_row);
    }
}

void LauncherWindow::on_result_activated(GtkListBox* box, GtkListBoxRow* row) {
    RowData* data = static_cast<RowData*>(g_object_get_data(G_OBJECT(row), "res_data"));
    if (data) {
        if (!data->result.command.empty()) {
            Utils::exec_detached(data->result.command);
            gtk_widget_set_visible(window, FALSE);
        }
    }
}

void LauncherWindow::on_search_changed_cb(GtkEditable* editable, gpointer user_data) {
    static_cast<LauncherWindow*>(user_data)->on_search_changed(editable);
}

void LauncherWindow::on_row_activated_cb(GtkListBox* box, GtkListBoxRow* row, gpointer user_data) {
    static_cast<LauncherWindow*>(user_data)->on_result_activated(box, row);
}

gboolean LauncherWindow::on_key_press_cb(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    LauncherWindow* self = static_cast<LauncherWindow*>(user_data);
    GtkListBox* list_box = GTK_LIST_BOX(self->results_list_box);

    if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down) {
        GtkListBoxRow* selected_row = gtk_list_box_get_selected_row(list_box);
        int index = selected_row ? gtk_list_box_row_get_index(selected_row) : -1;
        
        int total_rows = 0;
        GtkWidget* child = gtk_widget_get_first_child(GTK_WIDGET(list_box));
        while (child) { total_rows++; child = gtk_widget_get_next_sibling(child); }

        if (total_rows > 0) {
            if (keyval == GDK_KEY_Down) index = (index + 1) % total_rows;
            else index = (index - 1 + total_rows) % total_rows;

            GtkListBoxRow* new_row = gtk_list_box_get_row_at_index(list_box, index);
            if (new_row) {
                gtk_list_box_select_row(list_box, new_row);
                
                GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(self->scrolled_window));
                
                graphene_point_t p = GRAPHENE_POINT_INIT(0, 0);
                graphene_point_t out;
                
                if (gtk_widget_compute_point(GTK_WIDGET(new_row), GTK_WIDGET(list_box), &p, &out)) {
                    double row_y = out.y;
                    double row_height = gtk_widget_get_height(GTK_WIDGET(new_row));
                    double current_scroll = gtk_adjustment_get_value(adj);
                    double page_size = gtk_adjustment_get_page_size(adj);

                    if (row_y < current_scroll) {
                        gtk_adjustment_set_value(adj, row_y);
                    } else if (row_y + row_height > current_scroll + page_size) {
                        gtk_adjustment_set_value(adj, row_y + row_height - page_size);
                    }
                }
            }
        }
        return TRUE; 
    }

    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        GtkListBoxRow* row = gtk_list_box_get_selected_row(list_box);
        
        if (!row) {
            row = gtk_list_box_get_row_at_index(list_box, 0);
        }

        if (row) {
            self->on_result_activated(list_box, row);
            return TRUE;
        }
    }

    if (keyval == GDK_KEY_Escape) {
        gtk_widget_set_visible(self->window, FALSE);
        return TRUE;
    }

    return FALSE; 
}

void LauncherWindow::show() {
    gtk_widget_set_visible(window, TRUE);
    gtk_window_present(GTK_WINDOW(window));
}

void LauncherWindow::hide() {
    gtk_widget_set_visible(window, FALSE);
}

void LauncherWindow::toggle() {
    if (gtk_widget_get_visible(window))
        hide();
    else
        show();
}