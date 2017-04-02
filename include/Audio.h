#ifndef _AUDIO_H
#define _AUDIO_H

#include "Transform.h"
#include <AL/alut.h>
#include <vector>
#include <tbb\concurrent_queue.h>

using namespace std;

const vector<string> audioFiles{
	"explosion.wav",
	"tankShot.wav",
	"arcadeMusic.wav"
};

class Audio
{
public:
	static void Init(int *argc = nullptr, char **argv = nullptr);
	static void Play(uint32_t audioInd, vec3 pos);
	static void PlayQueue(Transform *transf);
	static void SetMusicTrack(ALint track) { musicTrack = track; }
	static void Cleanup();

private:
	static void CheckSources();
	static void CheckError(uint32_t line);

	static ALint musicTrack;

	static vector<ALuint> buffers;
	struct AudioSource {
		ALuint source;
		ALuint boundBuffer;
		ALint state;
	};
	static vector<AudioSource> sources;
	static AudioSource bgSource;

	struct AudioCommand {
		uint32_t audioInd;
		vec3 pos;
	};
	static tbb::concurrent_queue<AudioCommand> playCommands;
};

#endif