//
//  vz_server.h
//  ZMQServer
//
//  Created on 10/9/14.
//  Copyright (c) 2014 Verizon. All rights reserved.
//

#ifndef __ZMQServer__vz_server__
#define __ZMQServer__vz_server__

#include <iostream>
#include <map>
#include <string>
#include <czmq.h>
#include <curve.h>
#include <sstream>
#include <vector>

//-- No multicast (one - to - one)
#define E2E_ENCRYPTION

//-- Multicast (one - to - many)
#define E2E_CURVE

#ifdef E2E_CURVE
    #undef E2E_ENCRYPTION
#endif

class vz_rpcbase;

typedef enum
{
    pending = 0,                //  Waiting for connection
    connected = 1,              //  Ready for messages
    exception = 2               //  Failed due to some error
}
state_t;

typedef struct
{
    curve_codec_t *codec;       //  Client CurveZMQ codec
    state_t state;              //  Current state
    zframe_t *address;          //  Client address identity
    zmsg_t *incoming;           //  Incoming message, if any
    char *hashkey;              //  Key into clients hash
    char * identity;            //  Identity of the client
}
client_t;

class vz_server
{
    
private:
    
#ifdef E2E_ENCRYPTION
    curve_server_t *curve_server;
    
#elif defined E2E_CURVE
    zctx_t          * ctx;
    void            * router_socket;
    zhash_t         *clients;
    zcert_t         *cert;
    zhash_t         *clients_by_identity;
    
#else
    zctx_t  * context;
    void    * publisher_socket;
    void    * pull_socket;
#endif
    
    vz_rpcbase *rpc;
    
private:    
    void init();
    
#ifdef E2E_CURVE
    client_t * client_new (zframe_t *address);
    void send_frame_client (client_t *client, zframe_t *frame);
    void send_multicast(zframe_t *frame, client_t *except_client);
    char **split(char *input, char *delimiter);
#endif
    
public:
    static vz_server * instance();
    
public:
    vz_server(vz_rpcbase *aRpc);
    ~vz_server();
    
    void run();
};

#endif /* defined(__ZMQServer__vz_server__) */
