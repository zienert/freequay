/* XAudio2 mixing load, as a game with many sounds puts it on Wine's XAudio2 (FAudio): plays a
 * number of source voices at a rate other than the device's, so that the engine has to resample
 * every one, and measures whether it keeps up. It reports how many samples a probe voice played
 * against the clock (100 % is real time, below that the device runs dry, and the sound stutters
 * and falls behind), the share of the time the engine spent mixing, from OnProcessingPassStart to
 * OnProcessingPassEnd, and the CPU time of the process. All voices play silence, so nothing can be
 * heard. With filter 1 every voice gets a low-pass filter, as games use for muffled sounds.
 * Usage: xaudio2-mix [voices] [rate] [seconds] [filter]   (defaults: 64 voices at 44100 Hz for 10 s,
 * no filter; three of four voices are mono; rate 0 takes 22050, 32000 and 44100 Hz in turn).
 * Exit code 1 if the probe ran below 98 %.
 * Build: i686-w64-mingw32-gcc -O2 -o xaudio2-mix32.exe xaudio2-mix.c -lxaudio2_9 -lole32
 *        x86_64-w64-mingw32-gcc -O2 -o xaudio2-mix.exe xaudio2-mix.c -lxaudio2_9 -lole32 */
#define COBJMACROS
#include <windows.h>
#include <xaudio2.h>
#include <stdio.h>
#include <stdlib.h>

static LARGE_INTEGER freq, pass_start;
static volatile LONGLONG mix_ticks;
static volatile LONG passes;

static void STDMETHODCALLTYPE pass_start_cb(IXAudio2EngineCallback *iface)
{
    QueryPerformanceCounter(&pass_start);
}

static void STDMETHODCALLTYPE pass_end_cb(IXAudio2EngineCallback *iface)
{
    LARGE_INTEGER now;

    QueryPerformanceCounter(&now);
    mix_ticks += now.QuadPart - pass_start.QuadPart;
    passes++;
}

static void STDMETHODCALLTYPE critical_error_cb(IXAudio2EngineCallback *iface, HRESULT error)
{
    printf("OnCriticalError: hr=0x%08lx\n", error);
}

static IXAudio2EngineCallbackVtbl engine_callback_vtbl = { pass_start_cb, pass_end_cb, critical_error_cb };
static IXAudio2EngineCallback engine_callback = { &engine_callback_vtbl };

static double process_seconds(void)
{
    FILETIME c, e, k, u;

    if (!GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u)) return 0;
    return (((ULONGLONG)k.dwHighDateTime << 32 | k.dwLowDateTime) + ((ULONGLONG)u.dwHighDateTime << 32 | u.dwLowDateTime)) / 1e7;
}

static IXAudio2SourceVoice *make_voice(IXAudio2 *xa, DWORD rate, WORD channels, int filter, BYTE *silence)
{
    WAVEFORMATEX fmt = { WAVE_FORMAT_PCM, channels, rate, rate * channels * 2, channels * 2, 16, 0 };
    XAUDIO2_FILTER_PARAMETERS lowpass = { LowPassFilter, 0.25f, 1.0f };
    XAUDIO2_BUFFER buf = {0};
    IXAudio2SourceVoice *voice = NULL;

    if (FAILED(IXAudio2_CreateSourceVoice(xa, &voice, &fmt, filter ? XAUDIO2_VOICE_USEFILTER : 0,
                                          XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL))) return NULL;
    if (filter) IXAudio2SourceVoice_SetFilterParameters(voice, &lowpass, XAUDIO2_COMMIT_NOW);
    buf.AudioBytes = fmt.nAvgBytesPerSec; /* one second */
    buf.pAudioData = silence;
    buf.LoopCount = XAUDIO2_LOOP_INFINITE;
    if (FAILED(IXAudio2SourceVoice_SubmitSourceBuffer(voice, &buf, NULL))) return NULL;
    IXAudio2SourceVoice_Start(voice, 0, XAUDIO2_COMMIT_NOW);
    return voice;
}

int main(int argc, char **argv)
{
    static const DWORD rates[] = { 22050, 32000, 44100 };
    int count = argc > 1 ? atoi(argv[1]) : 64;
    DWORD rate = argc > 2 ? atoi(argv[2]) : 44100;
    double seconds = argc > 3 ? atof(argv[3]) : 10;
    int filter = argc > 4 ? atoi(argv[4]) : 0;
    IXAudio2 *xa = NULL;
    IXAudio2MasteringVoice *master = NULL;
    IXAudio2SourceVoice *probe, **voices;
    XAUDIO2_VOICE_DETAILS details;
    XAUDIO2_VOICE_STATE state;
    LARGE_INTEGER t0, t1;
    UINT64 played0;
    LONGLONG ticks0;
    LONG passes0;
    double cpu0, cpu1, elapsed, cursor, mixing;
    char rate_text[32] = "22050/32000/44100";
    BYTE *silence;
    HRESULT hr;
    int i;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    QueryPerformanceFrequency(&freq);
    if (FAILED(hr = XAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR))) { printf("XAudio2Create: hr=0x%08lx\n", hr); return 2; }
    if (FAILED(hr = IXAudio2_CreateMasteringVoice(xa, &master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE,
                                                  0, NULL, NULL, AudioCategory_GameEffects)))
    { printf("CreateMasteringVoice: hr=0x%08lx\n", hr); return 2; }
    IXAudio2MasteringVoice_GetVoiceDetails(master, &details);
    printf("device: %u Hz, %u channels\n", details.InputSampleRate, details.InputChannels);
    IXAudio2_RegisterForCallbacks(xa, &engine_callback);

    silence = calloc(1, 44100 * 2 * 2);
    voices = calloc(count, sizeof(*voices));
    for (i = 0; i < count; i++)
    {
        /* mostly mono, as most sounds of a game; every fourth in stereo */
        if (!(voices[i] = make_voice(xa, rate ? rate : rates[i % 3], i % 4 ? 1 : 2, filter, silence)))
        { printf("voice %d failed\n", i); return 2; }
    }
    if (!(probe = make_voice(xa, 44100, 1, filter, silence))) { printf("probe voice failed\n"); return 2; }

    Sleep(1000); /* let the engine settle */
    QueryPerformanceCounter(&t0);
    cpu0 = process_seconds();
    IXAudio2SourceVoice_GetState(probe, &state, 0);
    played0 = state.SamplesPlayed;
    ticks0 = mix_ticks;
    passes0 = passes;
    Sleep((DWORD)(seconds * 1000));
    IXAudio2SourceVoice_GetState(probe, &state, 0);
    QueryPerformanceCounter(&t1);
    cpu1 = process_seconds();

    elapsed = (double)(t1.QuadPart - t0.QuadPart) / freq.QuadPart;
    cursor = (state.SamplesPlayed - played0) / 44100.0 / elapsed;
    mixing = (double)(mix_ticks - ticks0) / freq.QuadPart / elapsed;
    if (rate) sprintf(rate_text, "%lu", rate);
    printf("%s, %d voices at %s Hz%s, %.1f s: probe at %.1f %% of real time, engine mixing %.0f %% of the time "
           "in %ld passes, process %.0f %% CPU\n",
           sizeof(void *) == 4 ? "32-bit" : "64-bit", count, rate_text, filter ? " with low-pass filters" : "",
           elapsed, 100 * cursor, 100 * mixing, passes - passes0, 100 * (cpu1 - cpu0) / elapsed);

    IXAudio2_StopEngine(xa);
    for (i = 0; i < count; i++) IXAudio2SourceVoice_DestroyVoice(voices[i]);
    IXAudio2SourceVoice_DestroyVoice(probe);
    IXAudio2MasteringVoice_DestroyVoice(master);
    IXAudio2_Release(xa);
    return cursor < 0.98;
}
