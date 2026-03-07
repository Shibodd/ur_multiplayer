#include <Ice/LocalExceptions.h>
#include <RaceControl.h>
#include <Ice/Endpoint.h>
#include <Ice/Identity.h>
#include <climits>
#include <iostream>
#include <algorithm>

#include <memory>
#include <multiplayer/participant_id.hpp>

#include <multiplayer/server.hpp>
#include <multiplayer/lobby.hpp>

namespace ur {
namespace multiplayer {

void LobbyServant::join(std::string version, std::optional<idl::ParticipantPrx> participant, const Ice::Current& current) {
    if (not participant.has_value()) {
        std::cout << "Join request with empty participant discarded." << std::endl;
        return;
    }

    idl::Hello hello;
    hello.raceControlIntervalMs.value = std::chrono::duration_cast<std::chrono::milliseconds>(m_race_control_interval).count();
    hello.updateIntervalMs.value = std::chrono::duration_cast<std::chrono::milliseconds>(m_update_interval).count();

    if (version != m_version) {
        std::cout << "Join request with wrong version discarded." << std::endl;

        hello.message = "Incompatible version. Server is running " + m_version;
        participant->hello(hello);
        return;
    }

    participant_id_t id = m_next_id.fetch_add(1);
    Ice::Identity server_identity { std::to_string(id), "server" };

    auto server_servant = std::make_shared<ServerServant>(*participant, id, m_update_interval, m_update_timeout);

    std::optional<Ice::ObjectPrx> server_proxy;

    {
        auto tok = server_servant->set_cleanup_callback([lobby=weak_from_this(), adapter=std::weak_ptr(current.adapter), id, server_identity]() {
            if (auto ptr = lobby.lock()) {
                ptr->m_active_servants.erase(id);
            }

            /*  The adapter tries to delete SOMETHING and lock SOMETHING on this same thread causing a deadlock.
                Couldn't figure out what those SOMETHINGs are, so just launch this on another thread and call it a day :)
            */
            std::thread([adapter, server_identity]() {
                if (auto ptr = adapter.lock()) {
                    try {
                        ptr->remove(server_identity);
                    } catch (const Ice::ObjectAdapterDestroyedException&) {
                        // don't care
                    }
                }
            }).detach();
        });

        server_proxy = current.adapter->add(server_servant, server_identity);

        {
            std::scoped_lock lock(m_active_servants_mtx);
            m_active_servants.emplace(id, server_servant);
        }

        tok.release();
    }

    if (not server_proxy) {
        hello.message = "Internal exception";
        participant->hello(hello);
        std::cerr << "Something went wrong while instantiating a ServerServant" << std::endl;
    } else {
        hello.participantId.value = static_cast<idl_participant_id_t>(id);
        hello.message = "Success";
        hello.server = Ice::uncheckedCast<idl::ServerPrx>(server_proxy)->ice_datagram();
        participant->hello(hello);
    
        std::cout << "A new participant has joined." << std::endl;
    }

}

} // namespace multiplayer
} // namespace ur