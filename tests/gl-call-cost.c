/* gl-call-cost: what one OpenGL call costs a Windows program on Wine, 32-bit against 64-bit.
 * Every OpenGL function of opengl32.dll is a thunk that hands its arguments to the Unix side of
 * Wine, where the Mac's OpenGL runs. For a 32-bit program the call also switches the processor
 * from 32-bit to 64-bit mode and back (WoW64), which Rosetta has to emulate. wined3d makes about
 * eight such calls per draw. The program times millions of calls of a few cheap functions:
 *   switch      32-bit only: a far call into 64-bit mode and straight back, as Wine's WoW64 thunk
 *               does it under Rosetta (lcall, lretq), without any of Wine's code
 *   syscall     NtQueryPerformanceCounter, a Wine syscall with little work on the Unix side
 *   no context  glDisable without a current context: thunk, switch and Wine's dispatcher, but
 *               no OpenGL work
 *   glDisable, glGetError, glActiveTexture, glBindTexture, glBlendColor, glUniform4fv
 *               with a current context, the calls wined3d makes for every draw
 * Prints nanoseconds per call, the best of three rounds. With patch 0011 a 32-bit program collects
 * glDisable, glActiveTexture, glBindTexture, glBlendColor and glUniform4fv and hands them over in
 * one switch with a later call, so their numbers there are what collecting costs; glGetError
 * still switches every time.
 * Usage: gl-call-cost [millions of calls per function]   (default 2)
 * Build: i686-w64-mingw32-gcc -O2 -o gl-call-cost32.exe gl-call-cost.c -lopengl32 -lgdi32
 *        x86_64-w64-mingw32-gcc -O2 -o gl-call-cost.exe gl-call-cost.c -lopengl32 -lgdi32 */
#include <windows.h>
#include <winternl.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GL_TEXTURE0 0x84c0
#define GL_FRAGMENT_SHADER 0x8b30
#define GL_VERTEX_SHADER 0x8b31

typedef void (WINAPI *PFN_ACTIVETEXTURE)(GLenum);
typedef void (WINAPI *PFN_BLENDCOLOR)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *PFN_UNIFORM4FV)(GLint, GLsizei, const GLfloat *);
typedef GLuint (WINAPI *PFN_CREATESHADER)(GLenum);
typedef void (WINAPI *PFN_SHADERSOURCE)(GLuint, GLsizei, const char *const *, const GLint *);
typedef void (WINAPI *PFN_COMPILESHADER)(GLuint);
typedef GLuint (WINAPI *PFN_CREATEPROGRAM)(void);
typedef void (WINAPI *PFN_ATTACHSHADER)(GLuint, GLuint);
typedef void (WINAPI *PFN_LINKPROGRAM)(GLuint);
typedef void (WINAPI *PFN_USEPROGRAM)(GLuint);
typedef GLint (WINAPI *PFN_GETUNIFORMLOCATION)(GLuint, const char *);
typedef NTSTATUS (WINAPI *PFN_NTQPC)(LARGE_INTEGER *, LARGE_INTEGER *);

static PFN_ACTIVETEXTURE p_glActiveTexture;
static PFN_BLENDCOLOR p_glBlendColor;
static PFN_UNIFORM4FV p_glUniform4fv;
static PFN_NTQPC p_NtQueryPerformanceCounter;
static GLuint textures[2];
static GLint uniform = -1;
static const GLfloat vec[4] = {0.25f, 0.5f, 0.75f, 1.0f};

#ifndef _WIN64
/* a far call into 64-bit mode to code that returns at once, as Wine's WoW64 thunk under Rosetta
 * enters and leaves 64-bit mode (dlls/wow64cpu/cpu.c, CW HACK 20760) */
static const BYTE stub64[] =
{
    0x89, 0xe4,                   /* mov %esp,%esp: the upper half of rsp is not set in 32-bit mode */
    0x8b, 0x14, 0x24,             /* mov (%rsp),%edx: eip of the caller */
    0x8b, 0x4c, 0x24, 0x04,       /* mov 4(%rsp),%ecx: its code segment */
    0x48, 0x83, 0xec, 0x08,       /* sub $8,%rsp */
    0x48, 0x89, 0x14, 0x24,       /* mov %rdx,(%rsp) */
    0x48, 0x89, 0x4c, 0x24, 0x08, /* mov %rcx,8(%rsp) */
    0x48, 0xcb,                   /* lretq */
};
static struct { DWORD offset; WORD selector; } __attribute__((packed)) far_target;

static void call_switch(void)
{
    __asm__ volatile( "lcall *%0" :: "m"(far_target) : "edx", "ecx", "memory" );
}

/* the 64-bit code segment, from the thunk ntdll's __wine_unix_call_dispatcher points to: under
 * Rosetta it starts with lcall *addr, and addr holds offset and selector */
static BOOL find_cs64(void)
{
    BYTE **dispatcher = (BYTE **)GetProcAddress(GetModuleHandleA("ntdll.dll"), "__wine_unix_call_dispatcher");
    BYTE *thunk;
    WORD *sel;

    if (!dispatcher || !(thunk = *dispatcher)) return FALSE;
    if (thunk[0] != 0xff || (thunk[1] != 0x1d && thunk[1] != 0x2d)) return FALSE;
    sel = (WORD *)(*(BYTE **)(thunk + 2) + 4);
    far_target.selector = *sel;
    return TRUE;
}
#endif

static void call_syscall(void)
{
    LARGE_INTEGER counter;
    p_NtQueryPerformanceCounter(&counter, NULL);
}
static void call_disable(void) { glDisable(GL_DITHER); }
static void call_geterror(void) { glGetError(); }
static void call_activetexture(void) { p_glActiveTexture(GL_TEXTURE0); }
static void call_bindtexture(void)
{
    static unsigned int i;
    glBindTexture(GL_TEXTURE_2D, textures[++i & 1]);
}
static void call_blendcolor(void) { p_glBlendColor(0.25f, 0.5f, 0.75f, 1.0f); }
static void call_uniform(void) { p_glUniform4fv(uniform, 1, vec); }

static double ns_per_call(void (*func)(void), unsigned int calls)
{
    LARGE_INTEGER freq, t0, t1;
    double best = 1e30;
    unsigned int i;
    int round;

    QueryPerformanceFrequency(&freq);
    for (round = 0; round < 3; round++)
    {
        QueryPerformanceCounter(&t0);
        for (i = 0; i < calls; i++) func();
        QueryPerformanceCounter(&t1);
        if ((double)(t1.QuadPart - t0.QuadPart) / freq.QuadPart * 1e9 / calls < best)
            best = (double)(t1.QuadPart - t0.QuadPart) / freq.QuadPart * 1e9 / calls;
    }
    return best;
}

static GLuint make_program(void)
{
    static const char *vs = "#version 120\nuniform vec4 tint;\nvoid main() { gl_Position = gl_Vertex * tint; }\n";
    static const char *fs = "#version 120\nuniform vec4 tint;\nvoid main() { gl_FragColor = tint; }\n";
    PFN_CREATESHADER create_shader = (PFN_CREATESHADER)wglGetProcAddress("glCreateShader");
    PFN_SHADERSOURCE shader_source = (PFN_SHADERSOURCE)wglGetProcAddress("glShaderSource");
    PFN_COMPILESHADER compile_shader = (PFN_COMPILESHADER)wglGetProcAddress("glCompileShader");
    PFN_CREATEPROGRAM create_program = (PFN_CREATEPROGRAM)wglGetProcAddress("glCreateProgram");
    PFN_ATTACHSHADER attach_shader = (PFN_ATTACHSHADER)wglGetProcAddress("glAttachShader");
    PFN_LINKPROGRAM link_program = (PFN_LINKPROGRAM)wglGetProcAddress("glLinkProgram");
    PFN_USEPROGRAM use_program = (PFN_USEPROGRAM)wglGetProcAddress("glUseProgram");
    PFN_GETUNIFORMLOCATION get_uniform = (PFN_GETUNIFORMLOCATION)wglGetProcAddress("glGetUniformLocation");
    GLuint v, f, program;

    if (!create_shader || !shader_source || !compile_shader || !create_program || !attach_shader ||
        !link_program || !use_program || !get_uniform) return 0;
    v = create_shader(GL_VERTEX_SHADER);
    shader_source(v, 1, &vs, NULL);
    compile_shader(v);
    f = create_shader(GL_FRAGMENT_SHADER);
    shader_source(f, 1, &fs, NULL);
    compile_shader(f);
    program = create_program();
    attach_shader(program, v);
    attach_shader(program, f);
    link_program(program);
    use_program(program);
    uniform = get_uniform(program, "tint");
    return program;
}

int main(int argc, char **argv)
{
    unsigned int calls = (unsigned int)((argc > 1 ? atof(argv[1]) : 2) * 1e6);
    PIXELFORMATDESCRIPTOR pfd = {0};
    HINSTANCE inst = GetModuleHandleW(NULL);
    const char *bits = sizeof(void *) == 4 ? "32-bit" : "64-bit";
    WNDCLASSW wc = {0};
    HGLRC ctx;
    HWND hwnd;
    HDC hdc;
    int format;

    p_NtQueryPerformanceCounter = (PFN_NTQPC)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryPerformanceCounter");
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = inst;
    wc.lpszClassName = L"glcallcost";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"glcallcost", L"freequay gl call cost", WS_OVERLAPPEDWINDOW, 100, 100, 160, 120,
                         NULL, NULL, inst, NULL);
    hdc = GetDC(hwnd);
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    if (!(format = ChoosePixelFormat(hdc, &pfd)) || !SetPixelFormat(hdc, format, &pfd) ||
        !(ctx = wglCreateContext(hdc)))
    {
        printf("no OpenGL context\n");
        return 1;
    }

#ifndef _WIN64
    {
        void *mem = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        if (mem && find_cs64())
        {
            memcpy(mem, stub64, sizeof(stub64));
            far_target.offset = (DWORD)mem;
            printf("%s, switch to 64-bit mode and back (selector %#x): %.1f ns\n", bits,
                   far_target.selector, ns_per_call(call_switch, calls));
        }
        else printf("%s, switch: no Rosetta thunk found\n", bits);
    }
#endif
    printf("%s, syscall NtQueryPerformanceCounter: %.1f ns\n", bits, ns_per_call(call_syscall, calls));
    printf("%s, glDisable without a context: %.1f ns\n", bits, ns_per_call(call_disable, calls));

    wglMakeCurrent(hdc, ctx);
    p_glActiveTexture = (PFN_ACTIVETEXTURE)wglGetProcAddress("glActiveTexture");
    p_glBlendColor = (PFN_BLENDCOLOR)wglGetProcAddress("glBlendColor");
    p_glUniform4fv = (PFN_UNIFORM4FV)wglGetProcAddress("glUniform4fv");
    glGenTextures(2, textures);
    printf("%s, glDisable: %.1f ns\n", bits, ns_per_call(call_disable, calls));
    printf("%s, glGetError: %.1f ns\n", bits, ns_per_call(call_geterror, calls));
    if (p_glActiveTexture) printf("%s, glActiveTexture: %.1f ns\n", bits, ns_per_call(call_activetexture, calls));
    printf("%s, glBindTexture: %.1f ns\n", bits, ns_per_call(call_bindtexture, calls));
    if (p_glBlendColor) printf("%s, glBlendColor: %.1f ns\n", bits, ns_per_call(call_blendcolor, calls));
    if (p_glUniform4fv && make_program() && uniform >= 0)
        printf("%s, glUniform4fv: %.1f ns\n", bits, ns_per_call(call_uniform, calls));
    if (glGetError() != GL_NO_ERROR) printf("%s, OpenGL reported an error\n", bits);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(ctx);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    return 0;
}
