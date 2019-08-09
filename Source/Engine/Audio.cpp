#include "Precomp.h"
#include "Audio.h"

#include <Core/Transform.h>

#ifdef USE_AUDIO
float Audio::ourVolume = 0.2f;
vector<ALuint> Audio::ourBuffers;
vector<Audio::AudioSource> Audio::ourSources;
tbb::concurrent_queue<Audio::AudioCommand> Audio::ourPlayCommands;
ALint Audio::ourMusicTrack = -1;
Audio::AudioSource Audio::ourBgSource;

// quick shortcut for error checking utility
#define CHECK_ERROR() CheckError(__LINE__)

void Audio::Init(int* anArgc /* =nullptr */, char** anArgv /* =nullptr */)
{
	alutInit(anArgc, anArgv);
	CHECK_ERROR();

	// load our audios
	for (const string& name : AudioFiles)
	{
		const string fullName = "../assets/audio/" + name;
		ourBuffers.push_back(alutCreateBufferFromFile(fullName.c_str()));
		CHECK_ERROR();
	}

	ourBgSource = { 0, 0, AL_STOPPED };
	alGenSources(1, &ourBgSource.mySource);
}

void Audio::Play(uint32_t anAudioInd, glm::vec3 aPos)
{
	// enqueue a play command
	ourPlayCommands.push({ anAudioInd, aPos });
}

// TODO: need more private utility methods to play/stop audio, and set volume
void Audio::PlayQueue(Transform& aTransf)
{
	CheckSources();

	// updating player related state
	glm::vec3 pos = aTransf.GetPos();
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
	CHECK_ERROR();

	glm::vec3 forward = aTransf.GetForward();
	glm::vec3 up = aTransf.GetUp();
	ALfloat alFacing[6] { forward.x, forward.y, forward.z, up.x, up.y, up.z };
	alListenerfv(AL_ORIENTATION, alFacing);
	CHECK_ERROR();

	// background music
	if (ourMusicTrack != -1)
	{
		// update it's position to camera's
		alSource3f(ourBgSource.mySource, AL_POSITION, pos.x, pos.y, pos.z);
		alSourcef(ourBgSource.mySource, AL_GAIN, ourVolume);

		// if it was switched - update it
		ALuint buffer = ourBuffers[ourMusicTrack];
		if (buffer != ourBgSource.myBoundBuffer)
		{
			alSourceStop(ourBgSource.mySource);
			ourBgSource.myState = AL_STOPPED;
			ourBgSource.myBoundBuffer = buffer;
			alSourcei(ourBgSource.mySource, AL_BUFFER, ourBgSource.myBoundBuffer);
		}
		// we'll do manual looping
		alGetSourcei(ourBgSource.mySource, AL_SOURCE_STATE, &ourBgSource.myState);
		if (ourBgSource.myState != AL_PLAYING)
		{
			alSourcePlay(ourBgSource.mySource);
		}
	}

	// TODO: refactor to avoid constantly calling to change volume
	// updating volume
	for (const AudioSource& src : ourSources)
	{
		if (src.myState == AL_PLAYING)
		{
			alSourcef(src.mySource, AL_GAIN, ourVolume);
		}
	}

	AudioCommand cmd;
	while (ourPlayCommands.try_pop(cmd))
	{
		// TODO: revisit this lookup loop
		// find our buffer
		bool found = false;
		for (AudioSource& src : ourSources)
		{
			if (cmd.myAudioInd == src.myBoundBuffer && src.myState == AL_STOPPED)
			{
				alSource3f(src.mySource, AL_POSITION, cmd.myPos.x, cmd.myPos.y, cmd.myPos.z);
				alSourcef(src.mySource, AL_GAIN, ourVolume);
				alSourcePlay(src.mySource);
				src.mySource = AL_PLAYING;
				found = true;
				CHECK_ERROR();
			}
		}
		// we didn't find an available source to play it out, so create a new one and use it
		if (!found)
		{
			AudioSource src;
			src.myBoundBuffer = ourBuffers[cmd.myAudioInd];
			alGenSources(1, &src.myBoundBuffer);
			alSourcei(src.mySource, AL_BUFFER, src.myBoundBuffer);
			alSourcef(src.mySource, AL_GAIN, ourVolume);

			alSource3f(src.mySource, AL_POSITION, cmd.myPos.x, cmd.myPos.y, cmd.myPos.z);
			alSourcePlay(src.mySource);
			src.myState = AL_PLAYING;

			ourSources.push_back(src);
			CHECK_ERROR();
		}
	}
}

void Audio::Cleanup()
{
	// first the sources, then the buffers
	for (AudioSource& src : ourSources)
	{
		alDeleteSources(1, &src.mySource);
	}

	// then all the buffers
	alDeleteBuffers(static_cast<ALsizei>(ourBuffers.size()), ourBuffers.data());
	CHECK_ERROR();
}

void Audio::CheckSources()
{
	// reset the state to signal that we can reuse it
	for (AudioSource& source : ourSources)
	{
		if (source.myState == AL_PLAYING)
		{
			alGetSourcei(source.mySource, AL_SOURCE_STATE, &source.myState);
		}
	}
	CHECK_ERROR();
}

void Audio::CheckError(uint32_t aLine)
{
	ALenum error = alutGetError();
	if (error != ALUT_ERROR_NO_ERROR)
	{
		printf("[Error] ALUT error at %d: %s\n", aLine, alutGetErrorString(error));
	}
}
#endif