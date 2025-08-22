#include "OdinPlaybackStreamReader.h"
#include "Odin.h"
#include "OdinAudioGeneratorLoopbackComponent.h"

FOdinAudioRingBuffer::FOdinAudioRingBuffer(const int32 InCapacity)
    : Capacity(InCapacity)
    , WriteIndex(0)
{
    Buffer.Init(0, Capacity);
}

int32 FOdinAudioRingBuffer::Write(const float* InData, const int32 NumSamples)
{
    if (NumSamples > Capacity) {
        UE_LOG(Odin, Error,
               TEXT("Tried Writing more Samples than Capacity in Buffer from FAudioRingBuffer, "
                    "aborting Write."));
        return WriteIndex.GetValue();
    }

    FScopeLock  WriteLock(&BufferSection);
    const int32 CurrentWriteIndex = WriteIndex.GetValue();

    const int32 NumFirstCopy = FMath::Min(NumSamples, Capacity - CurrentWriteIndex);
    FMemory::Memcpy(Buffer.GetData() + CurrentWriteIndex, InData, sizeof(float) * NumFirstCopy);

    if (NumFirstCopy < NumSamples) {
        const int32 NumSecondCopy = NumSamples - NumFirstCopy;
        FMemory::Memcpy(Buffer.GetData(), InData + NumFirstCopy, sizeof(float) * NumSecondCopy);
    }

    const int32 NewWriteIndex = (CurrentWriteIndex + NumSamples) % Capacity;
    WriteIndex.Set((CurrentWriteIndex + NumSamples) % Capacity);
    return NewWriteIndex;
}

int32 FOdinAudioRingBuffer::Read(const int32 ReaderIndex, float* OutData, const int32 NumSamples)
{
    if (NumSamples > Capacity) {
        UE_LOG(Odin, Error,
               TEXT("Tried Reading more Samples than available in Buffer from FAudioRingBuffer, "
                    "aborting Write."));
        return ReaderIndex;
    }

    if (ReaderIndex >= Capacity) {
        UE_LOG(Odin, Error,
               TEXT("Invalid ReaderIndex when trying to read data from FAudioRingBuffer"));
        return ReaderIndex;
    }

    FScopeLock ReadLock(&BufferSection);

    int32 NumFirstCopy = FMath::Min(NumSamples, Capacity - ReaderIndex);
    FMemory::Memcpy(OutData, Buffer.GetData() + ReaderIndex, sizeof(float) * NumFirstCopy);

    if (NumFirstCopy < Capacity) {
        int32 NumSecondCopy = NumSamples - NumFirstCopy;
        FMemory::Memcpy(OutData + NumFirstCopy, Buffer.GetData(), sizeof(float) * NumSecondCopy);
    }
    return (ReaderIndex + NumSamples) % Capacity;
}

int32 FOdinAudioRingBuffer::GetAvailableSamples(const int32 ReaderIndex) const
{
    if (ReaderIndex < 0 || ReaderIndex >= Capacity) {
        UE_LOG(Odin, Error, TEXT("Invalid ReaderIndex when trying to get available sample count."));
        return 0;
    }
    const int32 WriteSnap        = WriteIndex.GetValue();
    int32       AvailableSamples = WriteSnap - ReaderIndex;
    if (AvailableSamples < 0) {
        AvailableSamples += Capacity;
    }
    return AvailableSamples;
}

int32 FOdinAudioRingBuffer::GetCurrentWriteIndex() const
{
    return WriteIndex.GetValue();
}

FOdinPlaybackStreamReader::FOdinPlaybackStreamReader(OdinMediaStreamHandle MediaHandle,
                                                     int32                 BufferCapacity)
    : MediaHandle(MediaHandle)
{
    MultipleAccessCache = MakeShared<FOdinAudioRingBuffer, ESPMode::ThreadSafe>(BufferCapacity);
}

FOdinPlaybackStreamReader::~FOdinPlaybackStreamReader()
{
    FScopeLock LockListeners(&BufferListenerAccessSection);
    AudioBufferListeners.Empty();
}

OdinReturnCode FOdinPlaybackStreamReader::ReadData(int32& RefReaderIndex, float* OutAudio,
                                                   int32 NumSamples)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinPlaybackStreamReader::ReadData)
    bool bCanReadFromCache;
    {
        FScopeLock  ReadFromOdinLock(&ReadFromOdinSection);
        const int32 AvailableSamples = MultipleAccessCache->GetAvailableSamples(RefReaderIndex);
        bCanReadFromCache            = AvailableSamples >= NumSamples;
        if (!bCanReadFromCache) {
            TRACE_CPUPROFILER_EVENT_SCOPE(FOdinPlaybackStreamReader::ReadData
                                          - odin_audio_read_data call)
            CachedReturnCode = odin_audio_read_data(MediaHandle, OutAudio, NumSamples);
            if (!odin_is_error(CachedReturnCode)) {
                LatestReadIndex.Set(RefReaderIndex);
                RefReaderIndex = MultipleAccessCache->Write(OutAudio, NumSamples);
                FScopeLock LockListeners(&BufferListenerAccessSection);
                for (IAudioBufferListener* AudioBufferListener : AudioBufferListeners) {
                    if (AudioBufferListener) {
                        AudioBufferListener->OnGeneratedBuffer(OutAudio, NumSamples,
                                                               ODIN_DEFAULT_CHANNEL_COUNT);
                    }
                }
            }
        }
    }
    if (bCanReadFromCache) {
        TRACE_CPUPROFILER_EVENT_SCOPE(FOdinPlaybackStreamReader::ReadData - cached access call)
        RefReaderIndex = MultipleAccessCache->Read(RefReaderIndex, OutAudio, NumSamples);
    }
    return CachedReturnCode;
}

void FOdinPlaybackStreamReader::AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    if (nullptr != InAudioBufferListener) {
        FScopeLock LockListeners(&BufferListenerAccessSection);
        AudioBufferListeners.Add(InAudioBufferListener);
    }
}

void FOdinPlaybackStreamReader::RemoveAudioBufferListener(
    IAudioBufferListener* InAudioBufferListener)
{
    FScopeLock LockListeners(&BufferListenerAccessSection);
    AudioBufferListeners.Remove(InAudioBufferListener);
}

int32 FOdinPlaybackStreamReader::GetLatestReadIndex() const
{
    return LatestReadIndex.GetValue();
}
