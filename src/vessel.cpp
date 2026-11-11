#include "kitezh/vessel.hpp"

namespace kitezh {

std::optional<std::string> validateVessel(const Vessel& vessel) {
    if (vessel.call_sign.empty()) return "empty call sign";
    if (vessel.call_sign.find_first_of(" \t\n\r\v\f") != std::string::npos) return "bad call sign";
    if (vessel.length_m <= 0 || vessel.length_m > kChamberLengthM) return "bad length";
    if (vessel.draft_dm <= 0) return "bad draft";
    return std::nullopt;
}

bool fitsInChamber(const std::vector<Vessel>& inside, const Vessel& next) {
    long long length = next.length_m;
    for (const Vessel& vessel : inside) {
        length += vessel.length_m + kGapM;
    }
    return length <= kChamberLengthM;
}

}  // namespace kitezh
