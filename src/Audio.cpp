#include "Common.h"
#include "Audio.h"
#include "Transform.h"

float Audio::volume = 0.2f;
vector<ALuint> Audio::buffers;
vector<Audio::AudioSource> Audio::sources;
tbb::concurrent_queue<Audio::AudioCommand> Audio::playCommands;
ALint Audio::musicTrack = -1;
Audio::AudioSource Audio::bgSource;

void Audio::Init(int *argc, char **argv)
{
	alutInit(argc, argv);
	CheckError(__LINE__);

	// load our audios
	for (string name : audioFiles)
	{
		string fullName = "assets/audio/" + name;
		buffers.push_back(alutCreateBufferFromFile(fullName.c_str()));
		CheckError(__LINE__);
	}

	bgSource = { 0, 0, AL_STOPPED };
	alGenSources(1, &bgSource.source);
	
}

void Audio::Play(uint32_t audioInd, vec3 pos)
{
	// enqueue a play command
	playCommands.push({ audioInd, pos });
}

void Audio::PlayQueue(Transform *transf)
{
	CheckSources();

	// updating player related state
	vec3 pos = transf->GetPos();
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
	CheckError(__LINE__);

	vec3 forward = transf->GetForward();
	vec3 up = transf->GetUp();
	ALfloat alFacing[6] { forward.x, forward.y, forward.z, up.x, up.y, up.z };
	alListenerfv(AL_ORIENTATION, alFacing);
	CheckError(__LINE__);

	// background music
	if (musicTrack != -1)
	{
		// update it's position to camera's
		alSource3f(bgSource.source, AL_POSITION, pos.x, pos.y, pos.z);
		alSourcef(bgSource.source, AL_GAIN, volume);

		// if it was switched - update it
		ALuint buffer = buffers[musicTrack];
		if (buffer != bgSource.boundBuffer)
		{
			alSourceStop(bgSource.source);
			bgSource.state = AL_STOPPED;
			bgSource.boundBuffer = buffer;
			alSourcei(bgSource.source, AL_BUFFER, bgSource.boundBuffer);
		}
		// we'll do manual looping
		alGetSourcei(bgSource.source, AL_SOURCE_STATE, &bgSource.state);
		if (bgSource.state != AL_PLAYING)
			alSourcePlay(bgSource.source);
	}

	// updating volume
	for (AudioSource src : sources)
		if(src.state == AL_PLAYING)
			alSourcef(src.source, AL_GAIN, volume);

	AudioCommand cmd;
	while (playCommands.try_pop(cmd))
	{
		// find our buffer
		bool found = false;
		for (AudioSource &src : sources)
		{
			if (cmd.audioInd == src.boundBuffer && src.state == AL_STOPPED)
			{
				alSource3f(src.source, AL_POSITION, cmd.pos.x, cmd.pos.y, cmd.pos.z);
				alSourcef(src.source, AL_GAIN, volume);
				alSourcePlay(src.source);
				src.state = AL_PLAYING;
				found = true;
				CheckError(__LINE__);
			}
		}
		// we didn't find an available source to play it out, so create a new one and use it
		if (!found)
		{
			AudioSource src;
			src.boundBuffer = buffers[cmd.audioInd];
			alGenSources(1, &src.source);
			alSourcei(src.source, AL_BUFFER, src.boundBuffer);
			alSourcef(src.source, AL_GAIN, volume);

			alSource3f(src.source, AL_POSITION, cmd.pos.x, cmd.pos.y, cmd.pos.z);
			alSourcePlay(src.source);
			src.state = AL_PLAYING;

			sources.push_back(src);
			CheckError(__LINE__);
		}
	}
}

void Audio::Cleanup()
{
	// first the sources, then the buffers
	for (AudioSource src : sources)
		alDeleteSources(1, &src.source);

	// then all the buffers
	alDeleteBuffers(buffers.size(), buffers.data());
	CheckError(__LINE__);
}

void Audio::CheckSources()
{
	// reset the state to signal that we can reuse it
	for (AudioSource source : sources)
	{
		if (source.state == AL_PLAYING)
			alGetSourcei(source.source, AL_SOURCE_STATE, &source.state);
	}
	CheckError(__LINE__);
}

void Audio::CheckError(uint32_t line)
{
	ALenum error = alutGetError();
	if (error != ALUT_ERROR_NO_ERROR)
		printf("[Error] ALUT error at %d: %s\n", line, alutGetErrorString(error));
}