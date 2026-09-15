// Deterministic clocked output device for tests only. Production links real PortAudio.
#include "fakeportaudio.h"
#include <portaudio.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace {
std::atomic_bool deviceAvailable{true};
struct TestStream {
    PaStreamCallback *callback = nullptr;
    void *context = nullptr;
    std::atomic_bool active{false};
    std::atomic_bool stopped{true};
    std::thread thread;
    unsigned long frames = 512;
    int channels = 2;
    double rate = 48000;
};
void stop(TestStream *stream)
{
    stream->active = false;
    if (stream->thread.joinable()) stream->thread.join();
    stream->stopped = true;
}
}
void FakeAudio::setAvailable(bool available) { deviceAvailable = available; }
extern "C" {
PaError Pa_Initialize() { return paNoError; }
PaError Pa_Terminate() { return paNoError; }
const char *Pa_GetErrorText(PaError) { return "test output error"; }
PaDeviceIndex Pa_GetDefaultOutputDevice() { return deviceAvailable ? 0 : paNoDevice; }
const PaDeviceInfo *Pa_GetDeviceInfo(PaDeviceIndex index)
{
    static PaDeviceInfo info{2,"Test clocked sink",0,0,2,0,0.01,0,0.1,48000};
    return index == 0 && deviceAvailable ? &info : nullptr;
}
PaError Pa_IsFormatSupported(const PaStreamParameters *,const PaStreamParameters *output,double rate)
{
    return output->channelCount == 2 && output->sampleFormat == paFloat32 && rate == 48000 ? paNoError : paInvalidSampleRate;
}
PaError Pa_OpenStream(PaStream **result,const PaStreamParameters *,const PaStreamParameters *output,
                      double rate,unsigned long frames,PaStreamFlags,PaStreamCallback *callback,void *context)
{
    auto *stream = new TestStream;
    stream->callback=callback; stream->context=context; stream->frames=frames; stream->channels=output->channelCount; stream->rate=rate;
    *result=stream;
    return paNoError;
}
PaError Pa_StartStream(PaStream *opaque)
{
    auto *stream=static_cast<TestStream *>(opaque);
    if (!stream->stopped) return paStreamIsNotStopped;
    if (stream->thread.joinable()) stream->thread.join();
    stream->stopped=false; stream->active=true;
    stream->thread=std::thread([stream] {
        std::vector<float> output(stream->frames*stream->channels);
        PaStreamCallbackTimeInfo time{};
        auto deadline=std::chrono::steady_clock::now();
        while (stream->active) {
            const int result=stream->callback(nullptr,output.data(),stream->frames,&time,0,stream->context);
            deadline += std::chrono::microseconds(static_cast<long>(stream->frames*1000000/stream->rate));
            std::this_thread::sleep_until(deadline);
            if (result != paContinue) stream->active=false;
        }
    });
    return paNoError;
}
PaError Pa_StopStream(PaStream *stream) { stop(static_cast<TestStream *>(stream)); return paNoError; }
PaError Pa_AbortStream(PaStream *stream) { return Pa_StopStream(stream); }
PaError Pa_CloseStream(PaStream *stream) { auto *value=static_cast<TestStream *>(stream); stop(value); delete value; return paNoError; }
PaError Pa_IsStreamStopped(PaStream *stream) { return static_cast<TestStream *>(stream)->stopped ? 1 : 0; }
PaError Pa_IsStreamActive(PaStream *stream) { return static_cast<TestStream *>(stream)->active ? 1 : 0; }
}
