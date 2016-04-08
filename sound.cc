/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * sound.cc
 * Copyright (C) Takahiro Yakoh 2011 <yakoh@sd.keio.ac.jp>
 * $Revision: 1.11 $
 PulseAudioは音再生用サーバにアプリケーションがクライアントとして発音を依頼する
 通信モデルが採用されている。発音依頼には、毎回波形データを送信する方法と、
 予め波形データを送りつけて命名しておき(sample cache)名前指定する方法の2種類ある。
 名前指定の方が応答が速いため、ここでは名前指定を利用している。

 開始時、まずはPulseAudioとの接続を行い、接続したら波形データを必要回数だけ送りつける。
 この初期化作業もサーバの応答に合わせて進める必要があるため、こちらの処理とは非同期に
 ならざるを得ず、コールバック関数を利用して行っている。既にGTKを用いてる段階でglibの
 mainloopを利用しているので、ここにイベントとコールバック関数を登録している。
 このコールバック処理にも2種類が用いられており、1つは接続完了したら呼び出してもらう処理と、
 もう1種類は命名波形データ毎に送信準備完了したら呼び出してもらう処理である。これらの
 非同期な処理を通じて、波形データをサーバに送りつけている。以上をsound_initで行っている。

 あとは名前指定で再生するだけだ(sound_play)。

 プログラム終了時には、登録した音を削除し、接続を終了し、コールバック依頼を解除する。
 これらの作業はsound_finalizeで行っている。
 */

#include "common.h"
#ifdef SOUND
#include "sound.h"
#include <iostream>
#include <pulse/pulseaudio.h>
#include <pulse/context.h>
#include <pulse/glib-mainloop.h>
#include <sndfile.h>

#define CHANNELS	2
#define SAMPLERATE	44100
#define MAX_SOUNDS	3
#define MAX_SECONDS	10

pa_context *ctxt;
pa_glib_mainloop *m;
pa_proplist *p;
struct sounds_t {
	pa_stream *sts;
	char name[3];
};
sounds_t sounds[MAX_SOUNDS];

struct sndfile_t {
	const char *filename;
	SNDFILE *sf;
	SF_INFO sinfo;
};

static sndfile_t sndfiles[]= {
	{	"/usr/share/sounds/gnome/default/alerts/bark.ogg", NULL}, // 0
	{	"/usr/share/sounds/gnome/default/alerts/drip.ogg", NULL}, // 1
	{	"/usr/share/sounds/gnome/default/alerts/glass.ogg", NULL} // 2
};

static void context_state_callback(pa_context *c, void *userdata);
static void stream_notify_callback(pa_stream *st, void *userdata);
static void sound_prepare_upload(void);
static void sound_upload(pa_stream *st);

/* PulseAudioサーバと接続し、コールバック関数を登録している */
void sound_init(void) {
	p = pa_proplist_new();
	m = pa_glib_mainloop_new(NULL);
	ctxt = pa_context_new_with_proplist(pa_glib_mainloop_get_api(m),
			"samplegameapp", p);
	pa_context_connect(ctxt, NULL, pa_context_flags_t(PA_CONTEXT_NOAUTOSPAWN
					|PA_CONTEXT_NOFAIL), NULL);
	pa_context_set_state_callback(ctxt, context_state_callback, NULL);
}

/* 登録した音を名前指定で再生する */
void sound_play(int n) {
	int i, x;
	pa_operation *o;

	for(i=0, x=1; i<MAX_SOUNDS; ++i, x<<=1) {
		if(n & x) {
			o = pa_context_play_sample(ctxt, sounds[i].name, NULL, PA_VOLUME_NORM,
					NULL, NULL);
			if (o) {
				pa_operation_unref(o);
			}
		}
	}
}

/* 登録した音を削除し、接続を解除している */
void sound_finalize(void) {
	int n;
	pa_operation *o;

	for (n = 0; n < MAX_SOUNDS; ++n) {
		o=pa_context_remove_sample(ctxt, sounds[n].name, NULL, NULL);
		if (o) {
			pa_operation_unref(o);
		}
	}
	pa_context_disconnect(ctxt);
	pa_glib_mainloop_free(m);
}

/* 接続に関するコールバック関数であり、準備ができたら波形データ登録を起動している */
static void context_state_callback(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_FAILED:
		std::cout << "failed" << std::endl;
		pa_context_unref(ctxt);
		ctxt = NULL;
		break;
		case PA_CONTEXT_READY:
		sound_prepare_upload();
		break;
		default:
		break;
	}
}

/* 波形データの受け入れストリームの準備ができたら呼び出されるコールバック関数であり、
 波形データを送り出す関数を呼び出している。
 */
static void stream_notify_callback(pa_stream *st, void *userdata) {
	if (pa_stream_get_state(st) == PA_STREAM_READY) {
		sound_upload(st);
	}
}

/* 波形データを送りつけるためのストリームを作成し、コールバック関数を登録している。
 * ここで波形の種類（フォーマット、チャネル数、サンプリング周期）を決めている。 */
static void sound_prepare_upload(void) {
	int n, length;
	pa_sample_spec ss;

	for (n = 0; n < MAX_SOUNDS; ++n) {
		sndfiles[n].sinfo.format=0;
		sndfiles[n].sinfo.channels=2;
		sndfiles[n].sf=sf_open(sndfiles[n].filename, SFM_READ, &sndfiles[n].sinfo);
		if(sndfiles[n].sf==NULL) {
			std::cout << "Can't open file " << sndfiles[n].filename << std::endl;
			exit(0);
		}

		ss.format = PA_SAMPLE_S16NE;
		ss.channels = sndfiles[n].sinfo.channels;
		ss.rate = sndfiles[n].sinfo.samplerate;
		length=sndfiles[n].sinfo.frames;
		if(length>sndfiles[n].sinfo.samplerate*MAX_SECONDS) {
			length=sndfiles[n].sinfo.samplerate*MAX_SECONDS;
		}
		sndfiles[n].sinfo.frames=length;
		length=sndfiles[n].sinfo.frames*sndfiles[n].sinfo.channels*sizeof(short int);

		sounds[n].name[0] = 's';
		sounds[n].name[1] = n + '0';
		sounds[n].name[2] = 0;
		sounds[n].sts = pa_stream_new_with_proplist(ctxt, sounds[n].name, &ss,
				NULL, p);
		pa_stream_set_state_callback(sounds[n].sts, stream_notify_callback,
				NULL);
		pa_stream_connect_upload(sounds[n].sts, length);
	}
}

/* 波形データを送りつけている */
static void sound_upload(pa_stream *st) {
	short int buffer[400];
	int n, length, total;

	for (n = 0; n < MAX_SOUNDS; ++n) {
		if (sounds[n].sts == st)
		break;
	}
	length=sndfiles[n].sinfo.frames*sndfiles[n].sinfo.channels*sizeof(short int);
	for(total=0; total<sndfiles[n].sinfo.frames; total+=length) {
		length=sf_readf_short(sndfiles[n].sf, buffer, 100);
		pa_stream_write(st, buffer, length*sndfiles[n].sinfo.channels*sizeof(short int),
				NULL, 0, PA_SEEK_RELATIVE);
	}
	sf_close(sndfiles[n].sf);
	pa_stream_finish_upload(st);
}

#endif
