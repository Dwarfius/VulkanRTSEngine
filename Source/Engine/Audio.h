#pragma once

class Transform;

const vector<string> audioFiles{
	"explosion.wav",
	"tankShot.wav",
	"arcadeMusic.wav"
};

class Audio
{
public:
	static void Init(int *argc = nullptr, char **argv = nullptr);
	static void Play(uint32_t audioInd, glm::vec3 pos);
	static void PlayQueue(Transform *transf);
	static void SetMusicTrack(ALint track) { musicTrack = track; }
	static void Cleanup();

	static void IncreaseVolume() { if (volume <= 0.9f) volume += 0.1f; }
	static void DecreaseVolume() { if (volume >= 0.1f) volume -= 0.1f; }

private:
	static float volume;

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
		glm::vec3 pos;
	};
	static tbb::concurrent_queue<AudioCommand> playCommands;
};