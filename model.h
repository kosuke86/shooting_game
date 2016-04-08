/*
 * model.h
 *
 *  Created on: 2013/12/09
 *      Author: yakoh
 */

#ifndef MODEL_H_
#define MODEL_H_
#include <time.h>

#include "common.h"

class Model {
public:
	Model();
	virtual ~Model();

	void initModelWithScene(Scene *);

	int single_judgeState();
	int judgeState();

	void startMenu(int, input_t *);
	void preAction();
	void stepPlayer(int, input_t *);
	void postAction();

	void CPU_Action(int, input_t *);

private:
	Scene *s;

	struct m_room_t {
		int mx, my;
		int mw, mh;
	};

	struct m_tama_t {
		float x, y, vx, vy;
		int lock;	//ミサイル自体のロック対象）
	};
	struct m_player_t {
		float x, vx, ax, accel_x;
		float y, vy, ay, accel_y;
		float move_speed, rotate_speed, shot_power, shot_speed, reload_speed, armor; //機体の能力値
		int up_count;
		//int level[5]; //機体の能力レベル(このレベルに応じて上の能力値が決定される)
		float cx_off, cy_off;
		float cx, cy; //当たり判定（円形）の中心座標
		int  reload, destroyed;
		int damage_count, lock_count[max_players], having_shot_type[max_tama_type+1];
		int rl;	//ミサイルの射出位置（右側面か左側面か)
		struct m_tama_t tama[max_tama];
	};
	struct m_item_t {
		float x, y;
		int count;
	};
	struct m_effect_t{
		float x, y, cr;
		int target, damage_flag[max_players];	//各プレイヤーへの当たり判定フラグ（爆発をくらい続けないようにするため）
	};

	struct m_room_t room[max_room];
	struct m_player_t players[max_players];
	struct m_item_t item[max_item];
	struct m_effect_t effect[max_effect];

	void set_effect(float,float,int,int target=0);
	void missile_move(int, int);
	void create_Map();
	int get_MapChip(int, int);
	void set_MapChip(m_room_t*, int);
	void judge_MapChip(int, int);
	void block_and_player_hitCheck(m_player_t*);
	void block_and_tama_hitCheck(int, int);

};

#endif /* MODEL_H_ */
