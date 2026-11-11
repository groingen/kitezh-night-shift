#include "kitezh/lock.hpp"

namespace kitezh {

Lock::Lock() : level_(Level::Low), lower_(GateState::Closed),
               upper_(GateState::Closed), cycles_(0) {}

Level Lock::level() const { return level_; }
GateState Lock::gate(Gate gate) const { return gate == Gate::Lower ? lower_ : upper_; }
int Lock::pumpCycles() const { return cycles_; }
std::vector<Vessel> Lock::inside() const { return inside_; }

void Lock::open(Gate gate) {
    if ((gate == Gate::Lower && (level_ != Level::Low || upper_ == GateState::Open)) ||
        (gate == Gate::Upper && (level_ != Level::High || lower_ == GateState::Open))) {
        throw LockError("cannot open gate");
    }
    if (gate == Gate::Lower) lower_ = GateState::Open;
    else upper_ = GateState::Open;
}

void Lock::close(Gate gate) {
    if (gate == Gate::Lower) lower_ = GateState::Closed;
    else upper_ = GateState::Closed;
}

void Lock::pump() {
    if (lower_ == GateState::Open || upper_ == GateState::Open) {
        throw LockError("gate is open");
    }
    int depth = level_ == Level::Low ? kHighDepthDm : kLowDepthDm;
    for (const Vessel& vessel : inside_) {
        if (vessel.draft_dm > depth) throw LockError("not enough depth");
    }
    level_ = level_ == Level::Low ? Level::High : Level::Low;
    ++cycles_;
}

void Lock::enter(const Vessel& vessel) {
    Gate entrance = vessel.direction == Direction::Upstream ? Gate::Lower : Gate::Upper;
    int depth = level_ == Level::Low ? kLowDepthDm : kHighDepthDm;
    if (validateVessel(vessel) || vessel.draft_dm > depth ||
        gate(entrance) != GateState::Open || !fitsInChamber(inside_, vessel)) {
        throw LockError("cannot enter");
    }
    for (const Vessel& other : inside_) {
        if (other.call_sign == vessel.call_sign || other.kind == VesselKind::Oversized ||
            vessel.kind == VesselKind::Oversized) {
            throw LockError("cannot enter");
        }
    }
    inside_.push_back(vessel);
}

void Lock::exit(const std::string& call_sign) {
    if (inside_.empty() || inside_.front().call_sign != call_sign) {
        throw LockError("wrong vessel");
    }
    Gate exit_gate = inside_.front().direction == Direction::Upstream ? Gate::Upper : Gate::Lower;
    if (gate(exit_gate) != GateState::Open) throw LockError("exit gate is closed");
    inside_.erase(inside_.begin());
}

}  // namespace kitezh
