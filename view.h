#include <gtkmm.h>
#include "common.h"

#ifndef VIEW_H_
#define VIEW_H_

class ViewManager;

class MyDrawingArea: public Gtk::DrawingArea {
	friend class ViewManager;
public:
	MyDrawingArea(BaseObjectType*, const Glib::RefPtr<Gtk::Builder>&);
	virtual ~MyDrawingArea();

protected:
	virtual void on_realize();
	virtual bool on_key_press_event(GdkEventKey*);
	virtual bool on_key_release_event(GdkEventKey*);
	virtual bool on_expose_event(GdkEventExpose*);
	virtual bool on_button_press_event(GdkEventButton*);

private:
	input_t input;
	Scene *s;
	int key_stop[4]; //とりあえずキー押しっぱなしでの連続入力防止用
	void clearInput(void);
	void setScene(Scene *);
	void getInput(input_t *);
	void update();
	void draw_map();
	void draw_startMenu();
	void draw_main();
	void draw_gameSet();
	Cairo::RefPtr<Cairo::ImageSurface> cut_image(Cairo::RefPtr<Cairo::ImageSurface>, int, int, int);
	Cairo::RefPtr<Cairo::ImageSurface> mapchip_png[12];
	Cairo::RefPtr<Cairo::ImageSurface> player_png[30];
	Cairo::RefPtr<Cairo::ImageSurface> tama_png[5];
	Cairo::RefPtr<Cairo::ImageSurface> item_png[27];
	Cairo::RefPtr<Cairo::ImageSurface> icon_png[5];
	Cairo::RefPtr<Cairo::ImageSurface> prop_png[2];
	Cairo::RefPtr<Cairo::ImageSurface> bakuhatu_png[7];
	Cairo::RefPtr<Cairo::SurfacePattern> map_pattern;
	Cairo::RefPtr<Cairo::ImageSurface> map_image;
	char buff1[10], buff2[10];	//変数の数値を文字として表示するためのもの
};

class MyImageMenuItem: public Gtk::ImageMenuItem {
public:
	MyImageMenuItem(BaseObjectType*, const Glib::RefPtr<Gtk::Builder>&);
	virtual ~MyImageMenuItem();
	int id;
protected:
	virtual void on_activate();

private:
	Gtk::Window *subWindow;
};

class ViewManager {
public:
	static ViewManager& get_instance() {
		static ViewManager instance;
		return instance;
	}

	void init_view(MyDrawingArea *area) {
		drawingArea = area;
	}

	void init_view_with_scene(Scene *scene) {
		drawingArea->setScene(scene);
	}

	void update() {
		drawingArea->update();
	}

	void get_input(input_t *i) {
		drawingArea->getInput(i);
	}

private:
	ViewManager(){}
	ViewManager(ViewManager&);
	void operator =(ViewManager&);

	MyDrawingArea *drawingArea;
};

#endif /* VIEW_H_ */


/*
#ifndef VIEW_H_
#define VIEW_H_

#include <gtkmm.h>

struct dot{
	int x, y, visible;
};
const int max_dots=5;

class Team2DrawingArea: public Gtk::DrawingArea {
public:
	Team2DrawingArea(BaseObjectType*, const Glib::RefPtr<Gtk::Builder>&);
	virtual ~Team2DrawingArea();
	void update();
	void set_scene(scene_t *s);
	void set_input(input_t *input);

protected:
	virtual void on_realize();
	virtual bool on_key_press_event(GdkEventKey*);
	virtual bool on_expose_event(GdkEventExpose*);
	virtual bool on_button_press_event(GdkEventButton*);

private:
	Cairo::RefPtr<Cairo::ImageSurface> png[10];
	input_t *input;
	scene_t *s;
};

void view_init(Glib::RefPtr<Gtk::Builder> builder);
void show_message(const char *);
*/
//#endif /* VIEW_H_ */
