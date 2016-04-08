#include <iostream>
#include <gtkmm.h>
#include "view.h"
#include "model.h"
#include "network.h"
#include "manager.h"

using namespace std;
#define UI_FILE "glade.ui"

static Gtk::Window *mainWindow, *subWindow;
static MyDrawingArea *drawingArea;
static Gtk::Entry *sip, *sport, *cip, *cport, *name;
static Gtk::RadioButton *standalone, *server, *client;
static Gtk::Button *ok;
static MyImageMenuItem *menu[4];
static Model *model;
input_t input[max_players];

Gtk::Statusbar *statusBar;
int statusId, statusEraseId;
Scene *scene;

void process_a_step(Scene *s, input_t *in) {
	ViewManager &view =ViewManager::get_instance();
	view.init_view_with_scene(s);
	view.update();
	view.get_input(in);
}

gboolean eraseStatusbar(void *p) {
	statusBar->pop(statusEraseId++);
	return false;
}

void subHide(void) {
//	cout << "name=" << name->get_text() << endl;
	Manager &mgr = Manager::get_instance();

	if (server->get_active()) {
		client_terminate();

		mgr.set_mode(Manager::Server);
		if (!server_setup(sport->get_text().c_str(), name->get_text().c_str(),
				input)) {
			mgr.set_mode(Manager::Standalone);
		}
		scene = new Scene;
		model->initModelWithScene(scene);
		scene->players[0].attend = 1;

	} else if (client->get_active()) {
		server_terminate();
		mgr.set_mode(Manager::Client);
		if (!client_setup(cip->get_text().c_str(), cport->get_text().c_str(),
				name->get_text().c_str())) {
			mgr.set_mode(Manager::Standalone);
		}
	} else {
		server_terminate();
		client_terminate();
		mgr.set_mode(Manager::Standalone);
	}
	subWindow->hide();
}

int main(int argc, char *argv[]) {
	Manager &mgr = Manager::get_instance();
	mgr.init_status();

	model = new Model;
	statusId = statusEraseId = 0;
	Gtk::Main kit(argc, argv);
	Glib::RefPtr<Gtk::Builder> builder;
	try {
		builder = Gtk::Builder::create_from_file(UI_FILE);
	} catch (const Glib::FileError &ex) {
		cerr << ex.what() << endl;
		return 1;
	}
	builder->get_widget("window1", mainWindow);
	builder->get_widget("window2", subWindow);
	builder->get_widget("button1", ok);
	ok->signal_clicked().connect(sigc::pointer_functor0<void>(subHide));
	builder->get_widget("sip", sip);
	builder->get_widget("sport", sport);
	builder->get_widget("cip", cip);
	builder->get_widget("cport", cport);
	builder->get_widget("name", name);
	builder->get_widget("standalone", standalone);
	builder->get_widget("server", server);
	builder->get_widget("client", client);
	builder->get_widget_derived("drawingarea1", drawingArea);
	builder->get_widget("statusbar1", statusBar);
	builder->get_widget_derived("Start", menu[0]);
	builder->get_widget_derived("Stop", menu[1]);
	builder->get_widget_derived("SetMode", menu[2]);
	builder->get_widget_derived("Quit", menu[3]);
	for (int i = 0; i < 4; ++i) {
		menu[i]->id = i;
	}

	ViewManager::get_instance().init_view(drawingArea);

	kit.run(*(mainWindow));
	return 0;
}


/*
#include <gtkmm.h>
#include "common.h"
#ifdef OPENGLMM
#include <gtkglmm.h>
#include <gdk/gdkx.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glut.h>
#endif

#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

#include <iostream>
#include <glib.h>
#include "model.h"
#include "view.h"
#include "control.h"
#include "sound.h"
#include "network.h"

scene_t scene;
input_t input;

static int start_flag = 0;
static Gtk::Window *main_win;
static Gtk::Entry *myIP, *myPort, *serverIP, *serverPort, *name;
static Gtk::RadioButton *standalone, *server, *client;
static Gtk::Window* propertydialog;
static Gtk::AboutDialog* aboutdialog;

Team2DrawingArea *drawingArea = NULL;

gboolean single_play_tick(void *);
gboolean item_tick(void *);

// 開始メニューが選択されたら呼び出される。モードに応じた開始処理を行う。
void start_activate(void) {
	if (start_flag)
		return;
	start_flag = 1;
	if (standalone->get_active()) { // 一人遊び
		scene.players[0].attend = 1;
		scene.players[1].attend = 1;
		scene.players[2].attend = 1;
		scene.players[3].attend = 1;
		strcpy(scene.players[0].name, name->get_text().c_str());
		scene.id = 0;
		model_init(&scene);
		g_timeout_add(PERIOD_S * 1000 + PERIOD_MS, single_play_tick,
				(gpointer) NULL);
		g_timeout_add(3000, item_tick, (gpointer) NULL);
	} else if (server->get_active()) { // サーバになる
		server_start();
	} else if (client->get_active()) { // クライアントとしてサーバに接続する
		client_start();
	}

	control_init(drawingArea);
}

// 一人遊びの時、タイマーイベント毎にMVCを行うのだが、
// VCはサーバモードやクライアントモードでも行うため、
// process_a_stepという関数にまとめて共通化している
gboolean single_play_tick(void *p) {
	if (start_flag) {
		scene.time++;
		ransuu();
		model_global(&scene);
		model_step(&scene, &input, 0);
		model_judge(&scene, &input, 0);

		process_a_step(&scene, &input);
	}
	return start_flag;
}

gboolean item_tick(void *p) {
	if (start_flag) {
		model_item(&scene);
	}
	return start_flag;
}

//bool hand_key_press(GdkEventKey *) {
//	control_in(&in);
//	return true;
//}

// ViewとControlを行う。
void process_a_step(scene_t *s, input_t *interf) {
	if (drawingArea != NULL)
		drawingArea->update();

#ifdef SOUND
	if (scene->players[scene->id].sound)
	sound_play(scene->players[scene->id].sound);
#endif
	control_input(interf);
	control_clear(interf);
}

// 停止メニューが選択されたら呼び出される。モードに応じた停止処理を行う。
void stop_activate(void) {
	if (!start_flag)
		return;
	start_flag = 0;
	if (standalone->get_active()) {
		model_stop(&scene);
	} else if (server->get_active()) {
		server_stop();
	} else if (client->get_active()) {
		client_stop();
	}
}

void about_hide(int i) {
	aboutdialog->hide();
}

void about_activate(void) {
	aboutdialog->show();
}

// 終了メニューが選択されたら呼び出される。
void quit_activate(void) {
	stop_activate();
	// 各モジュールの終了処理
	if (server->get_active()) {
		server_terminate();
	}
	if (client->get_active()) {
		client_terminate();
	}
#ifdef SOUND
	sound_finalize();
#endif
	main_win->hide();
}

// 通信が途絶した場合など、一人遊びモードに切り替える。
void reset_mode(void) {
	standalone->set_active();
}

// 設定メニューでOKボタンが押された時に呼び出される。
// サーバかクライアントが選択されていれば接続を開始するし、
// そうでなければ接続していたものを解除している。
void property_hide(void) {
	propertydialog->hide();
	main_win->set_title(name->get_text());
	if (server->get_active()) { // サーバになる
		if (server_setup(myPort->get_text().c_str(),
				name->get_text().c_str())) {
			show_message("サーバを開始しました。");
		} else {
			reset_mode();
			show_message("サーバを開始できませんでした。");
		}
	} else {
		if (server_terminate()) {
			show_message("サーバを終了しました。");
		}
	}
	if (client->get_active()) { // クライアントとしてサーバに接続する
		if (client_setup(serverIP->get_text().c_str(),
				serverPort->get_text().c_str(), name->get_text().c_str())) {
			show_message("サーバに接続しました。");
		} else {
			reset_mode();
			show_message("サーバに接続できませんでした。");
		}
	} else {
		if (client_terminate()) {
			show_message("サーバとの接続を終了しました。");
		}
	}
}

void property_activate(void) {
	propertydialog->show();
}

// For testing propose use the local (not installed) ui file
// #define UI_FILE PACKAGE_DATA_DIR"/samplegameplus/ui/samplegameplus.ui"
#define UI_FILE "samplegamecpp.ui"

int main(int argc, char *argv[]) {

	Gtk::Main kit(argc, argv);
#ifdef OPENGLMM
	Gtk::GL::init(argc, argv);
#endif

	//Load the Glade file and instantiate its widgets:
	Glib::RefPtr<Gtk::Builder> builder;
	try {
		builder = Gtk::Builder::create_from_file(UI_FILE);
	} catch (const Glib::FileError & ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	// ウィンドウを構成するウィジェットとの関連付け
	builder->get_widget("main_window", main_win);
	view_init(builder);

	// メニュー選択時の関数呼び出しを設定しておく
	Gtk::ImageMenuItem *menu;
	builder->get_widget("property", menu);
	menu->signal_activate().connect(
			sigc::pointer_functor0<void>(property_activate));
	builder->get_widget("execute", menu);
	menu->signal_activate().connect(
			sigc::pointer_functor0<void>(start_activate));
	builder->get_widget("stop", menu);
	menu->signal_activate().connect(
			sigc::pointer_functor0<void>(stop_activate));
	builder->get_widget("quit", menu);
	menu->signal_activate().connect(
			sigc::pointer_functor0<void>(quit_activate));
	builder->get_widget("about", menu);
	menu->signal_activate().connect(
			sigc::pointer_functor0<void>(about_activate));

	// アバウトウィンドウとの関連付け
	builder->get_widget("aboutdialog", aboutdialog);
	aboutdialog->signal_response().connect(
			sigc::pointer_functor1<int, void>(about_hide));

	// 設定ウィンドウとの関連付け
	builder->get_widget("propertydialog", propertydialog);
	Gtk::Button *button = 0;
	builder->get_widget("ok", button);
	button->signal_clicked().connect(
			sigc::pointer_functor0<void>(property_hide));
	builder->get_widget("myIP", myIP);
	myIP->set_editable(false);
	{
		char ips[16];
		int ip;
		ip = get_myip();
		sprintf(ips, "%d.%d.%d.%d", 0xff & (ip >> 24), 0xff & (ip >> 16),
				0xff & (ip >> 8), 0xff & ip);
		myIP->set_text(ips);
	}
	builder->get_widget("myPort", myPort);
	builder->get_widget("serverIP", serverIP);
	builder->get_widget("serverPort", serverPort);
	builder->get_widget("standalone", standalone);
	builder->get_widget("server", server);
	builder->get_widget("client", client);
	builder->get_widget("name", name);
	name->set_text(getenv("USER"));
	main_win->set_title(name->get_text());

	builder->get_widget_derived("drawingarea", drawingArea);
	//init_scene (&scene);
	init_input(&input);
	drawingArea->set_scene(&scene);
	drawingArea->set_input(&input);

	// 各モジュールを初期化
	model_init(&scene);
#ifdef SOUND
	sound_init();
#endif

	// メインループへ突入する
	if (main_win) {
		kit.run(*main_win);
	}

	return 0;
}
*/
