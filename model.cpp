/*
 * model.cpp
 *
 *  Created on: 2013/12/09
 *      Author: yakoh
 */
//192.168.0.202
#include <iostream>
#include "model.h"


static int i,j,k;

static float distance2(float,float);

Model::Model() {
	// TODO Auto-generated constructor stub
}

Model::~Model() {
	// TODO Auto-generated destructor stub
}

int Model::single_judgeState(){
	if(s->run == 0 and s->players[0].visible) {
		s->run = 1;
		for(i=1; i<max_players; i++) s->players[i].visible = 1;
	}
	else if(s->run == 1){
		if(!s->players[1].visible and !s->players[2].visible and !s->players[3].visible) s->run = 2;
		else if(!s->players[0].visible) s->run = 3;
	}


	return s->run;
}

int Model::judgeState(){
	if(s->run == 0){
		for(i=0; i<max_players; i++){
			if(s->players[i].attend and !s->players[i].visible) break;
			if(i == max_players-1) s->run = 1;
		}
	}if(s->run == 1){
		s->winner=max_players;
		for(i=0; i<max_players; i++){
			if(s->players[i].attend and s->players[i].visible and i<s->winner) s->winner = i;
			if(s->players[i].attend and s->players[i].visible and i>s->winner) {s->winner = max_players; break;}
		}
		if(s->winner < max_players) s->run = 2;
	}
	return s->run;
}

void Model::initModelWithScene(Scene *scene){
//	std::cout << "Init" << std::endl;
	//scene=s;
	s=scene;
	s->run = 0;

	create_Map(); //マップを作成

	//プレイヤーの初期化
	for(i = 0; i < max_players; ++i) {
		s->players[i].attend = 0;
		s->players[i].visible = 0;
		s->players[i].hp = 100;
		s->players[i].base_angle = 0.0;
		s->players[i].angle = 0.0;
		s->players[i].type = 1;
		s->players[i].shot_type = 1;
		s->players[i].up_mode = 0;
		s->players[i].reload_time = 30;
		s->players[i].reload_count = s->players[i].reload_time;
		s->players[i].lock = 0;
		players[i].destroyed = 0;
		players[i].reload = 0;
		players[i].rl = 1;
		players[i].move_speed   = players_para[1].move_speed;
		players[i].rotate_speed = players_para[1].rotate_speed;
		players[i].shot_power   = players_para[1].shot_power;
		players[i].shot_speed   = players_para[1].shot_speed;
		players[i].reload_speed = players_para[1].reload_speed;
		players[i].armor 		= players_para[1].armor;
		for (j = 1; j <= max_players; ++j) s->players[i].outside[j] = 0;
		for (j = 1; j <= max_tama_type; ++j) players[i].having_shot_type[j] = 0;
		//for (j = 1; j <= 4; ++j) players[i].level[j] = 1;
		for (j = 0; j < max_tama; ++j) s->players[i].tama[j].visible = 0;

		static int mx, my;
		do{
			mx = g_random_int_range(1, map_mw-1);
			my = g_random_int_range(1, map_mh-1);
		}while(s->map[my][mx].type);
		players[i].x = (mx+0.5) * mapchip_size;
		players[i].y = (my+0.5) * mapchip_size;

	}

	for (i = 0; i < max_item; ++i) 	s->item[i].visible = 0;

	s->time = 0;
}

void Model::startMenu(int id, input_t *in){
	//スタート画面での操作

	//決定
	if(s->players[id].lock == 0){
		if(in->r and s->players[id].type < max_players_type) s->players[id].type++;
		if(in->l and s->players[id].type > 1) s->players[id].type--;
		if(in->enter) s->players[id].lock++;
	}else if(s->players[id].lock == 1){
		if(in->r and s->players[id].shot_type < max_tama_type) s->players[id].shot_type++;
		if(in->l and s->players[id].shot_type > 1) s->players[id].shot_type--;
		if(in->enter) s->players[id].lock++;
		if(in->cancel) s->players[id].lock--;
	}else{
		if(in->enter){

			//選択したタイプから機体の能力値、初期射撃弾を決定
			players[id].move_speed   = players_para[s->players[id].type].move_speed;
			players[id].rotate_speed = players_para[s->players[id].type].rotate_speed;
			players[id].shot_power   = players_para[s->players[id].type].shot_power;
			players[id].shot_speed   = players_para[s->players[id].type].shot_speed;
			players[id].reload_speed = players_para[s->players[id].type].reload_speed;
			players[id].armor  		 = players_para[s->players[id].type].armor;
			players[id].having_shot_type[s->players[id].shot_type] = 1;
			if(s->players[id].shot_type == 5) s->players[id].missile_number = 30;
			s->players[id].visible = 1;
		}if(in->cancel){
			//s->players[id].outside[s->players[id].lock-1] = 0;
			//s->players[id].shot_type = 1;
			s->players[id].lock--;
			if(s->players[id].missile_number > 0) s->players[id].missile_number = 0;
			s->players[id].visible = 0;
		}
	}
}

void Model::preAction(void){
	s->time++;

	//弾の時間消滅
	for(i=0; i < max_players; ++i){
		for(j=0; j < max_tama; ++j){
			if(s->players[i].tama[j].visible){
				s->players[i].tama[j].count++;
				if(s->players[i].tama[j].count >= tama[s->players[i].tama[j].type].visible_time){
					s->players[i].tama[j].visible = 0;
					if(s->players[i].tama[j].type == 4) set_effect(players[i].tama[j].x, players[i].tama[j].y, 2, i);
					if(s->players[i].tama[j].type == 5) set_effect(players[i].tama[j].x, players[i].tama[j].y, 3, i);
				}
			}
		}
	}
	//アイテムの変化、時間消滅
	for(i=0; i < max_item; ++i){
		if(s->item[i].visible){
			item[i].count++;
			if(s->item[i].type <= 4 and item[i].count % 30 == 0) {
				if(s->item[i].type == 4) s->item[i].type = 1;
				else s->item[i].type++;
			}else if(s->item[i].type == 11){
				if(item[i].count > 200){
					s->item[i].visible = 0;
					set_effect(item[i].x, item[i].y, 2, -1);
				}else if(item[i].count > 197) s->item[i].state = 5;
				else if(item[i].count > 194) s->item[i].state = 4;
				else if(item[i].count > 191) s->item[i].state = 3;
				else if(item[i].count > 10){
					if(item[i].count % 10 < 5) s->item[i].state = 1;
					else s->item[i].state = 2;
				}
			}
			if(item[i].count >= 500) s->item[i].visible = 0;
		}
	}

	//リロード、リロードゲージの処理
	for(i=0; i< max_players; ++i){
		if(players[i].reload) {
			s->players[i].reload_count++;
			if(s->players[i].reload_count >= s->players[i].reload_time) players[i].reload = 0;
			s->players[i].reload_r = 1.0;//,s->players[i].reload_g = 1.0,s->players[i].reload_b = 0.0;
		}
		else s->players[i].reload_r = 0.0;//,s->players[i].reload_g = 1.0,s->players[i].reload_b = 0.0;
	}

	//プレイヤーのアイテム強化時間の処理
	for(i=0; i< max_players; ++i){
		if(s->players[i].up_mode > 0) {
			players[i].up_count++;
			if(players[i].up_count >= 500){
				s->players[i].up_mode = 0;
				players[i].move_speed  = players_para[s->players[i].type].move_speed;
				players[i].shot_power  = players_para[s->players[i].type].shot_power;
			}
		}
	}

	//時間経過によるアイテムの出現
	if((s->time+1)%500 == 0 or (s->time+1)%100 == 0){
		for (i=0; i < max_item; ++i) {
			if(!s->item[i].visible){
				s->item[i].visible = 1;
				item[i].count = 0;
				s->item[i].state = 0;
				if((s->time+1)%500 == 0) s->item[i].type = g_random_int_range(1, 5);
				else s->item[i].type = g_random_int_range(5, 12);

				int mx,my;
				do{
					mx = g_random_int_range(1, map_mw-1);
					my = g_random_int_range(1, map_mh-1);
				}while(s->map[my][mx].type);
				item[i].x = (mx+0.5) * mapchip_size;
				item[i].y = (my+0.5) * mapchip_size;

				break;
				}
		}
	}
	//時間経過によるエフェクトの変化、消滅
	for(i=0; i < max_effect; ++i){
		if(s->effect[i].visible){
			s->effect[i].count++;
			switch(s->effect[i].type){
			case 1:
				if(s->effect[i].count >= 69) s->effect[i].visible = 0;
				break;
			case 2:
				if(s->effect[i].count <= 10) s->effect[i].r += 5.0;
				if(s->effect[i].count >= 15) s->effect[i].w -= 10.0, s->effect[i].r += 5.0;
				if(s->effect[i].count >= 25) s->effect[i].visible = 0;
				effect[i].cr =  s->effect[i].r +  s->effect[i].w/2;
				break;
			case 3:
				if(s->effect[i].count <= 10) s->effect[i].r += 2.0;
				if(s->effect[i].count >= 15) s->effect[i].w -= 4.0, s->effect[i].r += 2.0;
				if(s->effect[i].count >= 25) s->effect[i].visible = 0;
				effect[i].cr =  s->effect[i].r +  s->effect[i].w/2;
				break;
			case 4:
				s->effect[i].r -= 0.5;
				s->effect[i].a -= 0.1;
				if(s->effect[i].count >= 10) s->effect[i].visible = 0;
				break;
			case 5:
				effect[i].x = players[effect[i].target].x;
				effect[i].y = players[effect[i].target].y;
				if(s->effect[i].count <= 10) s->effect[i].r += 4.0;
				if(s->effect[i].count >  10) s->effect[i].r += 0.4;
				if(s->effect[i].count >= 20) s->effect[i].visible = 0;
				break;
			}
		}
	}
	//プレイヤーのダメージ表現の時間
	for(i=0; i < max_players; ++i){
		if(s->players[i].damage > 0)
			players[i].damage_count++;
			if(s->players[i].damage == 1 and players[i].damage_count >= 5){
				s->players[i].damage = 0;
				players[i].move_speed = players_para[s->players[i].type].move_speed;
			}else if(s->players[i].damage == 2 and players[i].damage_count >= 10){
				s->players[i].damage = 0;
				players[i].move_speed = players_para[s->players[i].type].move_speed;
			}
	}

}

void Model::postAction(void){

	// プレイヤー当たり判定用の変数を計算
	for(i=0; i < max_players; ++i) {
		players[i].cx_off = cos(s->players[i].base_angle + M_PI/2) * players_y_off;
		players[i].cy_off = sin(s->players[i].base_angle + M_PI/2) * players_y_off;
		players[i].cx = players[i].x + players[i].cx_off;
		players[i].cy = players[i].y + players[i].cy_off;
	}

	//プレイヤーと弾の当たり判定
	for(i=0; i < max_players; ++i) {
			for(j=0; j < max_players; ++j) {
				for (k=0; k < max_tama; ++k) {
					if(distance2(players[i].cx - players[j].tama[k].x, players[i].cy - players[j].tama[k].y)
							< (players_cr + tama[s->players[j].tama[k].type].r) * (players_cr + tama[s->players[j].tama[k].type].r)
							and s->players[i].visible
							and s->players[j].tama[k].visible
							and i!=j)
					{
						if(s->players[j].tama[k].type < 4){
							s->players[i].hp -= tama[s->players[j].tama[k].type].power * players[j].shot_power / players[i].armor;
							s->players[i].damage = 1;
							players[i].damage_count = 0;
							players[i].move_speed *= (100 - tama[s->players[j].tama[k].type].power)/ (3 * 100);
						}else if(s->players[j].tama[k].type == 4) set_effect(players[j].tama[k].x, players[j].tama[k].y, 2, j);
						else if(s->players[j].tama[k].type == 5) set_effect(players[j].tama[k].x, players[j].tama[k].y, 3, j);
						s->players[j].tama[k].visible = 0;
					}
				}
			}
	}
	//プレイヤーと爆発エフェクトの当たり判定
	for(i=0; i < max_players; ++i) {
			for(j=0; j < max_effect; ++j) {
				if((distance2(players[i].cx - effect[j].x, players[i].cy - effect[j].y)
						< (players_cr + effect[j].cr) * (players_cr + effect[j].cr)
					and s->players[i].visible
					and s->effect[j].visible
					and effect[j].damage_flag[i])
					and (s->effect[j].type == 2 or s->effect[j].type == 3))
				{
					if(s->effect[j].type == 2 and effect[j].target < 0) s->players[i].hp -= tama[4].power / players[i].armor; //爆弾ダメージ
					else if(s->effect[j].type == 2) s->players[i].hp -= tama[4].power * players[effect[j].target].shot_power / players[i].armor;//爆発弾ダメージ
					else if(s->effect[j].type == 3) s->players[i].hp -= tama[5].power * players[effect[j].target].shot_power / players[i].armor;//ミサイルダメージ
					s->players[i].damage = 2;
					players[i].damage_count = 0;
					effect[j].damage_flag[i] = 0;
					players[i].move_speed *= (100 - tama[s->effect[j].type+2].power)/ (3 * 100);
					players[i].vx += 0.25 * tama[s->effect[j].type+2].power / players[i].armor * cos(atan2(players[i].y - effect[j].y, players[i].x - effect[j].x));
					players[i].vy += 0.25 * tama[s->effect[j].type+2].power / players[i].armor * sin(atan2(players[i].y - effect[j].y, players[i].x - effect[j].x));
				}
			}
	}


	//アイテムの接触判定
	for(i=0; i < max_players; ++i) {
		for (j=0; j < max_item; ++j) {
			if(distance2(players[i].cx - item[j].x, players[i].cy - item[j].y)
					< (players_cr + item_cr) * (players_cr + item_cr)
					and s->players[i].visible
					and s->item[j].visible)
				{
				if(s->item[j].type < 11){
					s->item[j].visible = 0;
					set_effect(players[i].x, players[i].y, 5, i);
					if(s->item[j].type <= 4) players[i].having_shot_type[s->item[j].type] = 1;
					else if(s->item[j].type <= 7){
						s->players[i].up_mode = s->item[j].type - 4;
						 players[i].up_count = 0;
						if(s->item[j].type == 5) players[i].move_speed *= 1.2;
						else if(s->item[j].type == 6) players[i].shot_power *= 1.2;
					}else if(s->item[j].type <= 9){
						players[i].move_speed *= 1.2;
						players[i].having_shot_type[5] = 1;
						s->players[i].missile_number += (s->item[j].type - 7) * 5;
						if(s->players[i].missile_number > max_missile) s->players[i].missile_number = max_missile;
					}else{
						s->players[i].hp += 10;
						if(s->players[i].hp > max_hp) s->players[i].hp = max_hp;
					}
				}
				else if(s->item[j].type == 11 and s->item[j].state > 0){
						s->item[j].visible = 0;
						set_effect(item[j].x, item[j].y, 2, -1);
				}
			}
		}
	}



	//機体とブロックの接触判定
	for(i=0; i < max_players; ++i) {
		if(s->players[i].visible) block_and_player_hitCheck(&players[i]);
	}
	//弾とブロックの接触判定(水にはぶつからない)
	for(i=0; i < max_players; ++i) {
		for(j=0; j < max_tama; ++j) {
			if(s->players[i].tama[j].visible) block_and_tama_hitCheck(i, j);
		}
	}

	//弾とミサイルの当たり判定
	for(i=0; i < max_players; ++i) {
			for(j=0; j < max_players; ++j) {
				for (k=0; k < max_tama; ++k) {
					for (int l=0; l < max_tama; ++l) {
						if(distance2(players[i].tama[k].x - players[j].tama[l].x, players[i].tama[k].y - players[j].tama[l].y)
								< (tama[s->players[i].tama[k].type].r + tama[s->players[j].tama[l].type].r) * (tama[s->players[i].tama[k].type].r + tama[s->players[j].tama[l].type].r)
								and s->players[i].tama[k].visible
								and s->players[j].tama[l].visible
								and i!=j)
						{
						if(s->players[i].tama[k].type == 5)	s->players[i].tama[k].visible = 0, set_effect(players[i].tama[k].x, players[i].tama[k].y, 3);
						if(s->players[j].tama[l].type == 5)	s->players[j].tama[l].visible = 0, set_effect(players[j].tama[l].x, players[j].tama[l].y, 3);
						}
					}
				}
			}
	}

	//HPの色
	for(i=0; i<max_players; ++i){
		if		(s->players[i].hp >=40) s->players[i].hp_r =0.0,s->players[i].hp_g =0.0,s->players[i].hp_b =1.0;
		else if(s->players[i].hp >=15)	s->players[i].hp_r =1.0,s->players[i].hp_g =1.0,s->players[i].hp_b =0.0;
		else							s->players[i].hp_r =1.0,s->players[i].hp_g =0.0,s->players[i].hp_b =0.0;
	}
	//撃破判定
	for(i=0; i<max_players; ++i){
		if(s->players[i].hp <= 0 and !players[i].destroyed){
			s->players[i].hp = 0;
			s->players[i].visible = 0;
			players[i].destroyed = 1;
			set_effect(players[i].x, players[i].y, 1);
		}
	}

	//ミサイルのロックオン判定(砲塔の向きから一定角度内　＆　自機から一定範囲内　にいる他プレイヤーのうち、最も近いプレイヤーをロック)
	for(i=0; i<max_players; ++i){
		float min_distance = window_w/2 * window_w/2;
		s->players[i].lock = -5;
		for(j=0; j<max_players; ++j){
			if(s->players[j].visible and s->players[i].shot_type == 5){
				float angle = atan2(players[j].y - players[i].y, players[j].x - players[i].x);
				float angle_gap;
				if(angle >= M_PI/2) angle_gap = fabs(angle - (s->players[i].angle + 3*M_PI/2));
				else angle_gap = fabs(angle - (s->players[i].angle - M_PI/2));
				if(angle_gap < M_PI/4
						and distance2(players[j].y - players[i].y, players[j].x - players[i].x) < min_distance
						and i!=j){
					min_distance = distance2(players[j].y - players[i].y, players[j].x - players[i].x);
					s->players[i].lock = -(j+1);
					players[i].lock_count[j]++;
					if(s->players[j].up_mode < 3 and players[i].lock_count[j] > 30) s->players[i].lock = j;
					else if(s->players[j].up_mode == 3 and players[i].lock_count[j] > 50) s->players[i].lock = j;
				}
				else players[i].lock_count[j] = 0;
			}
		}
	}

	//相対座標の決定
	for(i=0; i < max_players; ++i){
		float px = players[i].x;
		float py = players[i].y;
		float camera_x, camera_y;

		if(px < window_w/2) camera_x = -(window_w/2 - px);
		else if(px > map_w - window_w/2) camera_x = window_w/2 - (map_w - px);
		else camera_x = 0;
		if(py < window_h/2) camera_y = -(window_h/2 - py);
		else if(py > map_h - window_h/2) camera_y = window_h/2 - (map_h - py);
		else camera_y = 0;

		s->xx[i] = window_w/2 - px + camera_x;
		s->yy[i] = window_h/2 - py + camera_y;

		for(j=0; j < max_players; ++j){
			s->players[j].xx[i] = players[j].x + s->xx[i];
			s->players[j].yy[i] = players[j].y + s->yy[i];
			for (k=0; k < max_tama; ++k) {
				s->players[j].tama[k].xx[i] = players[j].tama[k].x + s->xx[i];
				s->players[j].tama[k].yy[i] = players[j].tama[k].y + s->yy[i];
			}
		}for (j=0; j <max_item; ++j) {
			s->item[j].xx[i] = item[j].x + s->xx[i];
			s->item[j].yy[i] = item[j].y + s->yy[i];
		}for (j=0; j <max_effect; ++j) {
			s->effect[j].xx[i] = effect[j].x + s->xx[i];
			s->effect[j].yy[i] = effect[j].y + s->yy[i];
		}
	}

	//他プレイヤーが画面外にいるときの処理
	for(i=0; i < max_players; ++i){
		for(j=0; j < max_players; ++j){
			float pxx = s->players[j].xx[i];
			float pyy = s->players[j].yy[i];
			s->players[j].marker_angle[i] = atan2(s->players[j].yy[i] - window_h/2, s->players[j].xx[i] - window_w/2) + M_PI/2;
			s->players[j].outside[i] = 0;
			if(fabs(atan((pyy - window_h/2)/(pxx - window_w/2)))
					< atan(float(window_h)/float(window_w))){
				if(pxx > window_w){
					s->players[j].marker_xx[i] = window_w-10;
					s->players[j].outside[i] = 1;
					s->players[j].marker_yy[i] = window_h/2 + (pyy - window_h/2) * window_w/2 / (pxx - window_w/2);
				}else if(pxx < 0){
					s->players[j].marker_xx[i] = 10;
					s->players[j].outside[i] = 1;
					s->players[j].marker_yy[i] = window_h/2 - (pyy - window_h/2) * window_w/2 / (pxx - window_w/2);
				}
			}else{
				if(pyy > window_h){
					s->players[j].marker_yy[i] = window_h-10;
					s->players[j].outside[i] = 1;
					s->players[j].marker_xx[i] = window_w/2 + (pxx - window_w/2) * window_h/2 / (pyy - window_h/2);
				}else if(pyy < 0){
					s->players[j].marker_yy[i] = 10;
					s->players[j].outside[i] = 1;
					s->players[j].marker_xx[i] = window_w/2 - (pxx - window_w/2) * window_h/2 / (pyy - window_h/2);
				}
			}
		}
	}

}

void Model::stepPlayer(int id, input_t *in){

	//自機本体の方向転換、移動
	int x_angle = in->right - in->left;
	int y_angle = in->up - in->down;

	if(!x_angle and !y_angle){
		players[id].accel_x = 0.0;
		players[id].accel_y = 0.0;
	}else{
		if(x_angle == 0 and y_angle == -1) 	s->players[id].base_angle = M_PI;
		else s->players[id].base_angle = x_angle*M_PI * (2-y_angle)/4;
		players[id].accel_x = players[id].move_speed * sin(s->players[id].base_angle);
		players[id].accel_y = -players[id].move_speed * cos(s->players[id].base_angle);
	}


	//砲塔の回転
	s->players[id].angle += M_PI/16 * players[id].rotate_speed * (in->yaw_right - in->yaw_left);
	if(s->players[id].angle >= M_PI) s->players[id].angle -= 2*M_PI;
	if(s->players[id].angle < -M_PI) s->players[id].angle += 2*M_PI;

	//弾の発射
	if (in->shot and !players[id].reload and s->players[id].visible
			and !(s->players[id].shot_type == 5 and s->players[id].missile_number <= 0)){
		for (i=0; i < max_tama; ++i) {
			if (!s->players[id].tama[i].visible){
				s->players[id].tama[i].visible = 1;
				s->players[id].tama[i].count = 0;
				s->players[id].tama[i].type = s->players[id].shot_type;
				players[id].reload = 1;
				s->players[id].reload_count = 0;
				s->players[id].reload_time = tama[s->players[id].tama[i].type].reload_time / players[id].reload_speed;
				s->players[id].tama[i].angle = s->players[id].angle;
				if(s->players[id].tama[i].type < 5){
					players[id].tama[i].x = players[id].x + cos(s->players[id].angle-M_PI/2)*90;
					players[id].tama[i].y = players[id].y + sin(s->players[id].angle-M_PI/2)*90;
					players[id].tama[i].vx = tama[s->players[id].tama[i].type].v * players[id].shot_speed * cos(s->players[id].tama[i].angle - M_PI/2);
					players[id].tama[i].vy = tama[s->players[id].tama[i].type].v * players[id].shot_speed * sin(s->players[id].tama[i].angle - M_PI/2);
				}
				else if(s->players[id].tama[i].type == 5 and s->players[id].missile_number > 0){
					players[id].rl *= -1;
					players[id].tama[i].x = players[id].x + players[id].rl * cos(s->players[id].angle)*30;
					players[id].tama[i].y = players[id].y + players[id].rl * sin(s->players[id].angle)*30;
					players[id].tama[i].vx = tama[s->players[id].tama[i].type].v/2 * players[id].rl * cos(s->players[id].tama[i].angle);
					players[id].tama[i].vy = tama[s->players[id].tama[i].type].v/2 * players[id].rl * sin(s->players[id].tama[i].angle);
					players[id].tama[i].lock = s->players[id].lock;
					s->players[id].missile_number--;
					if(s->players[id].missile_number <= 0) {
						in->change_shot = 1;
						players[id].having_shot_type[5] = 0;
					}
				}
				break;
			}
		}
	}

	//弾の切り替え
	if (in->change_shot){
		for(int i = 0; i < max_tama_type; i++){
			if(s->players[id].shot_type == max_tama_type) s->players[id].shot_type = 1;
			else s->players[id].shot_type++;
			if(players[id].having_shot_type[s->players[id].shot_type]) break;
		}
	}

	//プレイヤーの移動
		players[id].ax = players[id].accel_x - players_move_c * players[id].vx;
		players[id].vx += players[id].ax;
		players[id].x += players[id].vx;

		players[id].ay = players[id].accel_y - players_move_c * players[id].vy;
		players[id].vy += players[id].ay;
		players[id].y += players[id].vy;

	//弾の移動
	for (i=0; i < max_tama; ++i) {
		if(s->players[id].tama[i].visible){
			if(s->players[id].tama[i].type == 5 and s->players[id].tama[i].count >= 5) missile_move(id, i);
			players[id].tama[i].x += players[id].tama[i].vx;
			players[id].tama[i].y += players[id].tama[i].vy;
		}
	}

}
//1人プレイのときは他の3機がCPUとして以下の行動をする
void Model::CPU_Action(int id, input_t *in){
	//キー入力を初期化
	in->right = in->left = in->up = in->down = in->yaw_right = in->yaw_left = in->shot = 0;

	//常に射撃
	in->shot = 1;

	//一定間隔で移動方向を変える（止まるときもある）
	static int x_move[3], y_move[3];
	if(s->time%100 == 0){
		x_move[id-1] = g_random_int_range(0, 3);
		y_move[id-1] = g_random_int_range(0, 3);
	}
	if(x_move[id-1] == 1) in->right = 1;
	else if(x_move[id-1] == 2) in->left = 1;
	if(y_move[id-1] == 1) in->up = 1;
	else if(y_move[id-1] == 2) in->down = 1;

	//砲塔は常に自機を狙う
	float target_angle;
	target_angle = atan2(players[0].y - players[id].y, players[0].x - players[id].x);
	if(target_angle >=M_PI/2) target_angle -= 3*M_PI/2;
	else target_angle += M_PI/2;
	if(fabs(target_angle - s->players[id].angle)  > M_PI/32){
		if(s->players[id].angle < target_angle) in->yaw_right = 1;
		else if(s->players[id].angle > target_angle) in->yaw_left = 1;
	}
}

//位置、種類、対象を指定してエフェクトを発生させる
void Model::set_effect(float x,float y,int type, int target){
	for(int i=0; i < max_effect; i++){
		if(!s->effect[i].visible){
			effect[i].x = x;
			effect[i].y = y;
			effect[i].target = target;
			s->effect[i].type = type;
			s->effect[i].visible = 1;
			s->effect[i].count = 0;
			if(type == 2) s->effect[i].r = 0.0, s->effect[i].w = 100.0;
			else if(type == 3) s->effect[i].r = 0.0, s->effect[i].w = 40.0;
			else if(type == 4) s->effect[i].r = 5.0, s->effect[i].a = 1.0;
			else if(type == 5) s->effect[i].r = 30.0;
			if(type == 2 or type == 3) for(int j=0; j < max_players; j++) effect[i].damage_flag[j] = 1;
			break;
		}
	}
}

//ミサイルの移動計算を行う
void Model::missile_move(int id, int i){
	const int type = 5;
	const float move_c = 0.05;
	const float accel = move_c * tama[type].v * players[id].shot_speed;
	const float remote_value = M_PI/96; //ミサイル追尾性能（１フレームでの最大補正角度）
	float x = players[id].tama[i].x;
	float y = players[id].tama[i].y;

	if(players[id].tama[i].lock >= 0){
		int target = players[id].tama[i].lock;
		float target_angle = atan2(players[target].y - players[id].tama[i].y, players[target].x - players[id].tama[i].x);

		if(target_angle >=M_PI/2) target_angle -= 3*M_PI/2;
		else target_angle += M_PI/2;

		if(fabs(target_angle - s->players[id].tama[i].angle) < remote_value) s->players[id].tama[i].angle = target_angle;
		else if(s->players[id].tama[i].angle < target_angle) s->players[id].tama[i].angle += remote_value;
		else if(s->players[id].tama[i].angle > target_angle) s->players[id].tama[i].angle -= remote_value;
	}
	float accel_x = accel * cos(s->players[id].tama[i].angle - M_PI/2);
	float accel_y = accel * sin(s->players[id].tama[i].angle - M_PI/2);
	float ax = accel_x - move_c * players[id].tama[i].vx;
	float ay = accel_y - move_c * players[id].tama[i].vy;
	players[id].tama[i].vx += ax;
	players[id].tama[i].vy += ay;
	if(s->players[id].tama[i].count%3 == 0)
		set_effect(x - cos(s->players[id].tama[i].angle - M_PI/2) * 10,
				y - sin(s->players[id].tama[i].angle - M_PI/2) * 10, 4);
}

//二点間の距離の二乗を返す
static float distance2(float x, float y){
	return x * x + y * y;
}
//引数の座標のマップチップ種類を返す
int Model::get_MapChip(int mx, int my){
	if(mx < 0 or mx >= map_mw or my < 0 or my >= map_mh) return 1;
	return s->map[my][mx].type;
}

//マップのランダム生成アルゴリズム
void Model::create_Map(){

	//マップセルの初期化（外周ブロックのみにする）
	for(int y = 0; y < map_mh; y++){
		for(int x = 0; x < map_mw; x++){
			if(x==0 or x==map_mw-1 or y==0 or y==map_mh-1) s->map[y][x].type = 1;
			else s->map[y][x].type = 0;
		}
	}
	//フィールドを複数の部屋に分割（ランダムサイズで二分割を繰り返す）
	int div_type = g_random_int_range(0, 2);
	if(div_type == 0) div_type = -1;
	room[0].mx = 1;
	room[0].my = 1;
	room[0].mw = map_mw-2;
	room[0].mh = map_mh-2;
	int temp = 0;
	for(i = 1; i < max_room; i++,div_type*=-1){
		if(div_type == 1){
			room[i].mw = g_random_int_range(room[temp].mw * min_room_area /100, room[temp].mw * max_room_area /100);
			room[i].mh = room[temp].mh;
			room[temp].mw = room[temp].mw - room[i].mw;
			room[i].mx = room[temp].mx + room[temp].mw;
			room[i].my = room[temp].my;
			if(room[i].mw > room[temp].mw) temp = i;
		}if(div_type == -1){
			room[i].mw = room[temp].mw;
			room[i].mh = g_random_int_range(room[temp].mh * min_room_area /100, room[temp].mh * max_room_area /100);
			room[temp].mh = room[temp].mh - room[i].mh;
			room[i].mx = room[temp].mx;
			room[i].my = room[temp].my + room[temp].mh;
			if(room[i].mh > room[temp].mh) temp = i;
		}
	}
	//各部屋にブロックをランダムに選んだパターンで配置
	for(i = 0; i < max_room; i++){
		set_MapChip(&room[i], g_random_int_range(0,2));
	}

	//配置位置（周囲のブロックセルの有無）からブロックセルの種類を分類
	for(int y = 0; y < map_mh; y++){
		for(int x = 0; x < map_mw; x++){
			if(s->map[y][x].type > 0) judge_MapChip(x, y);
		}
	}
}
//ルーム内にチップをランダムに選ばれたパターンで配置する
void Model::set_MapChip(m_room_t *r, int type){
	int mx, mxs, mxl;
	int my, mys, myl;
	int mw, mh;
	mxs = r->mx;
	mxl = mxs + r->mw - 1;
	mys = r->my;
	myl = mys + r->mh - 1;
	int block_type = g_random_int_range(1, 3);
	switch(type){
	case 0:
		mx = g_random_int_range(mxs, mxl+1);
		my = g_random_int_range(mys, myl+1);
		mw = g_random_int_range(1, mxl-mx+2);
		mh = g_random_int_range(1, myl-my+2);
		for(int y=my; y < my+mh; y++){
			for(int x=mx; x < mx+mw; x++){
				s->map[y][x].type = block_type;
			}
		}
		break;
	case 1:
		mx = g_random_int_range(mxs, mxl-1);
		my = g_random_int_range(mys, myl-1);
		mw = g_random_int_range(3, mxl-mx+2);
		mh = g_random_int_range(3, myl-my+2);
		int del = 0;
		do{
			for(int y=my; y < my+mh; y++){
				for(int x=mx; x < mx+mw; x++){
					if(((x==mx or x==mx+mw-1) and y!=my and y!=my+mh-1)
						or ((y==my or y==my+mh-1) and x!=mx and x!=mx+mw-1)){
						s->map[y][x].type = g_random_int_range(0,2);
						if(s->map[y][x].type > 0) s->map[y][x].type = block_type;
						else del++;
					}
				}
			}
		}while(del < 1);
		break;
	}
}
//配置位置（周囲のセルの状態）からチップの種類を分類
void Model::judge_MapChip(int mx, int my){
	for(int i=0; i < 4; i++){
		int vx = (i<2)? -1 : 1;
		int vy = (0<i and i<3)? -1 : 1;

		int mc  = get_MapChip(mx,      my     );
		int xc  = get_MapChip(mx + vx, my     );
		int yc  = get_MapChip(mx,      my + vy);
		int xyc = get_MapChip(mx + vx, my + vy);

		//int chip = &s->map[my][mx].chip;
		int n = 4;
		if(mc == xc and mc == yc and mc == xyc) s->map[my][mx].chip[i] = 4 * n + i;
		else if(mc == xc and mc == yc) s->map[my][mx].chip[i] = 3 * n + i;
		else if(mc == yc) s->map[my][mx].chip[i] = 2 * n + 1 + (1+vx);
		else if(mc == xc) s->map[my][mx].chip[i] = 2 * n + (1-vy);
		else s->map[my][mx].chip[i] = 1 * n + i;
	}
}
//プレイヤーとブロックセルとの当たり判定を行う
void Model::block_and_player_hitCheck(m_player_t *p){
	float x, xs, xl;
	float y, ys, yl;
	int mx, mxs, mxl;
	int my, mys, myl;
	float r = players_cr;
	x = p->cx;
	y = p->cy;
	xs = x - r;
	xl = x + r;
	ys = y - r;
	yl = y + r;
	mx = x / mapchip_size;
	my = y / mapchip_size;
	mxs = xs / mapchip_size;
	mxl = xl / mapchip_size;
	mys = ys / mapchip_size;
	myl = yl / mapchip_size;

	//円の十字方向の端4点がブロックセル内かどうかを調べ、そうなら位置補正
	if(s->map[my][mxs].type > 0 or s->map[my][mxl].type > 0 or s->map[mys][mx].type > 0 or s->map[myl][mx].type > 0){
		if(s->map[my][mxs].type > 0) p->x = (mxs+1) * mapchip_size + r - p->cx_off, p->vx = 0;
		if(s->map[my][mxl].type > 0) p->x = (mxl+0) * mapchip_size - r - p->cx_off, p->vx = 0;
		if(s->map[mys][mx].type > 0) p->y = (mys+1) * mapchip_size + r - p->cy_off, p->vy = 0;
		if(s->map[myl][mx].type > 0) p->y = (myl+0) * mapchip_size - r - p->cy_off, p->vy = 0;
	}
	//円のあるセルの周囲にあるブロックセルを調べ、その角が円内部にあるかどうかを調べ、そうなら位置補正
	else{
		float bxs, bxl;
		float bys, byl;
		bxs = (mx+0) * mapchip_size;
		bxl = (mx+1) * mapchip_size;
		bys = (my+0) * mapchip_size;
		byl = (my+1) * mapchip_size;
		if(s->map[my-1][mx-1].type > 0 and distance2(x-bxs, y-bys) < r*r) {
			float a = (y-bys)/(x-bxs);
			float dx = sqrt(r*r/(a*a+1));
			p->x = bxs +   dx - p->cx_off;
			p->y = bys + a*dx - p->cy_off;
			if(!p->accel_x < 0) p->vx = 0;
			if(!p->accel_y < 0) p->vy = 0;
		}if(s->map[my-1][mx+1].type > 0 and distance2(bxl-x, y-bys) < r*r) {
			float a = (y-bys)/(bxl-x);
			float dx = sqrt(r*r/(a*a+1));
			p->x = bxl -   dx - p->cx_off;
			p->y = bys + a*dx - p->cy_off;
			if(!p->accel_x > 0) p->vx = 0;
			if(!p->accel_y < 0) p->vy = 0;
		}if(s->map[my+1][mx-1].type > 0 and distance2(x-bxs, byl-y) < r*r) {
			float a = (byl-y)/(x-bxs);
			float dx = sqrt(r*r/(a*a+1));
			p->x = bxs +   dx - p->cx_off;
			p->y = byl - a*dx - p->cy_off;
			if(!p->accel_x < 0) p->vx = 0;
			if(!p->accel_y > 0) p->vy = 0;
		}if(s->map[my+1][mx+1].type > 0 and distance2(bxl-x, byl-y) < r*r) {
			float a = (byl-y)/(bxl-x);
			float dx = sqrt(r*r/(a*a+1));
			p->x = bxl -   dx - p->cx_off;
			p->y = byl - a*dx - p->cy_off;
			if(!p->accel_x > 0) p->vx = 0;
			if(!p->accel_y > 0) p->vy = 0;
		}
	}
}
//弾とブロックの当たり判定を行う。ただし、ここでの弾の当たり判定は正方形（円だとややこしい）
void Model::block_and_tama_hitCheck(int id, int i){
	float x, xs, xl;
	float y, ys, yl;
	int mxs, mxl;
	int mys, myl;
	float r = tama[s->players[id].tama[i].type].r;
	x = players[id].tama[i].x;
	y = players[id].tama[i].y;
	xs = x - r;
	xl = x + r;
	ys = y - r;
	yl = y + r;
	mxs = xs / mapchip_size;
	mxl = xl / mapchip_size;
	mys = ys / mapchip_size;
	myl = yl / mapchip_size;
	if(s->map[mys][mxs].type == 1 or s->map[mys][mxl].type == 1 or s->map[myl][mxs].type == 1 or s->map[myl][mxl].type == 1){
		s->players[id].tama[i].visible = 0;
		if(s->players[id].tama[i].type ==4) set_effect(x, y, 2, id);
		else if(s->players[id].tama[i].type ==5) set_effect(x, y, 3, id);
	}
}
