#include "kitezh/shift.hpp"
#include "kitezh/lock.hpp"

namespace kitezh {

ShiftReport runNightShift(const std::vector<Vessel>& queue) {
    ShiftReport report;
    std::vector<std::string> seen;
    std::vector<Vessel> waiting;
    for (const Vessel& vessel : queue) {
        bool duplicate = false;
        for (const std::string& sign : seen) {
            if (sign == vessel.call_sign) duplicate = true;
        }
        seen.push_back(vessel.call_sign);
        if (duplicate) report.refused.push_back({vessel.call_sign, "duplicate"});
        else if (validateVessel(vessel)) report.refused.push_back({vessel.call_sign, "invalid"});
        else if (vessel.draft_dm > kLowDepthDm) report.refused.push_back({vessel.call_sign, "draft"});
        else waiting.push_back(vessel);
    }

    Lock lock;
    while (!waiting.empty()) {
        std::vector<Vessel> batch;
        batch.push_back(waiting.front());
        waiting.erase(waiting.begin());
        bool skipped = false;
        if (batch.front().kind != VesselKind::Oversized) {
            for (std::size_t i = 0; i < waiting.size();) {
                const Vessel& vessel = waiting[i];
                if (vessel.direction == batch.front().direction &&
                    vessel.kind != VesselKind::Oversized && fitsInChamber(batch, vessel) &&
                    (!skipped || vessel.kind == VesselKind::Ambulance)) {
                    batch.push_back(vessel);
                    waiting.erase(waiting.begin() + i);
                } else {
                    skipped = true;
                    ++i;
                }
            }
        }

        bool up = batch.front().direction == Direction::Upstream;
        Level entry_level = up ? Level::Low : Level::High;
        Gate entry_gate = up ? Gate::Lower : Gate::Upper;
        Gate exit_gate = up ? Gate::Upper : Gate::Lower;
        if (lock.level() != entry_level) lock.pump();
        lock.open(entry_gate);
        for (const Vessel& vessel : batch) lock.enter(vessel);
        lock.close(entry_gate);
        lock.pump();
        lock.open(exit_gate);
        for (const Vessel& vessel : batch) {
            lock.exit(vessel.call_sign);
            report.passed.push_back(vessel.call_sign);
        }
        lock.close(exit_gate);
    }
    report.pump_cycles = lock.pumpCycles();
    return report;
}

}  // namespace kitezh
