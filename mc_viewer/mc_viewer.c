#include "cunk/graphics.h"
#include "cunk/math.h"
#include "cunk/print.h"
#include "cunk/memory.h"
#include "cunk/io.h"
#include "cunk/camera.h"

#include "enklume.h"
#include "nbt.h"

#include "chunk.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <assert.h>

static Window* window;
static GfxCtx* ctx;
static GfxShader* shader;

GLFWwindow* gfx_get_glfw_handle(Window*);

struct {
    bool wireframe, face_culling, depth_testing;
    bool finish, vsync;
    int render_mode;
} config = {
    .depth_testing = true,
};
typedef struct {
    GfxBuffer* buf;
    size_t num_verts;
    int x, y, z;
} ChunkMesh;

static float frand() {
    int r = rand();
    double rd = (double) r;
    rd /= (double) RAND_MAX;
    return ((float) rd);
}

static void init_chunk(Chunk* chunk) {
    for (int i = 0; i < 16; i+=8)
        for (int j = 0; j < 16; j+=8)
            for (int k = 0; k < 64; k+=8) {
                chunk_set_block_data(chunk, i, k, j, 1);
                assert(chunk_get_block_data(chunk, i, k, j) == 1);
            }
}

static ChunkMesh* update_chunk_mesh(const Chunk* chunk, ChunkMesh* mesh) {
    if (mesh) {
        gfx_destroy_buffer(mesh->buf);
    } else if (chunk) {
        mesh = malloc(sizeof(ChunkMesh));
        mesh->x = chunk->x;
        mesh->y = chunk->y;
        mesh->z = chunk->z;
    } else return NULL;

    Growy* g = cunk_new_growy();
    chunk_mesh(chunk, NULL, g, &mesh->num_verts);

    fprintf(stderr, "%zu vertices, totalling %zu KiB of data\n", mesh->num_verts, mesh->num_verts * sizeof(float) * 36 * 5 / 1024);
    fflush(stderr);

    size_t buffer_size = cunk_growy_size(g);
    char* buffer = cunk_growy_deconstruct(g);

    mesh->buf = gfx_create_buffer(ctx, buffer_size);
    gfx_copy_to_buffer(mesh->buf, buffer, buffer_size);
    free(buffer);
    return mesh;
}

static void key_callback(GLFWwindow* handle, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    switch (key) {
        case GLFW_KEY_1:
            config.render_mode ^= 1;
            break;
        case GLFW_KEY_2:
            config.wireframe ^= true;
            break;
        case GLFW_KEY_3:
            config.face_culling ^= true;
            break;
        case GLFW_KEY_4:
            config.depth_testing ^= true;
            break;
        case GLFW_KEY_5:
            config.finish ^= true;
            break;
        case GLFW_KEY_6:
            config.vsync ^= true;
            break;

        default: break;
    }
}

#define WORLD_SIZE 16
typedef struct {
    Chunk chunk;
    ChunkMesh* mesh;
} WorldChunk;

WorldChunk world[WORLD_SIZE][WORLD_SIZE];

void load_from_mcchunk(Chunk* dst_chunk, McChunk* chunk);

void load_map(const char* map_dir) {
    Printer* p = cunk_open_file_as_printer(stdout);

    McWorld* w = cunk_open_mcworld(map_dir);
    assert(w);

    for (int rx = 0; rx < 2; rx++) for (int rz = 0; rz < 2; rz++) {
        McRegion* r = cunk_open_mcregion(w,  0+ rx, 0 + rz);
        if (!r)
            continue;
        for (int rcx = 0; rcx < 32; rcx++) for (int rcz = 0; rcz < 32; rcz++) {
            int dcx = rx * 32 + rcx;
            int dcz = rz * 32 + rcz;
            if (dcx >= 0 && dcz >= 0 && dcx < WORLD_SIZE && dcz < WORLD_SIZE) {
                McChunk* c = cunk_open_mcchunk(r, rcx, rcz);
                if (c)
                    load_from_mcchunk(&world[dcx][dcz].chunk, c);
            }
        }
    }
}

static Camera camera = {
    .position = { .x = (WORLD_SIZE * CUNK_CHUNK_SIZE) / 2, .y = 64 + 5, .z = (WORLD_SIZE * CUNK_CHUNK_SIZE) / 2 },
    .rotation = {
        .yaw = M_PI * 0.5f * 1.5f, .pitch = 0
    },
    .fov = 90,
};
static CameraFreelookState camera_state = {
    .fly_speed = 0.25f,
    .mouse_sensitivity = 2.0f
};

static void draw_chunks() {
    gfx_cmd_resize_viewport(ctx, window);
    gfx_cmd_clear(ctx);
    GfxState state = {
        .wireframe = config.wireframe,
        .face_culling = config.face_culling,
        .depth_testing = config.depth_testing,
    };
    gfx_cmd_set_draw_state(ctx, state);
    gfx_cmd_use_shader(ctx, shader);

    Mat4f matrix = identity_mat4f;
    size_t width, height;
    gfx_get_window_size(window, &width, &height);
    matrix = mul_mat4f(camera_get_view_mat4(&camera, width, height), matrix);

    gfx_cmd_set_shader_extern(ctx, "myMatrix", &matrix.arr);
    gfx_cmd_set_shader_extern(ctx, "render_mode", &config.render_mode);

    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            WorldChunk wc = world[z][x];
            if (!wc.mesh)
                continue;
            ChunkMesh* mesh = wc.mesh;
            if (mesh->num_verts == 0)
                continue;

            int chunk_position[3] = { mesh->x, mesh->y, mesh->z };
            gfx_cmd_set_shader_extern(ctx, "chunk_position", &chunk_position);

            gfx_cmd_set_vertex_input(ctx, "vertexIn", mesh->buf, 3, sizeof(float) * 5, 0);
            gfx_cmd_set_vertex_input(ctx, "texCoordIn", mesh->buf, 2, sizeof(float) * 5, sizeof(float) * 3);

            assert(mesh->num_verts > 0);
            gfx_cmd_draw_arrays(ctx, 0, 36 * mesh->num_verts);
        }
    }
}

#define SMOOTH_FPS_ACC_FRAMES 32
static double last_frames_times[SMOOTH_FPS_ACC_FRAMES] = { 0 };
static int frame = 0;

int main(int argc, char* argv[]) {
    window = gfx_create_window("Hello", 640, 480, &ctx);
    glfwSetKeyCallback(gfx_get_glfw_handle(window), key_callback);

    char* test_vs, *test_fs;
    size_t test_vs_size, test_fs_size;
    read_file("../shaders/mc_viewer.vs", &test_vs_size, &test_vs);
    read_file("../shaders/mc_viewer.fs", &test_fs_size, &test_fs);
    shader = gfx_create_shader(ctx, test_vs, test_fs);

    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            WorldChunk* wc = &world[z][x];
            wc->chunk.x = x;
            wc->chunk.z = z;
            // init_chunk(&wc->chunk);
            // wc->mesh = update_chunk_mesh(&wc->chunk, wc->mesh);
        }
    }

    load_map(argv[1]);

    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            WorldChunk* wc = &world[z][x];
            wc->mesh = update_chunk_mesh(&wc->chunk, wc->mesh);
        }
    }

    fflush(stdout);
    fflush(stderr);

    glfwSwapInterval(config.vsync ? 1 : 0);

    while (!glfwWindowShouldClose(gfx_get_glfw_handle(window))) {
        double then = glfwGetTime();
        camera_move_freelook(&camera, window, &camera_state);

        draw_chunks();

        glfwSwapBuffers(gfx_get_glfw_handle(window));
        if (config.finish)
            gfx_wait_for_idle();

        double now = glfwGetTime();
        double frametime = 0.0;
        int available_frames = frame > SMOOTH_FPS_ACC_FRAMES ? SMOOTH_FPS_ACC_FRAMES : frame;
        int j = frame % SMOOTH_FPS_ACC_FRAMES;
        for (int i = 0; i < available_frames; i++) {
            double frame_start = last_frames_times[j];
            j = (j + 1) % SMOOTH_FPS_ACC_FRAMES;
            double frame_end;
            if (i + 1 == available_frames)
                frame_end = now;
            else
                frame_end = last_frames_times[j];
            double delta = frame_end - frame_start;
            frametime += delta;
        }

        frametime /= (double) available_frames;

        double fps = 1.0 / frametime;
        int ifps = (int) fps;

        const char* t = format_string("FPS: %d", ifps);
        glfwSetWindowTitle(gfx_get_glfw_handle(window), t);
        free(t);

        last_frames_times[frame % SMOOTH_FPS_ACC_FRAMES] = now;
        frame++;

        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
