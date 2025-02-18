#include "Platform/GX.h"

#include <string.h> // memcpy, memset

#define GLASS_GX_MAX_COUNT 16

typedef struct {
    GXCmd cmd;
    GXCallback_t callback;
    void* param;
    Barrier* barrier;
} CmdEntry;

typedef struct {
    LightLock mtx;
    CmdEntry commands[GLASS_GX_MAX_COUNT];
    size_t count;
    size_t index;
} CmdQueue;

static CmdQueue g_Queue;

static __attribute((constructor)) void GLASS_initGXQueue(void) {
    LightLock_Init(&g_Queue.mtx);
    memset(g_Queue.commands, 0, sizeof(CmdEntry) * GLASS_GX_MAX_COUNT);
    g_Queue.count = 0;
    g_Queue.index = 0;
}

static void GLASS_submitNextCommand(void) { gspSubmitGxCommand(g_Queue.commands[g_Queue.index].cmd.data); }

static void GLASS_onGXCommandCompleted(void) {
    CmdEntry last;

    LightLock_Lock(&g_Queue.mtx);

    ASSERT(g_Queue.count);

    // Copy current command.
    memcpy(&last, &g_Queue.commands[g_Queue.index], sizeof(CmdEntry));

    // Pop command from queue.
    g_Queue.index = (g_Queue.index + 1) % GLASS_GX_MAX_COUNT;
    --g_Queue.count;

    // Process next command.
    if (g_Queue.count)
        GLASS_submitNextCommand();

    LightLock_Unlock(&g_Queue.mtx);

    // Release barrier.
    if (last.barrier)
        GLASS_barrier_release(last.barrier);

    // Invoke callback.
    if (last.callback)
        last.callback(last.param);
}

void gxCmdQueueInterrupt(GSPGPU_Event irq) {
    // Ignore vblank.
    if (irq == GSPGPU_EVENT_VBlank0 || irq == GSPGPU_EVENT_VBlank1)
        return;

    GLASS_onGXCommandCompleted();
}

void GLASS_gx_runSync(GXCmd* cmd) {
    ASSERT(cmd);

    GSPGPU_Event event;
    switch (cmd->data[0]) {
        case GLASS_GX_CMD_PROCESS_COMMAND_LIST:
            event = GSPGPU_EVENT_P3D;
            break;
        case GLASS_GX_CMD_MEMORY_FILL:
            event = (cmd->data[0] == 0) ? GSPGPU_EVENT_PSC1 : GSPGPU_EVENT_PSC0;
            break;
        case GLASS_GX_CMD_DISPLAY_TRANSFER:
        case GLASS_GX_CMD_TEXTURE_COPY:
            event = GSPGPU_EVENT_PPF;
            break;
        default:
            UNREACHABLE("Invalid GX command!");
    }

    LightLock_Lock(&g_Queue.mtx);
    ASSERT(R_SUCCEEDED(gspSubmitGxCommand(cmd->data)));
    LightLock_Unlock(&g_Queue.mtx);
    gspWaitForEvent(event, false);
}

void GLASS_gx_runAsync(GXCmd* cmd, GXCallback_t callback, void* param, Barrier* barrier) {
    LightLock_Lock(&g_Queue.mtx);

    // Push command to queue.
    if (g_Queue.count >= GLASS_GX_MAX_COUNT) {
        UNREACHABLE("Exceeded maximum number of queued GX commands!");
    }

    CmdEntry* entry = &g_Queue.commands[(g_Queue.count + g_Queue.index) % GLASS_GX_MAX_COUNT];
    memcpy(&entry->cmd, cmd, sizeof(GXCmd));
    entry->callback = callback;
    entry->param = param;
    entry->barrier = barrier;

    ++g_Queue.count;

    // Acquire barrier.
    if (entry->barrier)
        GLASS_barrier_acquire(entry->barrier);

    // If this is the first command, kickstart processing.
    if (g_Queue.count == 1)
        GLASS_submitNextCommand();

    LightLock_Unlock(&g_Queue.mtx);
}