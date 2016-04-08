#include <iostream>
#include "view.h"
#include "manager.h"
#include "network.h"

//初期状態
MyDrawingArea::MyDrawingArea(BaseObjectType* o, const Glib::RefPtr<Gtk::Builder>& g):
	Gtk::DrawingArea(o){
	s=NULL;
	clearInput();
	input.up=0;
	input.down=0;
	input.left=0;
	input.right=0;
	input.yaw_left=0;
	input.yaw_right=0;
	input.shot=0;
	input.change_shot=0;
	input.enter=0;
	input.cancel=0;
	input.r=0;
	input.l=0;
}

MyDrawingArea::~MyDrawingArea(void){
}

void MyDrawingArea::on_realize(void){
	Gtk::DrawingArea::on_realize();

	int size, w, h;

	Cairo::RefPtr<Cairo::ImageSurface> map_chip = Cairo::ImageSurface::create_from_png("src/png/map_chip.png");
	size = 32;
	w = 4;
	h = 3;
	for(int i=0; i < w*h; i++) mapchip_png[i] = cut_image(map_chip, size, w, i);
	map_pattern = Cairo::SurfacePattern::create(mapchip_png[0]);	//背景用画像をパターンとして定義
	map_pattern->set_extend(Cairo::EXTEND_REPEAT);	//パターンの画像が連続で表示されるようにする

	Cairo::RefPtr<Cairo::ImageSurface> players = Cairo::ImageSurface::create_from_png("src/png/players.png");
	size = 128;
	w = 6;
	h = 5;
	for(int i=0; i < w*h; i++) player_png[i] = cut_image(players, size, w, i);

	Cairo::RefPtr<Cairo::ImageSurface> tama = Cairo::ImageSurface::create_from_png("src/png/tama.png");
	size = 48;
	w = 5;
	h = 1;
	for(int i=0; i < w*h; i++) tama_png[i] = cut_image(tama, size, w, i);

	Cairo::RefPtr<Cairo::ImageSurface> item = Cairo::ImageSurface::create_from_png("src/png/item.png");
	size = 48;
	w = 9;
	h = 3;
	for(int i=0; i < w*h; i++) item_png[i] = cut_image(item, size, w, i);

	Cairo::RefPtr<Cairo::ImageSurface> icon = Cairo::ImageSurface::create_from_png("src/png/icon.png");
	size = 48;
	w = 5;
	h = 1;
	for(int i=0; i < w*h; i++) icon_png[i] = cut_image(icon, size, w, i);

	Cairo::RefPtr<Cairo::ImageSurface> bakuhatu = Cairo::ImageSurface::create_from_png("src/png/bakuhatu.png");
	size = 240;
	w = 7;
	h = 1;
	for(int i=0; i < w*h; i++) bakuhatu_png[i] = cut_image(bakuhatu, size, w, i);

	prop_png[0] = Cairo::ImageSurface::create_from_png("src/png/lock.png");
	prop_png[1] = Cairo::ImageSurface::create_from_png("src/png/lock_on.png");

}
Cairo::RefPtr<Cairo::ImageSurface> MyDrawingArea::cut_image(Cairo::RefPtr<Cairo::ImageSurface> base, int size ,int w, int i){
	Cairo::RefPtr<Cairo::ImageSurface> png;
	png = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, size, size);
	Cairo::RefPtr<Cairo::Context> cc = Cairo::Context::create(png);
	int xoff = -(i % w) * size;
	int yoff = -(i / w) * size;
	cc->save();
	cc->translate(0, 0);
	cc->set_source(base, xoff, yoff);
	cc->paint();
	cc->restore();
	return png;
}

bool MyDrawingArea::on_expose_event( GdkEventExpose* e ){
	//std::cout << "Exposed" << std::endl;

	if(s==NULL)return true;

	if(s->run == 0) draw_map();			//マップを構築
	if(s->run == 0) draw_startMenu();	//ゲーム開始前
	if(s->run > 0) draw_main();			//ゲーム開始後
	if(s->run >= 2) draw_gameSet();		//ゲーム終了時

	return true;
}

void MyDrawingArea::draw_map(void){
	//マップ画像を作成（複数の描画を１つのサーフェスにまとめる）/////////////////

	map_image = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, map_w, map_h);
	Cairo::RefPtr<Cairo::Context> cc = Cairo::Context::create(map_image);

	//ベースとなる背景（チップ未配置の部分）
	cc->save();
	cc->set_source(map_pattern);
	cc->rectangle(0, 0, map_w, map_h);
	cc->fill();
	cc->restore();

	//マップチップを配置
	for(int my = 0; my < map_mh; my++){
		for(int mx = 0; mx < map_mw; mx++){
			if(s->map[my][mx].type > 0){
				for(int i=0; i < 4; i++){
					int vx = (i<2)? 0 : 1;
					int vy = (0<i and i<3)? 0 : 1;
					float xx = mx * mapchip_size + vx * mapchip_size/2 + mapchip_size/4;
					float yy = my * mapchip_size + vy * mapchip_size/2 + mapchip_size/4;
					float angle = M_PI/2 * (s->map[my][mx].chip[i] % 4);
					int type = s->map[my][mx].chip[i] / 4 + (s->map[my][mx].type-1) * 4 + 3;

					cc->save();
					cc->translate(xx, yy);
					cc->rotate(angle);
					cc->set_source(mapchip_png[type], -mapchip_size/4, -mapchip_size/4);
					cc->paint();
					cc->restore();
				}
			}
		}
	}
}
void MyDrawingArea::draw_startMenu(void){
	Cairo::RefPtr<Cairo::Context> cc = this->get_window()->create_cairo_context();

	//背景
	cc->save();
	cc->set_source_rgb(0, 0, 0);
	cc->rectangle(0.0, 0.0, this->get_width(), this->get_height());
	cc->fill();
	cc->restore();


	int step = s->players[s->id].lock;

	//機体能力選択列
	for(int i=1; i <= max_players_type; ++i){
		int pointer = s->players[s->id].type;
		cc->save();

		if(i == pointer and step == 0) cc->set_source_rgb(1, 0, 0);
		else if(i == pointer) cc->set_source_rgb(1, 1, 0);
		else cc->set_source_rgb(1, 1, 1);

		cc->set_line_width(10.0);
		cc->rectangle((window_w/(max_players_type+2))*i, window_h/2-250, 60, 60);
		cc->stroke_preserve();

		if(i == pointer) cc->set_source_rgb(0.3, 0.3, 0.3);
		else cc->set_source_rgb(0, 0, 0);

		cc->fill();
		cc->restore();

		cc->save();
		cc->set_font_size(20);
		cc->set_line_width(5.0);
		cc->set_source_rgb(1.0, 1.0, 1.0);
		cc->move_to((window_w/(max_players_type+2))*i, window_h/2-150);
		switch(i){
		case 1: cc->show_text(std::string("バランス")); break;
		case 2: cc->show_text(std::string("高速移動")); break;
		case 3: cc->show_text(std::string("高火力　")); break;
		case 4: cc->show_text(std::string("高弾速　")); break;
		case 5: cc->show_text(std::string("高速連射")); break;
		case 6: cc->show_text(std::string("高防御　")); break;
		}
		cc->restore();

		cc->save();
		cc->set_font_size(40);
		cc->set_line_width(5.0);
		cc->set_source_rgb(1.0, 1.0, 1.0);
		cc->move_to((window_w/(max_players_type+2))*i+20, window_h/2-210);
		switch(i){
		case 1: cc->show_text(std::string("1")); break;
		case 2: cc->show_text(std::string("2")); break;
		case 3: cc->show_text(std::string("3")); break;
		case 4: cc->show_text(std::string("4")); break;
		case 5: cc->show_text(std::string("5")); break;
		case 6: cc->show_text(std::string("6")); break;
		}
		cc->restore();
	}
	//初期射撃弾選択列
	for(int i=1; i <= max_tama_type; ++i){
		int pointer = s->players[s->id].shot_type;
		cc->save();

		if(i == pointer and step == 1) cc->set_source_rgb(1, 0, 0);
		else if(i == pointer and step == 2) cc->set_source_rgb(1, 1, 0);
		else cc->set_source_rgb(1, 1, 1);

		cc->set_line_width(10.0);
		cc->rectangle((window_w/7)*i, window_h/2-50, 100, 100);
		cc->stroke_preserve();

		if(i == pointer and step >= 1) cc->set_source_rgb(0.3, 0.3, 0.3);
		else cc->set_source_rgb(0, 0, 0);

		cc->fill();
		cc->restore();
		cc->save();
		cc->translate((window_w/7)*i+50, window_h/2);
		int png = i-1;
	//	if(i < 5) png = i + 1;
		//else png = 11;
		cc->set_source(icon_png[png], -24,  -24);
		cc->paint();
		cc->restore();
	}
	cc->save();
	if(step == 2 and s->players[s->id].visible) cc->set_source_rgb(1, 1, 0);
	else if(step == 2) cc->set_source_rgb(1, 0, 0);
	else cc->set_source_rgb(0.3, 0.3, 0.3);
	cc->set_line_width(10.0);
	cc->rectangle(window_w/2-100, window_h/2+150, 200, 70);
	cc->stroke_preserve();
	cc->set_source_rgb(0, 0, 0);
	cc->fill();
	cc->restore();
	cc->save();
	cc->set_font_size(30);
	cc->set_line_width(6.0);
	if(step == 2) cc->set_source_rgb(1, 1, 1);
	else cc->set_source_rgb(0.3, 0.3, 0.3);
	cc->move_to(window_w/2-70, window_h/2+200);
	cc->show_text(std::string("準備完了"));
	cc->restore();


	cc->save();
	cc->set_font_size(30);
	cc->set_line_width(6.0);
	cc->set_source_rgb(1.0, 1.0, 1.0);
	cc->move_to(window_w/2-300, window_h/2-110);
	cc->show_text(std::string("機体のタイプを選択してください"));
	cc->move_to(window_w/2-300, window_h/2+100);
	cc->show_text(std::string("初期状態の射撃タイプを選択してください"));
	cc->move_to(window_w/2-300, window_h/2+200);
	cc->show_text(std::string("(決定: w)"));
	cc->move_to(window_w/2-300, window_h/2+250);
	cc->show_text(std::string("(戻る: e)"));
	cc->restore();
}

void MyDrawingArea::draw_main(void){
	Cairo::RefPtr<Cairo::Context> cc = this->get_window()->create_cairo_context();
	static int i,j;

	//マップ画像を描画
	cc->save();
	cc->translate(s->xx[s->id], s->yy[s->id]);
	cc->set_source(map_image, 0, 0);
	cc->paint();
	cc->restore();

	//機体を描く
	for(i=0; i < max_players; ++i){
		if(s->players[i].visible){
			int png;
			//アイテム取得状態エフェクト
			if(0 < s->players[i].up_mode and s->players[i].up_mode < 3){
				cc->save();
				cc->translate(s->players[i].xx[s->id], s->players[i].yy[s->id]);
				cc->rotate(s->players[i].base_angle);
				png = s->players[i].up_mode + 6;
				cc->set_source(player_png[png], -64, -86.5);
				cc->paint();
				cc->restore();
			}
			//本体
			cc->save();
			cc->translate(s->players[i].xx[s->id], s->players[i].yy[s->id]);
			cc->rotate(s->players[i].base_angle);
			if(!s->players[i].damage and s->players[i].up_mode < 3) png = i + 3;
			else if(s->players[i].up_mode < 3) png = s->players[i].damage;
			else png = i + 9;
			cc->set_source(player_png[png], -64, -86.5);
			cc->paint();
			cc->restore();
			//砲塔
			cc->save();
			cc->translate(s->players[i].xx[s->id], s->players[i].yy[s->id]);
			cc->rotate(s->players[i].angle);
			if(!s->players[i].damage and s->players[i].up_mode < 3) png = i + 16;
			else if(s->players[i].up_mode < 3) png = s->players[i].damage + 13;
			else png = i + 22;
			cc->set_source(player_png[png], -64, -86.5);
			cc->paint();
			cc->restore();
			}
		}
	//画面外のプレイヤーへのマーカー(ステルス状態では出ない)
	for(i=0; i < max_players; ++i){
		if(s->players[i].visible and s->players[i].outside[s->id] and s->players[i].up_mode < 3){
			float xx = s->players[i].marker_xx[s->id];
			float yy = s->players[i].marker_yy[s->id];
			float angle = s->players[i].marker_angle[s->id];
			cc->save();
			cc->set_source_rgba(1,1,1,0.3);
			cc->set_line_width(10.0);
			cc->move_to(xx + cos(angle + 3*M_PI/4) * 50, yy + sin(angle + 3*M_PI/4) * 50);
			cc->line_to(xx, yy);
			cc->line_to(xx + cos(angle + M_PI/4) * 50, yy + sin(angle + M_PI/4) * 50);
			cc->stroke();
			cc->restore();
		}
	}
	// 弾を描く
	for(i=0; i < max_players; ++i){
		for (j=0; j < max_tama; ++j){
			if(s->players[i].tama[j].visible){
				cc->save();
				cc->translate(s->players[i].tama[j].xx[s->id], s->players[i].tama[j].yy[s->id]);
				cc->rotate(s->players[i].tama[j].angle);
				cc->set_source(tama_png[s->players[i].tama[j].type-1], -24, -24);
				cc->paint();
				cc->restore();
			}
		}
	}

	// アイテムを描く
	for (i=0; i < max_item; ++i) {
		if(s->item[i].visible){
			int light = (s->time/10)%2;
			int png;
			//枠
			if(s->item[i].type <= 7){
				cc->save();
				cc->translate(s->item[i].xx[s->id], s->item[i].yy[s->id]);
				cc->set_source(item_png[light], -24, -24);
				cc->paint();
				cc->restore();
				png = s->item[i].type + 1;
			}else if(s->item[i].type <= 10) png = (s->item[i].type - 8) * 2 + 9 + light;
			else png = s->item[i].state + 18;
			cc->save();
			cc->translate(s->item[i].xx[s->id], s->item[i].yy[s->id]);
			cc->set_source(item_png[png], -24, -24);
			cc->scale(0.5, 0.5);
			cc->paint();
			cc->restore();
		}
	}
	//エフェクトを描く
	for (i=0; i < max_effect; ++i) {
		if(s->effect[i].visible){
			cc->save();
			switch(s->effect[i].type){
			case 1://プレイヤーの撃破時
				cc->translate(s->effect[i].xx[s->id], s->effect[i].yy[s->id]);
				cc->set_source(bakuhatu_png[s->effect[i].count/10], -60, -60);
				cc->paint();
				break;
			case 2://爆発弾の爆発
				cc->set_source_rgba(1.0, 0.0, 0.0, 0.5);
				cc->set_line_width(s->effect[i].w);
				cc->arc(s->effect[i].xx[s->id], s->effect[i].yy[s->id], s->effect[i].r, 0, 2*M_PI);
				cc->stroke();
				break;
			case 3://ミサイルの爆発
				cc->set_source_rgba(1.0, 0.5, 0.0, 0.5);
				cc->set_line_width(s->effect[i].w);
				cc->arc(s->effect[i].xx[s->id], s->effect[i].yy[s->id], s->effect[i].r, 0, 2*M_PI);
				cc->stroke();
				break;
			case 4://ミサイルの煙
				cc->set_source_rgba(1.0, 1.0, 1.0, s->effect[i].a);
				cc->set_line_width(0);
				cc->arc(s->effect[i].xx[s->id], s->effect[i].yy[s->id], s->effect[i].r, 0, 2*M_PI);
				cc->fill();
				cc->stroke();
				break;
			case 5://アイテム取得(輪っか)
				cc->set_source_rgba(1.0, 1.0, 1.0, 0.3);
				cc->set_line_width(5.0);
				cc->arc(s->effect[i].xx[s->id], s->effect[i].yy[s->id], s->effect[i].r, 0, 2*M_PI);
				cc->stroke();
				break;
			}
			cc->restore();
		}
	}
	//機体HPバー
	for(i=0; i<max_players; ++i){
		if(s->players[i].visible){
			cc->save();
			cc->set_source_rgb(1, 1, 1);
			if(i==s->id)cc->set_line_width(4.0), cc->rectangle(49, 21, 2+max_hp*11, 18);
			else		cc->set_line_width(1.0), cc->rectangle(s->players[i].xx[s->id] - 40.5, s->players[i].yy[s->id] + 26.5, 1+max_hp*0.8, 7);
			cc->stroke_preserve();
			cc->set_source_rgb(0, 0, 0);
			cc->fill();
			cc->restore();

			cc->save();
			cc->set_source_rgb(s->players[i].hp_r, s->players[i].hp_g, s->players[i].hp_b);
			if(i==s->id){
				cc->set_line_width(16.0);
				cc->move_to(50, 30);
				cc->line_to(50+s->players[i].hp*11, 30);
			}
			else{
				cc->set_line_width(6.0);
				cc->move_to(s->players[i].xx[s->id] - 40.0, s->players[i].yy[s->id] + 30.0);
				cc->line_to(s->players[i].xx[s->id] - 40.0 + s->players[i].hp * 0.8, s->players[i].yy[s->id] + 30.0);
			}
			cc->stroke();
			cc->restore();
		}
	}

	//playerの名前を表示
	for(i=0; i<max_players; ++i){
		if(s->players[i].visible){
			cc->save();
			cc->set_font_size(15);
			cc->set_source_rgb(1.0,1.0,1.0);
			cc->move_to(s->players[i].xx[s->id] - 30.0, s->players[i].yy[s->id] + 45.0);
			switch(i){
			case 0: cc->show_text(std::string("player1"));break;
			case 1: cc->show_text(std::string("player2"));	break;
			case 2: cc->show_text(std::string("player3"));break;
			case 3: cc->show_text(std::string("player4"));break;
			}
			cc->restore();
		}
	}



	//プレイヤーのロック状態を表示
	if(s->players[s->id].lock > -5){
		static float xx,yy;
		static int xoff, yoff, n;
		if(s->players[s->id].lock < 0){
			xx = s->players[-s->players[s->id].lock-1].xx[s->id];
			yy = s->players[-s->players[s->id].lock-1].yy[s->id];
			xoff = yoff = 75;
			n = 0;
		}else{
			xx = s->players[s->players[s->id].lock].xx[s->id];
			yy = s->players[s->players[s->id].lock].yy[s->id];
			xoff = yoff = 60;
			n = 1;
			cc->save();
			cc->set_font_size(10);
			cc->set_source_rgb(1.0,0.3,0.0);
			cc->move_to(xx - 20.0,yy + 70.0);
			cc->show_text(std::string("LOCK ON"));
			cc->restore();
		}
		cc->save();
		cc->translate(xx, yy);
		cc->set_source(prop_png[n], -xoff, -yoff);
		cc->paint();
		cc->restore();
	}

	//HPと言う文字を入れる
	cc->save();
	cc->set_font_size(30);
	cc->set_source_rgb(1.0,1.0,1.0);
	cc->move_to(5, 40);
	cc->show_text(std::string("HP"));
	cc->restore();


	//リロードゲージ（自機のみ）
	cc->save();
	cc->set_source_rgb(1, 1, 1);
	cc->set_line_width(1.0), cc->rectangle(49.5, 46.5, 101, 7.0);
	cc->stroke_preserve();
	cc->set_source_rgb(0, 0, 0);
	cc->fill();
	cc->restore();

	cc->save();
	cc->set_source_rgb(s->players[s->id].reload_r,1.0,0.0);
	cc->set_line_width(6.0);
	cc->move_to(50, 50);
	cc->line_to(50 + s->players[s->id].reload_count/s->players[s->id].reload_time*100, 50);
	cc->stroke();
	cc->restore();

	//RELOADと言う文字を入れる
	cc->save();
	cc->set_font_size(10);
	cc->set_source_rgb(1.0,1.0,1.0);
	cc->move_to(5, 55);
	cc->show_text(std::string("RELOAD"));
	cc->restore();

	//現在の弾の種類を表示する
	cc->save();
	cc->set_source_rgb(1, 1, 1);
	cc->set_line_width(4.0);
	cc->rectangle(5, 70, 50, 50);
	cc->stroke_preserve();
	cc->set_source_rgb(0, 0, 0);
	cc->fill();
	cc->restore();

	cc->save();
	cc->translate(30, 95);
	int png;
	//if(s->players[s->id].shot_type < 5) png = s->players[s->id].shot_type + 1;
	//else png = 11;
	png = s->players[s->id].shot_type - 1;
	cc->set_source(icon_png[png], -24, -24);
	cc->paint();
	cc->restore();

	if(s->players[s->id].shot_type == 5){
		sprintf(buff1,"%d",s->players[s->id].missile_number);
		cc->save();
		cc->set_font_size(15);
		cc->set_line_width(2.0);
		cc->set_source_rgb(1.0,1.0,1.0);
		cc->move_to(40, 115);
		cc->show_text(buff1);
		cc->restore();
	}

}
void MyDrawingArea::draw_gameSet(void){
	Cairo::RefPtr<Cairo::Context> cc = this->get_window()->create_cairo_context();

	if(s->run == 2){
		sprintf(buff2,"WINNER %d",s->winner+1);
		cc->save();
		cc->set_font_size(80);
		cc->set_line_width(10.0);
		cc->set_source_rgb(1.0, 0.0, 1.0);
		cc->move_to(window_w/2-200, window_h/2);
		cc->show_text(std::string("GAME SET"));
		cc->set_font_size(40);
		cc->set_line_width(6.0);
		cc->set_source_rgb(1.0, 0.0, 0.0);
		cc->move_to(window_w/2-180, window_h/2+40);
		cc->show_text(buff2);
		cc->restore();
	}else if(s->run == 3){
		cc->save();
		cc->set_font_size(80);
		cc->set_line_width(10.0);
		cc->set_source_rgb(1.0, 0.0, 1.0);
		cc->move_to(window_w/2-200, window_h/2);
		cc->show_text(std::string("GAME SET"));
		cc->set_font_size(40);
		cc->set_line_width(6.0);
		cc->set_source_rgb(1.0, 0.0, 0.0);
		cc->move_to(window_w/2-180, window_h/2+40);
		cc->show_text(std::string("YOU ARE DESTROYED"));
		cc->restore();
	}

}


void MyDrawingArea::setScene(Scene *scene){
	s=scene;
}

void MyDrawingArea::update(){
	this->queue_draw();
}

void MyDrawingArea::clearInput(void){
	input.x=-1;
	input.y=-1;
	input.key=0;
	input.change_shot = 0;
	input.r=0;
	input.l=0;
	input.enter=0;
	input.cancel=0;
}

void MyDrawingArea::getInput(input_t *i){
	*i=input;
	clearInput();
}

// PressイベントとReleaseイベントの両方を見ることで
// 押し続けている状態を把握できるようにしている
bool MyDrawingArea::on_key_press_event(GdkEventKey* k){
//	std::cout << "Pressed " << k->keyval << std::endl;
	switch(k->keyval){
	case GDK_KEY_Up:
		input.up=1;
		break;
	case GDK_KEY_Down:
		input.down=1;
		break;
	case GDK_KEY_Left:
		if(s->run==0){
			if(!key_stop[0]) input.l=1;
		}
		else input.left=1;
		break;
	case GDK_KEY_Right:
		if(s->run==0){
			if(!key_stop[1]) input.r=1;
		}
		else input.right=1;
		break;
	case GDK_KEY_d:
		input.yaw_right=1;
		break;
	case GDK_KEY_a:
		input.yaw_left=1;
		break;
	case GDK_KEY_w:
		if(s->run==0){
			if(!key_stop[2]) input.enter=1;
		}
		else input.shot=1;
		break;
	case GDK_KEY_e:
		if(s->run==0){
			if(!key_stop[3]) input.cancel=1;
		}
		else input.change_shot=1;
		break;
	default:
		if(GDK_KEY_A<=k->keyval && k->keyval<=GDK_KEY_z){
			input.key=k->keyval;
		}
	}
	return true;
}
bool MyDrawingArea::on_key_release_event(GdkEventKey* k){
//	std::cout << "Released " << k->keyval << std::endl;
	switch(k->keyval){
	case GDK_KEY_Up:
		input.up=0;
		break;
	case GDK_KEY_Down:
		input.down=0;
		break;
	case GDK_KEY_Left:
		if(s->run==0) input.l=0, key_stop[0] = 0;
		else input.left=0;
		break;
	case GDK_KEY_Right:
		if(s->run==0) input.r=0, key_stop[1] = 0;
		else input.right=0;
		break;
	case GDK_KEY_d:
		input.yaw_right=0;
		break;
	case GDK_KEY_a:
		input.yaw_left=0;
		break;
	case GDK_KEY_w:
		if(s->run==0) input.enter=0, key_stop[2] = 0;
		else input.shot=0;
		break;
	case GDK_KEY_e:
		if(s->run==0) input.cancel=0;
		else input.change_shot=0;
		key_stop[3] = 0;
		break;
	default:
		if(GDK_KEY_A<=k->keyval && k->keyval<=GDK_KEY_z){
			input.key=0;
		}
	}
	return true;
}

bool MyDrawingArea::on_button_press_event (GdkEventButton* event){
//	std::cout << "Pressed " << event->x << "," << event->y << std::endl;
	input.x=event->x;
	input.y=event->y;
	return true;
}

MyImageMenuItem::MyImageMenuItem(BaseObjectType* o, const Glib::RefPtr<Gtk::Builder>& g):
	Gtk::ImageMenuItem(o){
	g->get_widget("window2", subWindow);
}

MyImageMenuItem::~MyImageMenuItem(void){
}

void MyImageMenuItem::on_activate(void){
	Gtk::ImageMenuItem::on_activate();

	Manager &mgr = Manager::get_instance();
	switch(id){
	case 0:
		if(mgr.get_state() != Manager::Run){
			mgr.set_state(Manager::Run);
			statusBar->push(Glib::ustring("Run"), statusId++);
			g_timeout_add(5000, eraseStatusbar, 0);
			switch(mgr.get_mode()){
			case Manager::Standalone:
				mgr.init_objects();
				mgr.attend_single_player();
				g_timeout_add(period, Manager::tick, NULL);
				break;
			case Manager::Server:
				mgr.init_objects();
				process_cmd(0, SCMD_START, 0, NULL);
				break;
			case Manager::Client:
				client_start();
				break;
			}
		}
		break;
	case 1:
		if(mgr.get_state() != Manager::Stop){
			mgr.set_state(Manager::Stop);
			statusBar->push(Glib::ustring("Stop"), statusId++);
			g_timeout_add(5000, eraseStatusbar, 0);
			switch(mgr.get_mode()){
			case Manager::Server:
				server_stop();
				break;
			case Manager::Client:
				client_stop();
				break;
			default:
				break;
			}
		}
		break;
	case 2:
		if(Manager::get_instance().get_state()==Manager::Stop){
			subWindow->show();
		}
		break;
	case 3:
		exit(0);
	}
}
