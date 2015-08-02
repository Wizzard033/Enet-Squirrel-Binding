////////////////////////////////////////////////////////////
//
// Copyright (c) 2013-2015 Brandon Haffen - bhaffen97@gmail.com
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <wizzardsrealms.hpp>
#include <enet/enetsqrat.hpp>
#include <cstdlib>
#include <iostream>


////////////////////////////////////////////////////////////
static SQInteger parse_address(HSQUIRRELVM v, const std::string& addr_str, ENetAddress& address)
{
    size_t colonPos = addr_str.find(':');
    if (colonPos == std::string::npos) return sq_throwerror(v, _SC("Failed to parse address (missing port in address?)"));
    std::string host_str = addr_str.substr(0, colonPos);
    std::string port_str = addr_str.substr(colonPos + 1);
    if (host_str.empty()) return sq_throwerror(v, _SC("Failed to parse address"));
    if (port_str.empty()) return sq_throwerror(v, _SC("Missing port in address"));
    if (host_str == "*") {
        address.host = ENET_HOST_ANY;
    } else {
        if (enet_address_set_host(&address, host_str.c_str()) != 0) return sq_throwerror(v, _SC("Failed to resolve host name"));
    }
    if (port_str == "*") {
        address.port = ENET_PORT_ANY;
    } else {
        address.port = std::atoi(port_str.c_str());
    }
    return 0;
}


////////////////////////////////////////////////////////////
static void push_event(HSQUIRRELVM v, const ENetEvent& event)
{
    sq_newtable(v);
    if (event.peer) {
        Sqrat::PushVar(v, "peer");
        Sqrat::PushVar(v, event.peer);
        sq_newslot(v, -3, false);
    }
    switch (event.type) {
    case ENET_EVENT_TYPE_CONNECT :
        Sqrat::PushVar(v, "data");
        Sqrat::PushVar(v, event.data);
        sq_newslot(v, -3, false);
        Sqrat::PushVar(v, "type");
        Sqrat::PushVar(v, ENET_EVENT_TYPE_CONNECT);
        break;
    case ENET_EVENT_TYPE_DISCONNECT :
        Sqrat::PushVar(v, "data");
        Sqrat::PushVar(v, event.data);
        sq_newslot(v, -3, false);
        Sqrat::PushVar(v, "type");
        Sqrat::PushVar(v, ENET_EVENT_TYPE_DISCONNECT);
        break;
    case ENET_EVENT_TYPE_RECEIVE :
        Sqrat::PushVar(v, "data");
        Sqrat::PushVar(v, std::string((const char*) event.packet->data, event.packet->dataLength));
        sq_newslot(v, -3, false);
        Sqrat::PushVar(v, "channel");
        Sqrat::PushVar(v, event.channelID);
        sq_newslot(v, -3, false);
        Sqrat::PushVar(v, "type");
        Sqrat::PushVar(v, ENET_EVENT_TYPE_RECEIVE);
        enet_packet_destroy(event.packet);
        break;
    case ENET_EVENT_TYPE_NONE :
        Sqrat::PushVar(v, "type");
        Sqrat::PushVar(v, ENET_EVENT_TYPE_NONE);
        break;
    }
    sq_newslot(v, -3, false);
}


////////////////////////////////////////////////////////////
static SQInteger read_packet(HSQUIRRELVM v, ENetPacket*& packet, const std::string& data, enet_uint32 flag)
{
    if (flag != ENET_PACKET_FLAG_RELIABLE && flag != ENET_PACKET_FLAG_UNSEQUENCED && flag != 0) {
        return sq_throwerror(v, _SC("Unknown packet flag"));
    }
    packet = enet_packet_create(data.c_str(), data.size(), flag);
    if (packet == NULL) {
        return sq_throwerror(v, _SC("Failed to create packet"));
    }
    return 0;
}


////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////
static SQInteger enetPeer_constructor(HSQUIRRELVM v)
{
    return sq_throwerror(v, _SC("constructor is protected"));
}


////////////////////////////////////////////////////////////
// Requests a disconnection from a peer
////////////////////////////////////////////////////////////
static SQInteger enetPeer_disconnect(HSQUIRRELVM v)
{
    if (sq_gettop(v) == 1) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        if (!Sqrat::Error::Occurred(v)) {
            enet_peer_disconnect(left.value, 0);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 2) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        Sqrat::Var<enet_uint32> data(v, 2);
        if (!Sqrat::Error::Occurred(v)) {
            enet_peer_disconnect(left.value, data.value);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Requests a disconnection from a peer, but only after all queued outgoing packets are sent
////////////////////////////////////////////////////////////
static SQInteger enetPeer_disconnect_later(HSQUIRRELVM v)
{
    if (sq_gettop(v) == 1) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        if (!Sqrat::Error::Occurred(v)) {
            enet_peer_disconnect_later(left.value, 0);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 2) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        Sqrat::Var<enet_uint32> data(v, 2);
        if (!Sqrat::Error::Occurred(v)) {
            enet_peer_disconnect_later(left.value, data.value);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Force an immediate disconnection from a peer
////////////////////////////////////////////////////////////
static SQInteger enetPeer_disconnect_now(HSQUIRRELVM v)
{
    if (sq_gettop(v) == 1) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        if (!Sqrat::Error::Occurred(v)) {
            enet_peer_disconnect_now(left.value, 0);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 2) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        Sqrat::Var<enet_uint32> data(v, 2);
        if (!Sqrat::Error::Occurred(v)) {
            enet_peer_disconnect_now(left.value, data.value);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Returns the index of the peer
////////////////////////////////////////////////////////////
static SQInteger enetPeer_index(HSQUIRRELVM v)
{
    if (sq_gettop(v) == 1) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        if (!Sqrat::Error::Occurred(v)) {
            std::size_t peer_index;
            for (peer_index = 0; peer_index < left.value->host->peerCount; peer_index++) {
                if (left.value == &(left.value->host->peers[peer_index])) {
                    Sqrat::PushVar(v, peer_index + 1);
                    return 1;
                }
            }
            return sq_throwerror(v, _SC("Could not find peer index"));
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Queues a packet to be sent
////////////////////////////////////////////////////////////
static SQInteger enetPeer_send(HSQUIRRELVM v)
{
    ENetPacket* packet;
    if (sq_gettop(v) == 2) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        Sqrat::Var<const std::string&> data(v, 2);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = read_packet(v, packet, data.value, ENET_PACKET_FLAG_RELIABLE);
            if (SQ_FAILED(result)) {
                return result;
            }
            enet_peer_send(left.value, 0, packet);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 3) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        Sqrat::Var<const std::string&> data(v, 2);
        Sqrat::Var<enet_uint8> channel_id(v, 3);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = read_packet(v, packet, data.value, ENET_PACKET_FLAG_RELIABLE);
            if (SQ_FAILED(result)) {
                return result;
            }
            enet_peer_send(left.value, channel_id.value, packet);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 4) {
        Sqrat::Var<ENetPeer*> left(v, 1);
        Sqrat::Var<const std::string&> data(v, 2);
        Sqrat::Var<enet_uint8> channel_id(v, 3);
        Sqrat::Var<enet_uint32> flag(v, 4);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = read_packet(v, packet, data.value, flag.value);
            if (SQ_FAILED(result)) {
                return result;
            }
            enet_peer_send(left.value, channel_id.value, packet);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Returns the downstream bandwidth of the client in bytes/second
////////////////////////////////////////////////////////////
static enet_uint32 enetPeer_incoming_bandwidth(ENetPeer* left)
{
    return left->incomingBandwidth;
}


////////////////////////////////////////////////////////////
// Returns the upstream bandwidth of the client in bytes/second
////////////////////////////////////////////////////////////
static enet_uint32 enetPeer_outgoing_bandwidth(ENetPeer* left)
{
    return left->outgoingBandwidth;
}


////////////////////////////////////////////////////////////
// Returns the mean packet loss of reliable packets as a percentage
////////////////////////////////////////////////////////////
static enet_uint32 enetPeer_packet_loss(ENetPeer* left)
{
    return left->packetLoss / (float) ENET_PEER_PACKET_LOSS_SCALE;
}


////////////////////////////////////////////////////////////
// Returns the current round trip time (i.e. ping)
////////////////////////////////////////////////////////////
static enet_uint32 enetPeer_round_trip_time(ENetPeer* left)
{
    return left->roundTripTime;
}


////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////
static SQInteger enetHost_constructor(HSQUIRRELVM v)
{
    return sq_throwerror(v, _SC("constructor is protected"));
}


////////////////////////////////////////////////////////////
// Queues a packet to be sent to all peers associated with the host
////////////////////////////////////////////////////////////
static SQInteger enetHost_broadcast(HSQUIRRELVM v)
{
    ENetPacket* packet;
    if (sq_gettop(v) == 2) {
        Sqrat::Var<ENetHost*> left(v, 1);
        Sqrat::Var<const std::string&> data(v, 2);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = read_packet(v, packet, data.value, ENET_PACKET_FLAG_RELIABLE);
            if (SQ_FAILED(result)) {
                return result;
            }
            enet_host_broadcast(left.value, 0, packet);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 3) {
        Sqrat::Var<ENetHost*> left(v, 1);
        Sqrat::Var<const std::string&> data(v, 2);
        Sqrat::Var<enet_uint8> channel_id(v, 3);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = read_packet(v, packet, data.value, ENET_PACKET_FLAG_RELIABLE);
            if (SQ_FAILED(result)) {
                return result;
            }
            enet_host_broadcast(left.value, channel_id.value, packet);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 4) {
        Sqrat::Var<ENetHost*> left(v, 1);
        Sqrat::Var<const std::string&> data(v, 2);
        Sqrat::Var<enet_uint8> channel_id(v, 3);
        Sqrat::Var<enet_uint32> flag(v, 4);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = read_packet(v, packet, data.value, flag.value);
            if (SQ_FAILED(result)) {
                return result;
            }
            enet_host_broadcast(left.value, channel_id.value, packet);
            return 0;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Checks for any queued events on the host and dispatches one if available
////////////////////////////////////////////////////////////
static SQInteger enetHost_check_events(HSQUIRRELVM v)
{
    if (sq_gettop(v) == 1) {
        Sqrat::Var<ENetHost*> left(v, 1);
        if (!Sqrat::Error::Occurred(v)) {
            ENetEvent event;
            int out = enet_host_check_events(left.value, &event);
            if (out == 0) {
                sq_pushnull(v);
                return 1;
            }
            if (out < 0) return sq_throwerror(v, _SC("Error checking network event"));
            push_event(v, event);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Initiates a connection to a foreign host
////////////////////////////////////////////////////////////
static SQInteger enetHost_connect(HSQUIRRELVM v)
{
    ENetAddress address;
    ENetPeer* peer;
    if (sq_gettop(v) == 2) {
        Sqrat::Var<ENetHost*> left(v, 1);
        Sqrat::Var<const std::string&> addr_str(v, 2);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = parse_address(v, addr_str.value, address);
            if (SQ_FAILED(result)) {
                return result;
            }
            peer = enet_host_connect(left.value, &address, 1, 0);
            if (peer == NULL) {
                return sq_throwerror(v, _SC("Failed to create peer"));
            }
            Sqrat::PushVar(v, peer);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 3) {
        Sqrat::Var<ENetHost*> left(v, 1);
        Sqrat::Var<const std::string&> addr_str(v, 2);
        Sqrat::Var<size_t> channelCount(v, 3);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = parse_address(v, addr_str.value, address);
            if (SQ_FAILED(result)) {
                return result;
            }
            peer = enet_host_connect(left.value, &address, channelCount.value, 0);
            if (peer == NULL) {
                return sq_throwerror(v, _SC("Failed to create peer"));
            }
            Sqrat::PushVar(v, peer);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 4) {
        Sqrat::Var<ENetHost*> left(v, 1);
        Sqrat::Var<const std::string&> addr_str(v, 2);
        Sqrat::Var<size_t> channelCount(v, 3);
        Sqrat::Var<enet_uint32> data(v, 4);
        if (!Sqrat::Error::Occurred(v)) {
            SQInteger result = parse_address(v, addr_str.value, address);
            if (SQ_FAILED(result)) {
                return result;
            }
            peer = enet_host_connect(left.value, &address, channelCount.value, data.value);
            if (peer == NULL) {
                return sq_throwerror(v, _SC("Failed to create peer"));
            }
            Sqrat::PushVar(v, peer);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Waits for events on the host specified and shuttles packets between the host and its peers
////////////////////////////////////////////////////////////
static SQInteger enetHost_service(HSQUIRRELVM v)
{
    if (sq_gettop(v) == 1) {
        Sqrat::Var<ENetHost*> left(v, 1);
        if (!Sqrat::Error::Occurred(v)) {
            ENetEvent event;
            int out = enet_host_service(left.value, &event, 0);
            if (out == 0) {
                sq_pushnull(v);
                return 1;
            }
            if (out < 0) return sq_throwerror(v, _SC("Error during network servicing"));
            push_event(v, event);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Enables an adaptive order-2 PPM range coder for the transmitted data of all peers
////////////////////////////////////////////////////////////
static bool enetHost_compress_with_range_coder(ENetHost* left)
{
    if (enet_host_compress_with_range_coder(left) == 0) {
        return true;
    }
    return false;
}


////////////////////////////////////////////////////////////
// Returns a new host for communicating to peers
////////////////////////////////////////////////////////////
static SQInteger host_create(HSQUIRRELVM v)
{
    ENetAddress address;
    ENetHost* host;
    if (sq_gettop(v) == 2) {
        Sqrat::Var<Sqrat::SharedPtr<std::string> > addr_str(v, 2);
        if (!Sqrat::Error::Occurred(v)) {
            if (addr_str.value.Get() != NULL) {
                SQInteger result = parse_address(v, *addr_str.value, address);
                if (SQ_FAILED(result)) {
                    return result;
                }
            }
            host = enet_host_create(addr_str.value.Get() != NULL ? &address : NULL, 64, 1, 0, 0);
            if (host == NULL) {
                wr::Print(v, "Failed to create host (port already in use?)\n");
                sq_pushnull(v);
                return 1;
            }
            Sqrat::PushVar(v, host);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 3) {
        Sqrat::Var<Sqrat::SharedPtr<std::string> > addr_str(v, 2);
        Sqrat::Var<size_t> peerCount(v, 3);
        if (!Sqrat::Error::Occurred(v)) {
            if (addr_str.value.Get() != NULL) {
                SQInteger result = parse_address(v, *addr_str.value, address);
                if (SQ_FAILED(result)) {
                    return result;
                }
            }
            host = enet_host_create(addr_str.value.Get() != NULL ? &address : NULL, peerCount.value, 1, 0, 0);
            if (host == NULL) {
                wr::Print(v, "Failed to create host (port already in use?)\n");
                sq_pushnull(v);
                return 1;
            }
            Sqrat::PushVar(v, host);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 4) {
        Sqrat::Var<Sqrat::SharedPtr<std::string> > addr_str(v, 2);
        Sqrat::Var<size_t> peerCount(v, 3);
        Sqrat::Var<size_t> channelLimit(v, 4);
        if (!Sqrat::Error::Occurred(v)) {
            if (addr_str.value.Get() != NULL) {
                SQInteger result = parse_address(v, *addr_str.value, address);
                if (SQ_FAILED(result)) {
                    return result;
                }
            }
            host = enet_host_create(addr_str.value.Get() != NULL ? &address : NULL, peerCount.value, channelLimit.value, 0, 0);
            if (host == NULL) {
                wr::Print(v, "Failed to create host (port already in use?)\n");
                sq_pushnull(v);
                return 1;
            }
            Sqrat::PushVar(v, host);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 5) {
        Sqrat::Var<Sqrat::SharedPtr<std::string> > addr_str(v, 2);
        Sqrat::Var<size_t> peerCount(v, 3);
        Sqrat::Var<size_t> channelLimit(v, 4);
        Sqrat::Var<enet_uint32> incomingBandwidth(v, 5);
        if (!Sqrat::Error::Occurred(v)) {
            if (addr_str.value.Get() != NULL) {
                SQInteger result = parse_address(v, *addr_str.value, address);
                if (SQ_FAILED(result)) {
                    return result;
                }
            }
            host = enet_host_create(addr_str.value.Get() != NULL ? &address : NULL, peerCount.value, channelLimit.value, incomingBandwidth.value, 0);
            if (host == NULL) {
                wr::Print(v, "Failed to create host (port already in use?)\n");
                sq_pushnull(v);
                return 1;
            }
            Sqrat::PushVar(v, host);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    } else if (sq_gettop(v) == 6) {
        Sqrat::Var<Sqrat::SharedPtr<std::string> > addr_str(v, 2);
        Sqrat::Var<size_t> peerCount(v, 3);
        Sqrat::Var<size_t> channelLimit(v, 4);
        Sqrat::Var<enet_uint32> incomingBandwidth(v, 5);
        Sqrat::Var<enet_uint32> outgoingBandwidth(v, 6);
        if (!Sqrat::Error::Occurred(v)) {
            if (addr_str.value.Get() != NULL) {
                SQInteger result = parse_address(v, *addr_str.value, address);
                if (SQ_FAILED(result)) {
                    return result;
                }
            }
            host = enet_host_create(addr_str.value.Get() != NULL ? &address : NULL, peerCount.value, channelLimit.value, incomingBandwidth.value, outgoingBandwidth.value);
            if (host == NULL) {
                wr::Print(v, "Failed to create host (port already in use?)\n");
                sq_pushnull(v);
                return 1;
            }
            Sqrat::PushVar(v, host);
            return 1;
        }
        return sq_throwerror(v, Sqrat::Error::Message(v).c_str());
    }
    return sq_throwerror(v, _SC("wrong number of parameters"));
}


////////////////////////////////////////////////////////////
// Initializes and registers the ENet library in the given VM
////////////////////////////////////////////////////////////
void RegisterEnetLib(HSQUIRRELVM v, Sqrat::Table& namespaceTable)
{
    Sqrat::ConstTable constTable(v);
    constTable.Const(_SC("ENET_EVENT_TYPE_CONNECT"), static_cast<int>(ENET_EVENT_TYPE_CONNECT));
    constTable.Const(_SC("ENET_EVENT_TYPE_DISCONNECT"), static_cast<int>(ENET_EVENT_TYPE_DISCONNECT));
    constTable.Const(_SC("ENET_EVENT_TYPE_RECEIVE"), static_cast<int>(ENET_EVENT_TYPE_RECEIVE));
    constTable.Const(_SC("ENET_EVENT_TYPE_NONE"), static_cast<int>(ENET_EVENT_TYPE_NONE));
    constTable.Const(_SC("ENET_PACKET_FLAG_RELIABLE"), static_cast<int>(ENET_PACKET_FLAG_RELIABLE));
    constTable.Const(_SC("ENET_PACKET_FLAG_UNSEQUENCED"), static_cast<int>(ENET_PACKET_FLAG_UNSEQUENCED));
    constTable.Const(_SC("ENET_PACKET_FLAG_UNRELIABLE"), 0);

    Sqrat::Class<ENetPeer, Sqrat::NoConstructor<ENetPeer> > enetPeer(v, _SC("enet.Peer"));
    enetPeer.SquirrelFunc(_SC("constructor"), &enetPeer_constructor);
    enetPeer.SquirrelFunc(_SC("disconnect"), &enetPeer_disconnect);
    enetPeer.SquirrelFunc(_SC("disconnect_later"), &enetPeer_disconnect_later);
    enetPeer.SquirrelFunc(_SC("disconnect_now"), &enetPeer_disconnect_now);
    enetPeer.SquirrelFunc(_SC("index"), &enetPeer_index);
    enetPeer.SquirrelFunc(_SC("send"), &enetPeer_send);
    enetPeer.GlobalFunc(_SC("destroy"), &enet_host_destroy);
    enetPeer.GlobalFunc(_SC("incoming_bandwidth"), &enetPeer_incoming_bandwidth);
    enetPeer.GlobalFunc(_SC("outgoing_bandwidth"), &enetPeer_outgoing_bandwidth);
    enetPeer.GlobalFunc(_SC("packet_loss"), &enetPeer_packet_loss);
    enetPeer.GlobalFunc(_SC("ping"), &enet_peer_ping);
    enetPeer.GlobalFunc(_SC("ping_interval"), &enet_peer_ping_interval);
    enetPeer.GlobalFunc(_SC("reset"), &enet_peer_reset);
    enetPeer.GlobalFunc(_SC("round_trip_time"), &enetPeer_round_trip_time);
    enetPeer.GlobalFunc(_SC("throttle_configure"), &enet_peer_throttle_configure);
    enetPeer.GlobalFunc(_SC("timeout"), &enet_peer_timeout);

    Sqrat::Class<ENetHost, Sqrat::NoConstructor<ENetHost> > enetHost(v, _SC("enet.Host"));
    enetHost.SquirrelFunc(_SC("constructor"), &enetHost_constructor);
    enetHost.SquirrelFunc(_SC("broadcast"), &enetHost_broadcast);
    enetHost.SquirrelFunc(_SC("check_events"), &enetHost_check_events);
    enetHost.SquirrelFunc(_SC("connect"), &enetHost_connect);
    enetHost.SquirrelFunc(_SC("service"), &enetHost_service);
    enetHost.GlobalFunc(_SC("bandwidth_limit"), &enet_host_bandwidth_limit);
    enetHost.GlobalFunc(_SC("compress_with_range_coder"), &enetHost_compress_with_range_coder);

    namespaceTable.Bind(_SC("Host"), enetHost);
    namespaceTable.SquirrelFunc(_SC("host_create"), &host_create);
}
