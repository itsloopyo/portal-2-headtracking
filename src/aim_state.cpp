#include "aim_state.h"

#include "plugin.h"

namespace headtracking {

namespace {

AimState g_state[Plugin::kMaxPlayers];
int g_currentSlot = 0;

}  // namespace

void PublishAimState(int slot, const AimState& state) {
    if (slot < 0 || slot >= Plugin::kMaxPlayers) return;
    g_state[slot] = state;
    g_currentSlot = slot;
}

const AimState& CurrentAimState() {
    return g_state[g_currentSlot];
}

}  // namespace headtracking
