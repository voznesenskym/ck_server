//
//  vz_server.cpp
//  ZMQServer
//
//  Created on 10/9/14.
//  Copyright (c) 2014 Verizon. All rights reserved.
//

#include "vz_server.h"
#include "vz_rpcbase.h"
#include <czmq.h>

#define CERT_DIR "CERTS"

#ifdef E2E_CURVE
client_t * vz_server::client_new (zframe_t *address)
{
    client_t *self = (client_t *) zmalloc (sizeof (client_t));
    assert (self);
    
    //curve_codec_t
    self->codec = curve_codec_new_server (this->cert, this->ctx);
    assert (self->codec);

    // zlist_t *hash_keys_from_meta = zhash_keys(metadata_from_codec);

    // printf("z_list is %s \n", hash_keys_from_meta);

    curve_codec_set_verbose(self->codec, true);
    self->address = zframe_dup (address);
    self->hashkey = zframe_strhex (address);
    return self;
}

void vz_server::send_frame_client(client_t *client, zframe_t *frame)
{
    if (client->incoming == NULL)
        client->incoming = zmsg_new ();

    zframe_t *cleartext = zframe_dup(frame);
    zmsg_add (client->incoming, cleartext);
    zmsg_pushstr (client->incoming, client->hashkey);
    zframe_t *encrypted_next = curve_codec_encode (client->codec, &cleartext);
    
    if (encrypted_next) {
        zframe_send (&client->address, this->router_socket, ZFRAME_MORE + ZFRAME_REUSE);
        zframe_send (&encrypted_next, this->router_socket, 0);
    }
}

void vz_server::send_multicast(zframe_t *frame, client_t *except_client)
{
    zlist_t *list_key = zhash_keys(this->clients);
    char *key = (char *)zlist_first(list_key);
    
    while (key != NULL)
    {
        client_t *client = (client_t *)zhash_lookup(this->clients, key);
        
        if (client != except_client && except_client->state == connected)
            this->send_frame_client(client, frame);
        
        key = (char *)zlist_next(list_key);
    }
}

#endif

vz_server * vz_server::instance()
{
    return new vz_server(NULL);
}

void vz_server::init()
{
#ifdef E2E_ENCRYPTION
    zctx_t *ctx = zctx_new ();
    zauth_t *auth = zauth_new (ctx);
    assert (auth);
    zauth_set_verbose (auth, true);
    zauth_configure_curve (auth, "*", CERT_DIR);
    
    zcert_t *server_cert = zcert_load(CERT_DIR "/server.cert");
    assert(server_cert);
    
    this->curve_server = curve_server_new (ctx, &server_cert);
    curve_server_set_verbose(this->curve_server, true);
    
#elif defined E2E_CURVE
    ctx = zctx_new ();
    zauth_t *auth = zauth_new (ctx);
    assert (auth);
    zauth_set_verbose (auth, true);
    zauth_configure_curve (auth, "*", CERT_DIR);
    
    this->router_socket = zsocket_new (ctx, ZMQ_ROUTER);
    this->cert = zcert_load (CERT_DIR "/server.cert");
    assert (this->cert);
    
    this->clients = zhash_new();
    assert (this->clients);
    
#else
    this->context = zctx_new();
    this->publisher_socket  = zsocket_new(context, ZMQ_PUB);
    this->pull_socket = zsocket_new(context, ZMQ_PULL);
#endif
}

vz_server::vz_server(vz_rpcbase *aRpc)
{
    this->rpc = aRpc;
    this->init();
}

void vz_server::run()
{

#ifdef E2E_ENCRYPTION
    printf("running E2E_ENCRYPTION \n");
    curve_server_bind(this->curve_server, (char *)"tcp://*:9000");
    bool finished = false;
    
    while (!finished)
    {
        zmsg_t *msg = curve_server_recv (this->curve_server);
        curve_server_send (curve_server, &msg);
        zmsg_destroy(&msg);
    }
    curve_server_destroy (&this->curve_server);
    
#elif defined E2E_CURVE
    printf("running E2E_CURVE \n");
    int rc = zsocket_bind (router_socket, "tcp://*:9000");
    assert (rc != -1);

    while (true)
    {
        zframe_t *address = zframe_recv (this->router_socket);
        char *hashkey = zframe_strhex (address);
        client_t *client = (client_t *)zhash_lookup (this->clients, hashkey);
        
        if (client == NULL)
        {
            client = this->client_new(address);
            client->state = pending;
            zhash_insert (this->clients, hashkey, client);
        }
        
        free (hashkey);
        zframe_destroy (&address);
        
        if (client == NULL)
        {
            assert(0);
            return;
        }
        
        if (client->state == pending)
        {
            printf("Connection pending... \n");
            zframe_t *input = zframe_recv (this->router_socket);
            zframe_t *output = curve_codec_execute (client->codec, &input);
            
            if (output)
            {
                zframe_send (&client->address, this->router_socket, ZFRAME_MORE + ZFRAME_REUSE);
                zframe_send (&output, this->router_socket, 0);
                if (curve_codec_connected (client->codec)) {
                    client->state = connected;

                    printf("Connected \n");
                }
            }
            else
            {
                printf("exception \n");
                client->state = exception;
            }
        }
        else if (client->state == connected)
        {
            zframe_t *encrypted = zframe_recv (this->router_socket);
            zframe_t *cleartext = curve_codec_decode (client->codec, &encrypted);
            zframe_t *cleartext_cpy = zframe_dup(cleartext);
            

            zhash_t *metadata_from_codec = curve_codec_metadata(client->codec);
            zhash_save(metadata_from_codec, "hashprintout");
            printf("Printed File \n");
            
            if (cleartext)
            {
                if (client->incoming == NULL)
                    client->incoming = zmsg_new ();
                
                zmsg_add (client->incoming, cleartext);
                
                if (!zframe_more (cleartext))
                {
                    zmsg_pushstr (client->incoming, client->hashkey);
                    zframe_t *encrypted = curve_codec_encode (client->codec, &cleartext);
                    

                    if (encrypted)
                    {
                        zframe_send (&client->address, this->router_socket, ZFRAME_MORE + ZFRAME_REUSE);
                        zframe_send (&encrypted, this->router_socket, 0);
                        this->send_multicast(cleartext_cpy, client);
                    }
                    else
                    {
                        client->state = exception;
                    }
                }
                
                zframe_destroy(&cleartext_cpy);
            }
            else
            {
                client->state = exception;
            }
        }
        
        if (client->state == exception)
        {
            zhash_delete (this->clients, client->hashkey);
        }
    }
    
#else
    zsocket_bind(this->publisher_socket, "tcp://*:9001");
    zsocket_bind(this->pull_socket, "tcp://*:9000");
        
    while (true)
    {
        printf("%s\n","Running...");
        
        zframe_t *fheader = zframe_recv(this->pull_socket);
        char *sheader = zframe_strdup(fheader);
        printf("Recv message from: %s\n",sheader);
        
        zframe_t *fcontent = zframe_recv(this->pull_socket);
        int rc = zframe_send(&fheader, this->publisher_socket, ZFRAME_MORE);
        assert(rc == 0);
        rc = zframe_send(&fcontent, this->publisher_socket, 0);
        assert(rc == 0);
        
        zframe_destroy(&fcontent);
        zframe_destroy(&fheader);
        
        sleep(0.0001);
    }
    
    printf("%s\n","Shutdowing the server.\n");
    
    zsocket_destroy(this->context, this->publisher_socket);
    zsocket_destroy(this->context, this->pull_socket);
    zctx_destroy (&this->context);
#endif
}

vz_server::~vz_server()
{
#ifdef E2E_ENCRYPTION
     curve_server_destroy (&curve_server);
    
#elif (defined E2E_CURVE)
    zsocket_destroy(ctx, router_socket);
    zctx_destroy(&ctx);
    
#else
    zsocket_destroy(this->context, this->publisher_socket);
    zsocket_destroy(this->context, this->pull_socket);
    zctx_destroy(&this->context);
#endif
}