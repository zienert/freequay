/* d3d9-cpu: the CPU cost of a Direct3D 9 frame, as a game with many objects puts it on Wine's d3d9
 * and wined3d. Every frame draws a grid of small cubes with one draw call each and sets for every
 * cube what a game sets for an object: shader constants (vs_3_0 and ps_3_0), a texture out of four,
 * and for every eighth cube blending, depth writes, culling and texture addressing. Four times a
 * frame it fills a dynamic vertex buffer through Lock and Unlock and draws it with the fixed-function
 * pipeline, as games draw particles and text. The window is small and the cubes are a few pixels
 * wide, so the GPU waits for the CPU, and the frame time is what d3d9, wined3d and the OpenGL calls
 * cost on the CPU. Presents without vsync.
 * Modes: shader (default); ff, the fixed-function pipeline with the world transform per cube, two
 * lights, a material and fog; d3dx, the shader mode with the matrices of every cube built by D3DX
 * each frame (D3DXMatrixRotationYawPitchRoll, Translation, Multiply, Transpose), as many games do.
 * Usage: d3d9-cpu [shader|ff|d3dx] [draws per frame] [seconds]   (defaults: shader 1000 10)
 * Prints the frames per second, the time per frame and the CPU of the process; then draws one more
 * frame at a fixed time, reads it back, saves it as d3d9-cpu-<mode>-<bits>.bmp and prints a hash of
 * the image, to compare builds.
 * Build: i686-w64-mingw32-gcc -O2 -o d3d9-cpu32.exe d3d9-cpu.c -ld3d9 -ld3dx9_43 -ld3dcompiler_47
 *        x86_64-w64-mingw32-gcc -O2 -o d3d9-cpu.exe d3d9-cpu.c -ld3d9 -ld3dx9_43 -ld3dcompiler_47 */
#define COBJMACROS
#include <windows.h>
#include <d3d9.h>
#include <d3dx9math.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 240
#define COLS 40
#define BACKGROUND D3DCOLOR_XRGB(51, 153, 255)

enum mode { MODE_SHADER, MODE_FF, MODE_D3DX };

struct vertex { float x, y, z, nx, ny, nz, u, v; };
struct tvertex { float x, y, z, rhw; DWORD color; };

struct object
{
    D3DXMATRIX world, wvp_t, world_t; /* the transposed ones go into the shader constants */
    float x, y, yaw, pitch;
    float color[4];
};

static const char shader[] =
    "float4x4 wvp : register(c0);\n"
    "float4x4 world : register(c4);\n"
    "float4 light : register(c8);\n"
    "struct vs_in { float4 pos : POSITION; float3 n : NORMAL; float2 uv : TEXCOORD0; };\n"
    "struct vs_out { float4 pos : POSITION; float2 uv : TEXCOORD0; float shade : TEXCOORD1; };\n"
    "vs_out vs(vs_in i)\n"
    "{\n"
    "    vs_out o;\n"
    "    float3 n = normalize(mul(i.n, (float3x3)world));\n"
    "    o.pos = mul(i.pos, wvp);\n"
    "    o.uv = i.uv;\n"
    "    o.shade = 0.3 + 0.7 * saturate(dot(n, light.xyz));\n"
    "    return o;\n"
    "}\n"
    "sampler2D s0 : register(s0);\n"
    "float4 tint : register(c0);\n"
    "float4 ps(float2 uv : TEXCOORD0, float shade : TEXCOORD1) : COLOR\n"
    "{\n"
    "    return tex2D(s0, uv) * tint * shade;\n"
    "}\n";

static ID3DBlob *compile(const char *entry, const char *target)
{
    ID3DBlob *blob = NULL, *errors = NULL;
    HRESULT hr = D3DCompile(shader, sizeof(shader) - 1, "d3d9-cpu", NULL, NULL, entry, target, 0, 0, &blob, &errors);
    if (FAILED(hr))
    {
        printf("D3DCompile %s: hr=0x%08lx %s\n", entry, hr, errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "");
        return NULL;
    }
    return blob;
}

static DWORD float_bits(float f)
{
    DWORD d;

    memcpy(&d, &f, sizeof(d));
    return d;
}

static double process_seconds(void)
{
    FILETIME c, e, k, u;

    if (!GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u)) return 0;
    return (((ULONGLONG)k.dwHighDateTime << 32 | k.dwLowDateTime) + ((ULONGLONG)u.dwHighDateTime << 32 | u.dwLowDateTime)) / 1e7;
}

static void save_bmp(const char *path, const unsigned char *bgra, INT pitch)
{
    BITMAPFILEHEADER fh = {0};
    BITMAPINFOHEADER ih = {0};
    FILE *f = fopen(path, "wb");
    int y;

    if (!f) return;
    fh.bfType = 0x4d42;
    fh.bfOffBits = sizeof(fh) + sizeof(ih);
    fh.bfSize = fh.bfOffBits + W * H * 4;
    ih.biSize = sizeof(ih);
    ih.biWidth = W;
    ih.biHeight = -H;
    ih.biPlanes = 1;
    ih.biBitCount = 32;
    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);
    for (y = 0; y < H; y++) fwrite(bgra + y * pitch, W * 4, 1, f);
    fclose(f);
}

static void make_cube(struct vertex *v, WORD *idx)
{
    static const float n[6][3] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    int f, k;

    for (f = 0; f < 6; f++)
    {
        /* two axes across the face, and the order of the corners that is clockwise from outside */
        float a[3] = {1, 0, 0}, b[3] = {0, 1, 0};
        int flip;

        if (n[f][0] != 0) { a[0] = 0; a[2] = 1; }
        else if (n[f][1] != 0) { b[1] = 0; b[2] = 1; }
        flip = (a[1] * b[2] - a[2] * b[1]) * n[f][0] + (a[2] * b[0] - a[0] * b[2]) * n[f][1]
             + (a[0] * b[1] - a[1] * b[0]) * n[f][2] < 0;
        for (k = 0; k < 4; k++)
        {
            float s = k & 1 ? 0.5f : -0.5f, t = k & 2 ? 0.5f : -0.5f;
            struct vertex *p = &v[f * 4 + k];

            p->x = 0.2f * (0.5f * n[f][0] + s * a[0] + t * b[0]);
            p->y = 0.2f * (0.5f * n[f][1] + s * a[1] + t * b[1]);
            p->z = 0.2f * (0.5f * n[f][2] + s * a[2] + t * b[2]);
            p->nx = n[f][0]; p->ny = n[f][1]; p->nz = n[f][2];
            p->u = s + 0.5f; p->v = t + 0.5f;
        }
        idx[f * 6 + 0] = f * 4 + 0; idx[f * 6 + 1] = f * 4 + (flip ? 2 : 1); idx[f * 6 + 2] = f * 4 + (flip ? 1 : 2);
        idx[f * 6 + 3] = f * 4 + 2; idx[f * 6 + 4] = f * 4 + (flip ? 3 : 1); idx[f * 6 + 5] = f * 4 + (flip ? 1 : 3);
    }
}

/* the world matrix of a cube at time t, built by D3DX as a game builds it */
static void update_object(struct object *o, const D3DXMATRIX *viewproj, float t)
{
    D3DXMATRIX rot, trans, wvp;
    D3DXVECTOR3 center = {0, 0, 0}, out;

    D3DXMatrixRotationYawPitchRoll(&rot, o->yaw + t, o->pitch, 0.0f);
    D3DXMatrixTranslation(&trans, o->x, o->y, 0.0f);
    D3DXMatrixMultiply(&o->world, &rot, &trans);
    D3DXMatrixMultiply(&wvp, &o->world, viewproj);
    D3DXVec3TransformCoord(&out, &center, &wvp); /* as for a visibility test */
    D3DXMatrixTranspose(&o->wvp_t, &wvp);
    D3DXMatrixTranspose(&o->world_t, &o->world);
}

int main(int argc, char **argv)
{
    static const DWORD texels[4][4] =
    {
        {0xffffffff, 0xffc0c0c0, 0xffc0c0c0, 0xffffffff},
        {0xffff8080, 0xffffffff, 0xffffffff, 0xffff8080},
        {0xff80ff80, 0xff408040, 0xff408040, 0xff80ff80},
        {0xffffff80, 0xff808040, 0xff808040, 0xffffff80},
    };
    const char *mode_name = argc > 1 ? argv[1] : "shader";
    enum mode mode = !strcmp(mode_name, "ff") ? MODE_FF : !strcmp(mode_name, "d3dx") ? MODE_D3DX : MODE_SHADER;
    int draws = argc > 2 ? atoi(argv[2]) : 1000;
    double seconds = argc > 3 ? atof(argv[3]) : 10;
    int rows = (draws + COLS - 1) / COLS;
    HINSTANCE inst = GetModuleHandleW(NULL);
    WNDCLASSW wc = {0};
    D3DPRESENT_PARAMETERS pp = {0};
    D3DADAPTER_IDENTIFIER9 id;
    D3DLOCKED_RECT lr;
    D3DXMATRIX view, proj, viewproj;
    D3DXVECTOR3 eye = {0, 0, -10}, at = {0, 0, 0}, up = {0, 1, 0};
    D3DLIGHT9 light[2];
    D3DMATERIAL9 material;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *dev = NULL;
    IDirect3DTexture9 *tex[4];
    IDirect3DVertexBuffer9 *vb = NULL, *dyn = NULL;
    IDirect3DIndexBuffer9 *ib = NULL;
    IDirect3DVertexShader9 *vs = NULL;
    IDirect3DPixelShader9 *ps = NULL;
    IDirect3DSurface9 *back = NULL, *sysmem = NULL;
    struct tvertex *quads;
    struct object *obj;
    struct vertex cube[24];
    WORD cube_idx[36];
    LARGE_INTEGER freq, t0, now;
    double cpu0 = 0, cpu1, elapsed;
    char bmp[64];
    float fog_start = 8.0f, fog_end = 14.0f, t = 0;
    enum { PHASE_WARMUP, PHASE_MEASURE, PHASE_IMAGE } phase = PHASE_WARMUP;
    int frames = 0, background = 0, i, j, x, y;
    unsigned int hash = 2166136261u;
    void *p;
    HRESULT hr;
    HWND hwnd;

    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = inst;
    wc.lpszClassName = L"d3d9cpu";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"d3d9cpu", L"freequay d3d9 cpu", WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE,
                         100, 100, W, H, NULL, NULL, inst, NULL);

    if (!(d3d = Direct3DCreate9(D3D_SDK_VERSION))) { printf("Direct3DCreate9 failed\n"); return 1; }
    if (SUCCEEDED(IDirect3D9_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &id)))
        printf("adapter: %s\n", id.Description);
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = W;
    pp.BackBufferHeight = H;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.hDeviceWindow = hwnd;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                 D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &dev);
    if (FAILED(hr)) { printf("CreateDevice: hr=0x%08lx\n", hr); return 1; }

    if (mode != MODE_FF)
    {
        ID3DBlob *vsb, *psb;

        if (!(vsb = compile("vs", "vs_3_0")) || !(psb = compile("ps", "ps_3_0"))) return 1;
        if (FAILED(IDirect3DDevice9_CreateVertexShader(dev, ID3D10Blob_GetBufferPointer(vsb), &vs)) ||
            FAILED(IDirect3DDevice9_CreatePixelShader(dev, ID3D10Blob_GetBufferPointer(psb), &ps)))
        {
            printf("creating the shaders failed\n");
            return 1;
        }
    }

    for (i = 0; i < 4; i++)
    {
        hr = IDirect3DDevice9_CreateTexture(dev, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex[i], NULL);
        if (FAILED(hr) || FAILED(IDirect3DTexture9_LockRect(tex[i], 0, &lr, NULL, 0))) { printf("texture: hr=0x%08lx\n", hr); return 1; }
        for (j = 0; j < 4; j++) ((DWORD *)((BYTE *)lr.pBits + (j / 2) * lr.Pitch))[j % 2] = texels[i][j];
        IDirect3DTexture9_UnlockRect(tex[i], 0);
    }

    make_cube(cube, cube_idx);
    if (FAILED(IDirect3DDevice9_CreateVertexBuffer(dev, sizeof(cube), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &vb, NULL)) ||
        FAILED(IDirect3DVertexBuffer9_Lock(vb, 0, 0, &p, 0))) { printf("vertex buffer failed\n"); return 1; }
    memcpy(p, cube, sizeof(cube));
    IDirect3DVertexBuffer9_Unlock(vb);
    if (FAILED(IDirect3DDevice9_CreateIndexBuffer(dev, sizeof(cube_idx), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib, NULL)) ||
        FAILED(IDirect3DIndexBuffer9_Lock(ib, 0, 0, &p, 0))) { printf("index buffer failed\n"); return 1; }
    memcpy(p, cube_idx, sizeof(cube_idx));
    IDirect3DIndexBuffer9_Unlock(ib);

    /* 64 small pretransformed quads, as particles or text: written four times a frame */
    quads = malloc(64 * 6 * sizeof(*quads));
    for (i = 0; i < 64; i++)
    {
        float qx = 4.0f + (i % 16) * 19.0f, qy = H - 40.0f + (i / 16) * 9.0f;
        static const float corner[6][2] = {{0, 0}, {6, 0}, {0, 6}, {0, 6}, {6, 0}, {6, 6}};

        for (j = 0; j < 6; j++)
        {
            struct tvertex *q = &quads[i * 6 + j];
            q->x = qx + corner[j][0]; q->y = qy + corner[j][1]; q->z = 0.1f; q->rhw = 1.0f;
            q->color = 0xff000000 | (i * 0x030507);
        }
    }
    if (FAILED(IDirect3DDevice9_CreateVertexBuffer(dev, 64 * 6 * sizeof(*quads), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                                   D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &dyn, NULL)))
    { printf("dynamic vertex buffer failed\n"); return 1; }

    D3DXMatrixLookAtLH(&view, &eye, &at, &up);
    D3DXMatrixPerspectiveFovLH(&proj, 0.8f, (float)W / H, 1.0f, 100.0f);
    D3DXMatrixMultiply(&viewproj, &view, &proj);
    obj = calloc(draws, sizeof(*obj));
    for (i = 0; i < draws; i++)
    {
        struct object *o = &obj[i];

        o->x = -5.2f + 10.4f * (i % COLS) / (COLS - 1);
        o->y = 3.9f - 7.8f * (i / COLS) / (rows > 1 ? rows - 1 : 1);
        o->yaw = 0.37f * i;
        o->pitch = 0.21f * (i % 7);
        o->color[0] = 0.5f + 0.5f * (i % 3) / 2; o->color[1] = 0.5f + 0.5f * (i % 5) / 4;
        o->color[2] = 0.5f + 0.5f * (i % 7) / 6; o->color[3] = 0.75f;
        update_object(o, &viewproj, 0.0f);
    }

    memset(light, 0, sizeof(light));
    for (i = 0; i < 2; i++)
    {
        light[i].Type = D3DLIGHT_DIRECTIONAL;
        light[i].Diffuse.r = light[i].Diffuse.g = light[i].Diffuse.b = i ? 0.4f : 0.8f;
        light[i].Direction.x = i ? 0.6f : -0.4f; light[i].Direction.y = -0.5f; light[i].Direction.z = 0.7f;
    }
    memset(&material, 0, sizeof(material));
    material.Diffuse.a = 1.0f;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    for (;;)
    {
        static const float light_dir[4] = {0.4f, 0.5f, -0.7f, 0.0f};

        IDirect3DDevice9_Clear(dev, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, BACKGROUND, 1.0f, 0);
        IDirect3DDevice9_BeginScene(dev);
        IDirect3DDevice9_SetFVF(dev, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);
        IDirect3DDevice9_SetStreamSource(dev, 0, vb, 0, sizeof(struct vertex));
        IDirect3DDevice9_SetIndices(dev, ib);
        IDirect3DDevice9_SetRenderState(dev, D3DRS_ZENABLE, D3DZB_TRUE);
        IDirect3DDevice9_SetSamplerState(dev, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        IDirect3DDevice9_SetSamplerState(dev, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        IDirect3DDevice9_SetRenderState(dev, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        IDirect3DDevice9_SetRenderState(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        if (mode == MODE_FF)
        {
            IDirect3DDevice9_SetVertexShader(dev, NULL);
            IDirect3DDevice9_SetPixelShader(dev, NULL);
            IDirect3DDevice9_SetTransform(dev, D3DTS_VIEW, &view);
            IDirect3DDevice9_SetTransform(dev, D3DTS_PROJECTION, &proj);
            IDirect3DDevice9_SetRenderState(dev, D3DRS_LIGHTING, TRUE);
            IDirect3DDevice9_SetRenderState(dev, D3DRS_AMBIENT, D3DCOLOR_XRGB(60, 60, 60));
            for (i = 0; i < 2; i++)
            {
                IDirect3DDevice9_SetLight(dev, i, &light[i]);
                IDirect3DDevice9_LightEnable(dev, i, TRUE);
            }
            IDirect3DDevice9_SetRenderState(dev, D3DRS_FOGENABLE, TRUE);
            IDirect3DDevice9_SetRenderState(dev, D3DRS_FOGCOLOR, BACKGROUND);
            IDirect3DDevice9_SetRenderState(dev, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            IDirect3DDevice9_SetRenderState(dev, D3DRS_FOGSTART, float_bits(fog_start));
            IDirect3DDevice9_SetRenderState(dev, D3DRS_FOGEND, float_bits(fog_end));
            IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        }
        else
        {
            IDirect3DDevice9_SetVertexShader(dev, vs);
            IDirect3DDevice9_SetPixelShader(dev, ps);
            IDirect3DDevice9_SetVertexShaderConstantF(dev, 8, light_dir, 1);
        }

        for (i = 0; i < draws; i++)
        {
            struct object *o = &obj[i];

            if (mode == MODE_D3DX) update_object(o, &viewproj, t);
            if (mode == MODE_FF)
            {
                IDirect3DDevice9_SetTransform(dev, D3DTS_WORLD, &o->world);
                if (!(i & 15))
                {
                    material.Diffuse.r = material.Ambient.r = o->color[0];
                    material.Diffuse.g = material.Ambient.g = o->color[1];
                    material.Diffuse.b = material.Ambient.b = o->color[2];
                    IDirect3DDevice9_SetMaterial(dev, &material);
                }
            }
            else
            {
                IDirect3DDevice9_SetVertexShaderConstantF(dev, 0, (const float *)&o->wvp_t, 4);
                IDirect3DDevice9_SetVertexShaderConstantF(dev, 4, (const float *)&o->world_t, 4);
                IDirect3DDevice9_SetPixelShaderConstantF(dev, 0, o->color, 1);
            }
            IDirect3DDevice9_SetTexture(dev, 0, (IDirect3DBaseTexture9 *)tex[i & 3]);
            if (!(i & 7))
            {
                int k = i >> 3;
                IDirect3DDevice9_SetRenderState(dev, D3DRS_ALPHABLENDENABLE, k & 1);
                IDirect3DDevice9_SetRenderState(dev, D3DRS_ZWRITEENABLE, !(k & 1));
                IDirect3DDevice9_SetRenderState(dev, D3DRS_CULLMODE, k & 2 ? D3DCULL_NONE : D3DCULL_CCW);
                IDirect3DDevice9_SetSamplerState(dev, 0, D3DSAMP_ADDRESSU, k & 4 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
            }
            IDirect3DDevice9_DrawIndexedPrimitive(dev, D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
        }

        /* particles or text: pretransformed quads through the fixed-function pipeline */
        IDirect3DDevice9_SetVertexShader(dev, NULL);
        IDirect3DDevice9_SetPixelShader(dev, NULL);
        IDirect3DDevice9_SetTexture(dev, 0, NULL);
        IDirect3DDevice9_SetRenderState(dev, D3DRS_LIGHTING, FALSE);
        IDirect3DDevice9_SetRenderState(dev, D3DRS_FOGENABLE, FALSE);
        IDirect3DDevice9_SetRenderState(dev, D3DRS_ALPHABLENDENABLE, FALSE);
        IDirect3DDevice9_SetRenderState(dev, D3DRS_ZENABLE, D3DZB_FALSE);
        IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
        IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        IDirect3DDevice9_SetFVF(dev, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        IDirect3DDevice9_SetStreamSource(dev, 0, dyn, 0, sizeof(struct tvertex));
        for (i = 0; i < 4; i++)
        {
            if (FAILED(IDirect3DVertexBuffer9_Lock(dyn, 0, 0, &p, D3DLOCK_DISCARD))) { printf("Lock failed\n"); return 1; }
            memcpy(p, quads + i * 16 * 6, 16 * 6 * sizeof(*quads));
            IDirect3DVertexBuffer9_Unlock(dyn);
            IDirect3DDevice9_DrawPrimitive(dev, D3DPT_TRIANGLELIST, 0, 32);
        }
        IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        IDirect3DDevice9_EndScene(dev);

        if (phase == PHASE_IMAGE) break;
        if (FAILED(hr = IDirect3DDevice9_Present(dev, NULL, NULL, NULL, NULL))) { printf("Present: hr=0x%08lx\n", hr); return 1; }
        t += 0.01f;
        QueryPerformanceCounter(&now);
        if (phase == PHASE_WARMUP)
        {
            /* one second to warm up: wined3d compiles its shaders, Rosetta translates */
            if (now.QuadPart - t0.QuadPart < freq.QuadPart) continue;
            phase = PHASE_MEASURE;
            t0 = now;
            cpu0 = process_seconds();
            continue;
        }
        frames++;
        if (now.QuadPart - t0.QuadPart < seconds * freq.QuadPart) continue;
        cpu1 = process_seconds();
        elapsed = (double)(now.QuadPart - t0.QuadPart) / freq.QuadPart;
        printf("%s, %s, %d draws a frame, %.1f s: %.1f frames per second, %.2f ms per frame, process %.0f %% CPU\n",
               sizeof(void *) == 4 ? "32-bit" : "64-bit", mode_name, draws, elapsed, frames / elapsed,
               1000 * elapsed / frames, 100 * (cpu1 - cpu0) / elapsed);
        /* one more frame at a fixed time for the image */
        t = 0;
        if (mode == MODE_D3DX) for (i = 0; i < draws; i++) update_object(&obj[i], &viewproj, t);
        phase = PHASE_IMAGE;
    }

    IDirect3DDevice9_GetBackBuffer(dev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &back);
    IDirect3DDevice9_CreateOffscreenPlainSurface(dev, W, H, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &sysmem, NULL);
    hr = IDirect3DDevice9_GetRenderTargetData(dev, back, sysmem);
    if (FAILED(hr) || FAILED(IDirect3DSurface9_LockRect(sysmem, &lr, NULL, D3DLOCK_READONLY)))
    {
        printf("readback: hr=0x%08lx\n", hr);
        return 1;
    }
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
        {
            DWORD c = ((const DWORD *)((const BYTE *)lr.pBits + y * lr.Pitch))[x] & 0xffffff;
            if (c == (BACKGROUND & 0xffffff)) background++;
            for (j = 0; j < 3; j++) hash = (hash ^ ((c >> (8 * j)) & 0xff)) * 16777619u;
        }
    sprintf(bmp, "d3d9-cpu-%s-%d.bmp", mode_name, sizeof(void *) == 4 ? 32 : 64);
    save_bmp(bmp, lr.pBits, lr.Pitch);
    IDirect3DSurface9_UnlockRect(sysmem);
    printf("image: hash %08x, %d of %d pixels background\n", hash, background, W * H);

    IDirect3DSurface9_Release(sysmem);
    IDirect3DSurface9_Release(back);
    IDirect3DVertexBuffer9_Release(dyn);
    IDirect3DVertexBuffer9_Release(vb);
    IDirect3DIndexBuffer9_Release(ib);
    for (i = 0; i < 4; i++) IDirect3DTexture9_Release(tex[i]);
    if (ps) IDirect3DPixelShader9_Release(ps);
    if (vs) IDirect3DVertexShader9_Release(vs);
    IDirect3DDevice9_Release(dev);
    IDirect3D9_Release(d3d);
    DestroyWindow(hwnd);
    /* a good image has cubes over at least a tenth of the window and background around them */
    return background < W * H / 10 || background > W * H * 9 / 10;
}
