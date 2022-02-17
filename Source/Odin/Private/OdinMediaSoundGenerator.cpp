#include "OdinMediaSoundGenerator.h"

#include "OdinLibrary/include/odin.h"

OdinMediaSoundGenerator::OdinMediaSoundGenerator() = default;

int32 OdinMediaSoundGenerator::OnGenerateAudio(float *OutAudio, int32 NumSamples)
{
    if (stream_ == nullptr) {
        return 0;
    }
    return odin_audio_read_data(stream_, OutAudio, NumSamples, OdinChannelLayout_Mono);
}

void OdinMediaSoundGenerator::SetOdinStream(OdinMediaStream *stream)
{
    stream_ = stream;
}

void OdinMediaSoundGenerator::OnBeginGenerate() {}

void OdinMediaSoundGenerator::OnEndGenerate() {}