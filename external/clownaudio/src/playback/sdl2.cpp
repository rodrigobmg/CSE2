/*
 *  (C) 2018-2020 Clownacy
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */

#include "clownaudio/playback.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

struct ClownAudio_Stream
{
	void (*user_callback)(void*, float*, size_t);
	void *user_data;

	SDL_AudioDeviceID device;
	float volume;
};

static bool sdl_already_init;

static unsigned int NextPowerOfTwo(unsigned int value)
{
	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value++;

	return value;
}

static void Callback(void *user_data, Uint8 *output_buffer_uint8, int bytes_to_do)
{
	ClownAudio_Stream *stream = (ClownAudio_Stream*)user_data;
	const unsigned long frames_to_do = bytes_to_do / (sizeof(float) * CLOWNAUDIO_STREAM_CHANNEL_COUNT);
	float *output_buffer = (float*)output_buffer_uint8;

	stream->user_callback(stream->user_data, output_buffer, frames_to_do);

	// Handle volume in software, since SDL2's API doesn't have volume control
	if (stream->volume != 1.0f)
		for (unsigned long i = 0; i < frames_to_do * CLOWNAUDIO_STREAM_CHANNEL_COUNT; ++i)
			output_buffer[i] *= stream->volume;
}

CLOWNAUDIO_EXPORT bool ClownAudio_InitPlayback(void)
{
	bool success = true;

	sdl_already_init = SDL_WasInit(SDL_INIT_AUDIO);

	if (!sdl_already_init)
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
			success = false;

	return success;
}

CLOWNAUDIO_EXPORT void ClownAudio_DeinitPlayback(void)
{
	if (!sdl_already_init)
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

CLOWNAUDIO_EXPORT ClownAudio_Stream* ClownAudio_CreateStream(void (*user_callback)(void*, float*, size_t), void *user_data)
{
	ClownAudio_Stream *stream = (ClownAudio_Stream*)malloc(sizeof(ClownAudio_Stream));

	if (stream != NULL)
	{
		SDL_AudioSpec want;
		memset(&want, 0, sizeof(want));
		want.freq = CLOWNAUDIO_STREAM_SAMPLE_RATE;
		want.format = AUDIO_F32;
		want.channels = CLOWNAUDIO_STREAM_CHANNEL_COUNT;
		want.samples = NextPowerOfTwo(((CLOWNAUDIO_STREAM_SAMPLE_RATE * 10) / 1000) * CLOWNAUDIO_STREAM_CHANNEL_COUNT);	// A low-latency buffer of 10 milliseconds
		want.callback = Callback;
		want.userdata = stream;

		SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);

		if (device > 0)
		{
			stream->user_callback = user_callback;
			stream->user_data = user_data;

			stream->device = device;
			stream->volume = 1.0f;

			return stream;
		}

		free(stream);
	}

	return NULL;
}

CLOWNAUDIO_EXPORT bool ClownAudio_DestroyStream(ClownAudio_Stream *stream)
{
	if (stream != NULL)
	{
		SDL_CloseAudioDevice(stream->device);
		free(stream);
	}

	return true;
}

CLOWNAUDIO_EXPORT bool ClownAudio_SetStreamVolume(ClownAudio_Stream *stream, float volume)
{
	if (stream != NULL)
		stream->volume = volume * volume;

	return true;
}

CLOWNAUDIO_EXPORT bool ClownAudio_PauseStream(ClownAudio_Stream *stream)
{
	if (stream != NULL)
		SDL_PauseAudioDevice(stream->device, -1);

	return true;
}

CLOWNAUDIO_EXPORT bool ClownAudio_ResumeStream(ClownAudio_Stream *stream)
{
	if (stream != NULL)
		SDL_PauseAudioDevice(stream->device, 0);

	return true;
}