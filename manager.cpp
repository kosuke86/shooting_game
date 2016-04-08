/*
 * manager.cpp
 *
 *  Created on: 2013/12/17
 *      Author: sdjikken
 */

#include "manager.h"
#include "network.h"

Manager& Manager::get_instance() {
	static Manager instance;
	return instance;
}

void Manager::init_status() {
	state = Stop;
	mode = Standalone;
}

void Manager::init_objects() {
	model.initModelWithScene(&s);
	ViewManager::get_instance().init_view_with_scene(&s);
}

const Manager::State Manager::get_state() const {
	return state;
}
void Manager::set_state(State s) {
	state = s;
}

const Manager::Mode Manager::get_mode() const {
	return mode;
}
void Manager::set_mode(Manager::Mode s) {
	mode = s;
}

void Manager::attend_single_player() {
	s.players[0].attend = 1;
}

void Manager::attend_player(int id) {
	s.players[id].attend = 1;
}

void Manager::absent_player(int id) {
	s.players[id].attend = 0;
}

gboolean Manager::tick(void *p) {
	Manager &mgr = Manager::get_instance();
	ViewManager &view = ViewManager::get_instance();

	int run = mgr.model.single_judgeState();

	if(run < 2) view.get_input(&input[0]);
	if(run == 1) for(int i=1; i<max_players; ++i) mgr.model.CPU_Action(i, &input[i]);
	if(run == 0) mgr.model.startMenu(0, &input[0]);
	if(run > 0) mgr.model.preAction();
	if(run == 1) for(int i=0; i<max_players; ++i) mgr.model.stepPlayer(i, &input[i]);
	if(run > 0) mgr.model.postAction();
	view.update();

	if (mgr.get_state() == Manager::Run) { // trueを返すとタイマーを再設定し、falseならタイマーを停止する
		return true;
	} else {
		view.init_view_with_scene(NULL);
		return false;
	}
}

gboolean Manager::tickServer(void *p) {
	Manager &mgr = Manager::get_instance();
	ViewManager &view = ViewManager::get_instance();

	int run = mgr.model.judgeState();

	if(run < 2) view.get_input(&input[0]); // 他のプレーヤーの入力は、既に通信で非同期に届いている
	if(run == 0){
		for (int i = 0; i < max_players; ++i) {
			if (mgr.s.players[i].attend) {
				mgr.model.startMenu(i, &input[i]);
			}
		}
	}
	if(run > 0) mgr.model.preAction();
	if(run == 1){
		for (int i = 0; i < max_players; ++i) {
			if (mgr.s.players[i].attend) {
				mgr.model.stepPlayer(i, &input[i]);
			}
		}
	}
	if(run > 0) mgr.model.postAction();
	for (int i = 1; i < max_players; ++i) { // 自分には送る必要ないので1から
		if (mgr.s.players[i].attend) {
			mgr.s.id = i;//後から追加したコード
			sendScene(i, &mgr.s);
			mgr.s.id = 0;//後から追加したコード
		}
	}
	view.update();
	if (mgr.get_state() == Manager::Run) { // trueを返すとタイマーを再設定し、falseならタイマーを停止する
		return true;
	} else {
		view.init_view_with_scene(NULL);
		return false;
	}
	return true;
}
