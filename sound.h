/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * sound.h
 * Copyright (C) Takahiro Yakoh 2011 <yakoh@sd.keio.ac.jp>
 * $Revision: 1.8 $
 */

#ifdef SOUND
void sound_init(void);
void sound_play(int);
void sound_finalize(void);
#endif
