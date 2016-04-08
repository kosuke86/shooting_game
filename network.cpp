/*
 * network.cc
 * Copyright (C) Takahiro Yakoh 2011 <yakoh@sd.keio.ac.jp>
 * $Revision: 1.20 $
 */

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <glib.h>
#include <gtkmm.h>

#include "common.h"
#include "network.h"
#include "view.h"
#include "model.h"
#include "manager.h"

#ifdef linux
#define DEV_NAME "eth0"
#endif
#ifdef __APPLE__
#define DEV_NAME "en0"
#endif

struct member_t {
	int attend, ready;
	char name[20];
	GIOChannel *gioc;
	guint sid;
};
int w;

static int server_flag = 0;
struct member_t members[max_players];
static int num_attend = 0;
static bool is_server_start = false;
static bool is_client_start = false;

void sendScene(int id, Scene *scene){
	struct message_t m;
	gsize n;
	m.command = SCMD_DRAW;
	m.length = sizeof(Scene);
	if (m.length > max_msglen) {
		std::cout << "一度に送るデータ量が大き過ぎます。" << m.length
				<< " < max_msglenを見直してください。" << std::endl;
		exit(0);
	}
	g_io_channel_write_chars(members[id].gioc, (char *) &m,
			sizeof(message_t), &n, NULL);
	g_io_channel_write_chars(members[id].gioc, (char *) scene,
			m.length, &n, NULL);
}

unsigned int get_myip(void) {
	int s;
	struct ifreq ifr;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, DEV_NAME, IFNAMSIZ - 1);
	ioctl(s, SIOCGIFADDR, &ifr);
	close(s);
	return ntohl(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr);
}

bool channel_read(GIOChannel* gioc, gchar *buffer, int length) {
	gsize s;

	do {
		if (g_io_channel_read_chars(gioc, buffer, length, &s, NULL)
				!= G_IO_STATUS_NORMAL)
			return false;
		buffer += s;
		length -= s;
	} while (length > 0);
	return true;
}

static int find_member_from_gioc(GIOChannel* gioc) {
	int i;
	for (i = 0; i < max_players; ++i) {
		if (members[i].gioc == gioc)
			return i;
	}
	return -1;
}

bool broadcast_message() {
	char buffer[50];
	int a, r, i;
	struct message_t m;
	gsize n;

	for (i = a = r = 0; i < max_players; ++i) {
		if (members[i].attend)
			a++;
		if (members[i].ready)
			r++;
	}
	sprintf(buffer, "%d / %d", r, a);
	m.command = SCMD_STATUS;
	m.length = strlen(buffer) + 1;
	for (i = 1; i < max_players; ++i) {
		if (members[i].attend) {
			g_io_channel_write_chars(members[i].gioc, (char *) &m,
					sizeof(message_t), &n, NULL);
			g_io_channel_write_chars(members[i].gioc, buffer,
					m.length, &n, NULL);
		}
	}
	statusBar->push(Glib::ustring(buffer), statusId++);
	g_timeout_add(5000, eraseStatusbar, 0);

	if (a == r)
		return true;
	else
		return false;
}

void process_cmd(int id, int command, int length, GIOChannel* gioc) {
	int i;
	char message[max_msglen];
	struct message_t m;
	gsize s;
	Manager &mgr = Manager::get_instance();

	switch (command) {
	case SCMD_CONNECT:
		members[id].attend = 1;
		channel_read(gioc, (gchar *) members[id].name, length);
		broadcast_message();
		break;
	case SCMD_START:
		members[id].ready = 1;
		if (broadcast_message()) {
			server_flag = 1;
			for (i = 0; i < max_players; ++i) {
				if (members[i].attend) {
					mgr.attend_player(i);
					strcpy(mgr.get_scene().players[i].name, members[i].name);
				} else {
					mgr.absent_player(i);
				}
			}
			g_timeout_add(period, Manager::tickServer, (gpointer) NULL);
		}
		break;
	case SCMD_STOP:
		members[id].ready = 0;
		broadcast_message();
		server_flag = 0;
		break;
	case SCMD_DISCONNECT:
		members[id].attend = 0;
		broadcast_message();
		break;
	case SCMD_INPUT:
		//		channel_read(gioc, (gchar *) &members[id].input, length);
		channel_read(gioc, (gchar *) &input[id], length);
		break;
	case SCMD_STATUS:
		channel_read(gioc, (gchar *) message, length);
		statusBar->push(Glib::ustring(message), statusId++);
		g_timeout_add(5000, eraseStatusbar, 0);
		break;
	case SCMD_DRAW:
		channel_read(gioc, (gchar *) &(mgr.get_scene()), length);
		process_a_step(&(mgr.get_scene()), &input[id]);
//		std::cout << members[id].input.x << std::endl;
		m.command = SCMD_INPUT;
		m.length = sizeof(input_t);
		g_io_channel_write_chars(members[0].gioc, (char *) &m,
				sizeof(message_t), &s, NULL);
		g_io_channel_write_chars(members[0].gioc, (char *) &input[id],
				m.length, &s, NULL);
		break;
	}
}

gboolean server_receive(GIOChannel* gioc, GIOCondition cond, void *arg) {
	int player;
	struct message_t m;

	player = find_member_from_gioc(gioc);
	if (channel_read(gioc, (gchar *) &m, sizeof(message_t))) {
		process_cmd(player, m.command, m.length, gioc);
	} else {
		g_source_remove(members[player].sid);
		g_io_channel_shutdown(members[player].gioc, TRUE, NULL);
		members[player].attend = 0;
		num_attend--;
		return FALSE;
	}
	return TRUE;
}

gboolean server_accept(GIOChannel *gioc, GIOCondition cond, void *arg) {
	struct sockaddr_in cli;
	unsigned int len=sizeof(cli);
	int s, on;

	s = accept(g_io_channel_unix_get_fd(gioc), (struct sockaddr *) &cli, &len);
	on = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

	if (num_attend == max_players) {
		close(s);
		return TRUE;
	}
	members[num_attend].attend = 1;
	members[num_attend].gioc = g_io_channel_unix_new(s);
	g_io_channel_set_encoding(members[num_attend].gioc, NULL, NULL);
	g_io_channel_set_buffered(members[num_attend].gioc, FALSE);
	members[num_attend].sid = g_io_add_watch(members[num_attend].gioc, G_IO_IN,
			server_receive, NULL);
	num_attend++;

	return TRUE;
}

bool server_setup(const char *port, const char *name, input_t *input) {
	int i, on;
	struct sockaddr_in serv;

	if (is_server_start)
		return true;

	if ((w = socket(PF_INET, SOCK_STREAM, 0)) == (-1)) {
		perror("Can't create a socket.\n");
		return false;
	}
	on = 1;
	setsockopt(w, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	setsockopt(w, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = INADDR_ANY;
	serv.sin_port = htons(atoi((const char *) port));

	if (bind(w, (struct sockaddr *) &serv, sizeof(serv)) != 0) {
		perror("Can't bind the socket.\n");
		return false;
	}

	if (listen(w, 0) == -1) {
		perror("Can't listen the socket.\n");
		return false;
	}

	for (i = 0; i < max_players; ++i) {
		members[i].attend = 0;
		members[i].ready = 0;
		members[i].gioc = NULL;
		members[i].sid = 0;
//		members[i].input=&input[i];
	}

	members[0].gioc = g_io_channel_unix_new(w);
	g_io_channel_set_encoding(members[0].gioc, NULL, NULL);
	g_io_channel_set_buffered(members[0].gioc, FALSE);
	members[0].sid = g_io_add_watch(members[0].gioc, G_IO_IN, server_accept,
			NULL);
	members[0].attend = 1;
	num_attend = 1;
	strcpy(members[0].name, name);

	is_server_start = true;
	return true;
}

void server_start(void) {
	process_cmd(0, SCMD_START, 0, NULL);
}
void server_stop(void) {
	process_cmd(0, SCMD_STOP, 0, NULL);
}
bool server_terminate(void) {
	int i;

	if (!is_server_start)
		return false;
	for (i = 0; i < max_players; ++i) {
		if (members[i].attend) {
			g_source_remove(members[i].sid);
			g_io_channel_shutdown(members[i].gioc, TRUE, NULL);
		}
	}
	if(w){
		close(w);
		w=0;
	}
	is_server_start = false;
	return true;
}

bool client_setup(const char *addr, const char *port, const char *name) {
	struct sockaddr_in serv;
	struct hostent *dst;
	struct message_t m;
	int s, on;
	gsize n;

	if (is_client_start)
		return true;

	if ((s = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		return false;
	}
	on = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

	if (!(dst = gethostbyname(addr))) {
		close(s);
		return false;
	}
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = *((in_addr_t *) *dst->h_addr_list);
	serv.sin_port = htons(atoi(port));
	if (connect(s, (struct sockaddr *) &serv, sizeof(serv)) == (-1)) {
		close(s);
		return false;
	}
	members[0].gioc = g_io_channel_unix_new(s);
	g_io_channel_set_encoding(members[0].gioc, NULL, NULL);
	g_io_channel_set_buffered(members[0].gioc, FALSE);
	members[0].sid = g_io_add_watch(members[0].gioc, G_IO_IN, client_receive,
			NULL);
	m.command = SCMD_CONNECT;
	m.length = strlen(name) + 1;
	g_io_channel_write_chars(members[0].gioc, (char *) &m,
			sizeof(message_t), &n, NULL);
	g_io_channel_write_chars(members[0].gioc, name,
			m.length, &n, NULL);
	members[0].attend = 1;
	is_client_start = 1;
	return true;
}

void client_start(void) {
	struct message_t m;
	gsize n;

	m.command = SCMD_START;
	m.length = 0;
	g_io_channel_write_chars(members[0].gioc, (char *) &m,
			sizeof(message_t), &n, NULL);
}

gboolean client_receive(GIOChannel *gioc, GIOCondition cond, void *arg) {
	struct message_t m;

	if (channel_read(gioc, (gchar *) &m, sizeof(message_t))) {
		process_cmd(0, m.command, m.length, gioc);
	} else {
		g_source_remove(members[0].sid);
		g_io_channel_shutdown(members[0].gioc, TRUE, NULL);
		members[0].attend = 0;
		members[0].ready = 0;
		is_client_start = 0;
//		reset_mode();

		statusBar->push(Glib::ustring("サーバが接続を切断したため、スタンドアローンモードに変更しました。"), statusId++);
		g_timeout_add(5000, eraseStatusbar, 0);
}
	return true;
}

void client_stop(void) {
	struct message_t m;
	gsize n;

	m.command = SCMD_STOP;
	m.length = 0;
	g_io_channel_write_chars(members[0].gioc, (char *) &m,
			sizeof(message_t), &n, NULL);
}

bool client_terminate(void) {
	struct message_t m;
	gsize n;
	if (!is_client_start)
		return false;

	m.command = SCMD_DISCONNECT;
	m.length = 0;
	g_io_channel_write_chars(members[0].gioc, (char *) &m,
			sizeof(message_t), &n, NULL);
	g_source_remove(members[0].sid);
	g_io_channel_shutdown(members[0].gioc, TRUE, NULL);
	is_client_start = false;
	return true;
}
