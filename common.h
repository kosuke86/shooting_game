/*
 * common.h
 *
 *  Created on: 2013/12/09
 *      Author: yakoh
 */

#ifndef COMMON_H_
#define COMMON_H_
#include <gtkmm.h>
//#include <math.h>
//#include <time.h>

const int period = 30;
const int max_msglen = 100000; //データが送れる最大の量
const int mapchip_size = 64; //マップチップの幅、高さ
const int map_mw = 30, map_mh = 30;	// マップの幅、高さ（マップチップの配置数）
const int map_w = mapchip_size * map_mw;	// マップの幅
const int map_h = mapchip_size * map_mh;	// マップの幅
const int window_w = 1200, window_h = 800;		// ウィンドウの幅、高さ

const int max_room = 5;
const int min_room_area = 25;
const int max_room_area = 100 - min_room_area;
const int max_block_area = 50;

const int max_players = 4; // 最大プレイヤー数
const int max_players_type = 6;//プレイヤーの種類の数
//const int max_players_level = 5; //プレイヤーの能力値の最大レベル
const int max_tama = 20;	// 一人が撃てる弾数
const int max_tama_type = 5;	// 弾の種類の数

const int max_item = 5;		//最大出現アイテム数
const int max_item_type = 2;	// アイテムの種類の数
const int item_cr = 32; //アイテムの当たり判定の半径
const int max_effect = 50;	//最大エフェクト数

const int block_overarea = 30;			//ブロックの最大重なり領域（当たり判定用）
const int players_cr = 32;	//当たり判定の半径
const int players_x_off = 0, players_y_off = -15; //当たり判定のxオフセット、yオフセット（cx,cyがx,yからどれだけズレてるか）
const int max_hp = 100; //最大HPの定義
const int max_missile = 30; //ミサイル最大所持数
const float players_move_c = 0.2; //プレイヤーの移動における減衰係数


////////////////////////////////////////////////////////////////

// 機体のタイプと能力値の関係を定義
struct players_parameter{
	float   move_speed,  rotate_speed, shot_power, shot_speed, reload_speed, armor;};
const struct players_parameter players_para[max_players_type+1] ={{},
	//			　移動速度	砲塔回転速度	　 攻撃力		弾速		リロード速度		防御力
	/* バランス型*/	{0.7,  		1.0,  		1.0,	  	1.0,		1.0,		 1.0},
	/* 高速移動型*/	{1.0,   	1.3,  	 	0.7,	  	1.0,		1.1,         0.8},
	/* 高火力型	*/	{0.5,  		1.0,  	 	1.3,	  	0.9,		0.8,		 1.2},
	/* 高弾速型	*/	{0.6,   	0.7,   		1.1,		1.3,		0.9,		 1.1},
	/* 連射型	*/	{0.7,   	1.2,   	 	0.9,	  	1.0,		1.3,		 0.9},
	/* 高防御型 	*/	{0.5,   	1.1,   	 	0.9,	  	1.0,		1.0,		 1.4}};

////////////////////////////////////////////////////////////////

// 弾の種類ごとのパラメータを定義
struct tama_parameter{
	float   		r,      v,	   power, reload_time, visible_time; };
const struct tama_parameter tama[max_tama_type+1] ={{},
	//				大きさ	弾速		威力		リロード		消滅時間
	/* 1 通常弾   */	{13,  	 5,  	 10,	  30,		100},
	/* 2 高速弾   */	{13,   	10,  	 13,	  50,		100},
	/* 3 連射弾   */	{7.5,  	 5,  	  5,	  10,		70},
	/* 4 爆発弾   */	{23,   	 3,   	 30,	  100,		70},
	/* 5 ミサイル */	{13,   	 8,   	 10,	  10,		150}};

////////////////////////////////////////////////////////////////

struct tama_t {
	float xx[max_players], yy[max_players];
	float angle;
	int  visible, count, type;
};
struct player_t {
	float xx[max_players], yy[max_players];
	float angle, base_angle;
	int hp;
	float hp_r, hp_g, hp_b;
	float reload_r/*, reload_g, reload_b*/;
	int reload_count;
	float reload_time;
	int  attend, visible, damage, type, shot_type, lock, up_mode, outside[max_players], missile_number;
	float marker_xx[max_players], marker_yy[max_players], marker_angle[max_players];
	struct tama_t tama[max_tama];
	char name[20];
};
struct item_t {
	float xx[max_players], yy[max_players];
	int visible, state, type;
};
struct effect_t{
	float xx[max_players], yy[max_players];
	float r, w, a;
	int visible, count, type;
};
struct map_t{
	int type;
	int chip[4];
};
// 画面全体を表す構造体。このサイズは、できるだけ小さく抑えるべし
struct Scene{
	struct item_t item[max_item];
	struct effect_t effect[max_effect];
	struct player_t players[max_players];
	struct map_t map[map_mh][map_mw];
	int time, id, run, winner;
	float xx[max_players], yy[max_players];
};
//ここでキーを格納して使えるようにする。
struct input_t {
	int up, down, left, right;
	int yaw_right, yaw_left, shot, change_shot;
	int enter, cancel, r, l;
	int x, y;
	int key;
};

extern Gtk::Statusbar *statusBar;
extern int statusId, statusEraseId;
//extern Scene *scene;
extern input_t input[max_players];

gboolean eraseStatusbar(void *p);
gboolean tickServer(void *p);
void process_a_step(Scene *s, input_t *in);

#endif /* COMMON_H_ */
