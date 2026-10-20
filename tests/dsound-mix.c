/* DirectSound mixing load, as a game with many sounds puts on Wine's dsound: plays a number of
 * secondary buffers at a rate other than the device's, so that dsound has to resample every one,
 * and measures whether its mixer keeps up. It reports how fast the play cursor of a probe buffer
 * moves against the clock (100 % is real time, below that the device runs dry, and the sound
 * stutters and falls behind) and the CPU time of the process, nearly all of it the mixer thread
 * wine_dsound_mixer when no busy threads run (Wine has no CPU times of single threads on macOS).
 * All buffers hold silence, so nothing can be heard. Busy threads, as a game has them, can run
 * next to the mixer at normal priority.
 * Usage: dsound-mix [buffers] [rate] [seconds] [busy threads]   (defaults: 64 buffers at 44100 Hz
 * for 10 s, no busy threads; rate 0 takes 22050, 32000 and 44100 Hz in turn).
 * Exit code 1 if the cursor ran below 98 %.
 * Build: i686-w64-mingw32-gcc -O2 -o dsound-mix32.exe dsound-mix.c -ldsound -ldxguid -lole32
 *        x86_64-w64-mingw32-gcc -O2 -o dsound-mix.exe dsound-mix.c -ldsound -ldxguid -lole32 */
#define COBJMACROS
#define INITGUID
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <dsound.h>
#include <stdio.h>
#include <stdlib.h>

static double process_seconds(void)
{
    FILETIME c, e, k, u;

    if (!GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u)) return 0;
    return (((ULONGLONG)k.dwHighDateTime << 32 | k.dwLowDateTime) + ((ULONGLONG)u.dwHighDateTime << 32 | u.dwLowDateTime)) / 1e7;
}

static void print_device_format(void)
{
    IMMDeviceEnumerator *en = NULL;
    IMMDevice *dev = NULL;
    IAudioClient *client = NULL;
    WAVEFORMATEX *fmt = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **)&en))
        && SUCCEEDED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(en, eRender, eConsole, &dev))
        && SUCCEEDED(IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&client))
        && SUCCEEDED(IAudioClient_GetMixFormat(client, &fmt)))
        printf("device: %lu Hz, %u channels\n", fmt->nSamplesPerSec, fmt->nChannels);
    else
        printf("device: format unknown\n");
}

static volatile LONG stop_busy;

static DWORD WINAPI busy_thread(void *arg)
{
    volatile float x = 1.0f;

    while (!stop_busy) x = x * 1.0001f + 0.5f;
    return 0;
}

static IDirectSoundBuffer *make_buffer(IDirectSound8 *ds, DWORD rate, WORD channels)
{
    WAVEFORMATEX fmt = { WAVE_FORMAT_PCM, channels, rate, rate * channels * 2, channels * 2, 16, 0 };
    DSBUFFERDESC desc = { sizeof(desc) };
    IDirectSoundBuffer *buf = NULL;
    void *p1, *p2;
    DWORD n1, n2;

    desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY | DSBCAPS_GETCURRENTPOSITION2
                 | DSBCAPS_GLOBALFOCUS;
    if (channels == 2) desc.dwFlags &= ~DSBCAPS_CTRLPAN;
    desc.dwBufferBytes = fmt.nAvgBytesPerSec; /* one second */
    desc.lpwfxFormat = &fmt;
    if (FAILED(IDirectSound8_CreateSoundBuffer(ds, &desc, &buf, NULL))) return NULL;
    if (SUCCEEDED(IDirectSoundBuffer_Lock(buf, 0, 0, &p1, &n1, &p2, &n2, DSBLOCK_ENTIREBUFFER)))
    {
        memset(p1, 0, n1);
        IDirectSoundBuffer_Unlock(buf, p1, n1, p2, n2);
    }
    return buf;
}

int main(int argc, char **argv)
{
    static const DWORD rates[] = { 22050, 32000, 44100 };
    int count = argc > 1 ? atoi(argv[1]) : 64;
    DWORD rate = argc > 2 ? atoi(argv[2]) : 44100;
    double seconds = argc > 3 ? atof(argv[3]) : 10;
    int busy = argc > 4 ? atoi(argv[4]) : 0;
    IDirectSound8 *ds = NULL;
    IDirectSoundBuffer *probe, **bufs;
    LARGE_INTEGER freq, t0, t1, now, last_move;
    DWORD pos, last, probe_len, moved = 0, max_stall_ms = 0, stalls = 0;
    double cpu0, cpu1, cursor, elapsed;
    char rate_text[32] = "22050/32000/44100";
    int i;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    print_device_format();
    if (FAILED(DirectSoundCreate8(NULL, &ds, NULL))) { printf("DirectSoundCreate8 failed\n"); return 2; }
    IDirectSound8_SetCooperativeLevel(ds, GetDesktopWindow(), DSSCL_PRIORITY);

    bufs = calloc(count, sizeof(*bufs));
    for (i = 0; i < count; i++)
    {
        /* mostly mono, as most sounds of a game; every fourth in stereo */
        if (!(bufs[i] = make_buffer(ds, rate ? rate : rates[i % 3], i % 4 ? 1 : 2))) { printf("buffer %d failed\n", i); return 2; }
        IDirectSoundBuffer_Play(bufs[i], 0, 0, DSBPLAY_LOOPING);
    }
    if (!(probe = make_buffer(ds, 44100, 1))) { printf("probe buffer failed\n"); return 2; }
    probe_len = 44100 * 2;
    IDirectSoundBuffer_Play(probe, 0, 0, DSBPLAY_LOOPING);

    for (i = 0; i < busy; i++) CloseHandle(CreateThread(NULL, 0, busy_thread, NULL, 0, NULL));
    Sleep(1000); /* let the mixer settle */
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    last_move = t0;
    cpu0 = process_seconds();
    IDirectSoundBuffer_GetCurrentPosition(probe, &last, NULL);
    do
    {
        Sleep(2);
        IDirectSoundBuffer_GetCurrentPosition(probe, &pos, NULL);
        QueryPerformanceCounter(&now);
        if (pos != last)
        {
            DWORD stall = (now.QuadPart - last_move.QuadPart) * 1000 / freq.QuadPart;
            if (stall > max_stall_ms) max_stall_ms = stall;
            if (stall >= 30) stalls++;
            moved += (pos + probe_len - last) % probe_len;
            last = pos;
            last_move = now;
        }
    } while ((now.QuadPart - t0.QuadPart) < seconds * freq.QuadPart);
    t1 = now;
    cpu1 = process_seconds();
    stop_busy = 1;

    elapsed = (double)(t1.QuadPart - t0.QuadPart) / freq.QuadPart;
    cursor = moved / 2.0 / 44100 / elapsed;
    if (rate) sprintf(rate_text, "%lu", rate);
    printf("%s, %d buffers at %s Hz, %d busy threads, %.1f s: play cursor at %.1f %% of real time, "
           "longest stall %lu ms, stalls of 30 ms or more: %lu, process %.0f %% CPU\n",
           sizeof(void *) == 4 ? "32-bit" : "64-bit", count, rate_text, busy, elapsed, 100 * cursor,
           max_stall_ms, stalls, 100 * (cpu1 - cpu0) / elapsed);

    for (i = 0; i < count; i++) IDirectSoundBuffer_Release(bufs[i]);
    IDirectSoundBuffer_Release(probe);
    IDirectSound8_Release(ds);
    return cursor < 0.98;
}
