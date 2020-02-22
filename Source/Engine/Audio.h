#pragma once

class Transform;

const std::vector<std::string> AudioFiles 
{
	"explosion.wav",
	"tankShot.wav",
	"arcadeMusic.wav"
};

#ifdef USE_AUDIO
// TODO: need to review Audio for refactoring - should be singleton instead of a full-static class
class Audio
{
public:
	static void Init(int* anArgc = nullptr, char** anArgv = nullptr);
	static void Play(uint32_t anAudioInd, glm::vec3 aPos);
	/*	TODO: need to refactor Transform to get rid of lazy-caching
		so I can properly pass it around as const& */
	static void PlayQueue(Transform& aTransf);
	static void SetMusicTrack(ALint aTrack) { ourMusicTrack = aTrack; }
	static void Cleanup();

	static void IncreaseVolume() { if (ourVolume <= 0.9f) ourVolume += 0.1f; }
	static void DecreaseVolume() { if (ourVolume >= 0.1f) ourVolume -= 0.1f; }

private:
	static float ourVolume;

	static void CheckSources();
	static void CheckError(uint32_t aLine);

	static ALint ourMusicTrack;

	static vector<ALuint> ourBuffers;
	struct AudioSource {
		ALuint mySource;
		ALuint myBoundBuffer;
		ALint myState;
	};
	static vector<AudioSource> ourSources;
	static AudioSource ourBgSource;

	struct AudioCommand {
		uint32_t myAudioInd;
		glm::vec3 myPos;
	};
	static tbb::concurrent_queue<AudioCommand> ourPlayCommands;
};
#else
class Audio
{
public:
	static void Init(int* anArgc = nullptr, char** anArgv = nullptr) {}
	static void Play(uint32_t anAudioInd, glm::vec3 aPos) { }
	/*	TODO: need to refactor Transform to get rid of lazy-caching
		so I can properly pass it around as const& */
	static void PlayQueue(Transform& aTransf) { }
	static void SetMusicTrack(int aTrack) { }
	static void Cleanup() { }

	static void IncreaseVolume() {}
	static void DecreaseVolume() {}
};
#endif // ifdef USE_AUDIO