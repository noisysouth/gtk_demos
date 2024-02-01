#include <stddef.h>
#include <stdbool.h>
#include <gtk/gtk.h>

#define MY_GLADE_FILE   "prog_bar_ui.glade"

GtkBuilder* gui_builder = NULL;
GtkWidget* main_window = NULL;
GtkWidget* prog_test_prog = NULL;

// Shared between worker thread and GUI thread.
pthread_mutex_t		shared_mutex;
bool				do_worker_quit = false;
// Mutex can also protect other shared data...

void
shared_mutex_init (void) {
	pthread_mutex_init (&shared_mutex, NULL);
}

void
shared_mutex_lock (void) {
	pthread_mutex_lock (&shared_mutex);
}

void
shared_mutex_unlock (void) {
	pthread_mutex_unlock (&shared_mutex);
}

void
shared_mutex_destroy (void) {
	pthread_mutex_destroy (&shared_mutex);
}

gboolean
my_worker_timeout_cb (gpointer user_data) {
	int result;
    struct timespec ts_now;
    double prog_norm;

    // ... (do work that does not access shared data)
    clock_gettime (CLOCK_MONOTONIC, &ts_now);

    prog_norm = (double)ts_now.tv_nsec / (double)(1000*1000*1000);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(prog_test_prog), prog_norm);

	shared_mutex_lock();
	if (do_worker_quit) {
		shared_mutex_unlock();
		return FALSE; // Don't keep timeout.
	} else {
		shared_mutex_unlock();
	}
	return TRUE; // Keep timeout.
}

GtkWidget *my_get_widget (char *widg_name) {
    GtkWidget *widg;

    widg = GTK_WIDGET(gtk_builder_get_object(gui_builder, widg_name));
	if (widg == NULL) {
		g_critical("Can't find widget %s in file \"%s\".", widg_name, MY_GLADE_FILE);
	}
    return widg;
}

static void
my_main_destroy(GtkWidget* widget, gpointer data)
{
	// Main window is closing. (Can do some cancellation tasks here..)

	// Exit application.
	gtk_main_quit();
}

void
my_window_show (void) {
    guint bld_result = 0;
    GError* bld_err = NULL;

    // Set up GUI Builder to access Glade XML file.
    gui_builder = gtk_builder_new();
    bld_result = gtk_builder_add_from_file(gui_builder, MY_GLADE_FILE, &bld_err);
	if (bld_result == 0) {
		if (bld_err != NULL) {
			if (bld_err->message != NULL) {
				g_critical("Can't load file \"%s\": %s", MY_GLADE_FILE, bld_err->message);
			}
			g_error_free(bld_err);
		}
	}
    gtk_builder_connect_signals(gui_builder, NULL);



    main_window = my_get_widget ("wndMain");

    // Allow the application to exit when the main window is closed.
	g_signal_connect(main_window, "destroy", G_CALLBACK(my_main_destroy), NULL);

    // Show the window.
    if (main_window != NULL) {
        gtk_window_present(GTK_WINDOW(main_window));
    }


    prog_test_prog = my_get_widget ("progressTestProgress");
}

int
main (int argc, char *argv[]) {
	// Initialize GTK windowing library.
	gtk_init (&argc, &argv);

    my_window_show();

    // Create mutex for synchronization
    shared_mutex_init ();
    // Add callback function to perform worker functions
    g_timeout_add (10 /*ms*/, my_worker_timeout_cb, NULL);

    gtk_main ();		/* Kick off the main GTK loop */
    printf ("Done gtk_main...\n");

    shared_mutex_lock ();
    do_worker_quit = true;
    shared_mutex_unlock ();

    //  Worker callback should stop.

    shared_mutex_destroy ();

    printf ("Done main().\n");
}
