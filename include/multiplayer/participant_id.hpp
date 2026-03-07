#pragma once

#include "ParticipantId.h"

namespace ur {
namespace multiplayer {

using idl_participant_id_t = decltype(ur::multiplayer::idl::ParticipantId::value);
using participant_id_t = std::make_unsigned_t<idl_participant_id_t>;

} // namespace multiplayer
} // namespace ur
