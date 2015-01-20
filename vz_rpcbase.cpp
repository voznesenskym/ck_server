//
//  vz_rpcbase.cpp
//  ZMQServer
//
//  Created on 10/9/14.
//  Copyright (c) 2014 Verizon. All rights reserved.
//

#include "vz_rpcbase.h"

vz_rpcbase::vz_rpcbase()
{
    // register remote_produce_test
    void (vz_rpcbase::* method)(const char *) = &vz_rpcbase::remote_produce_test;
    void *cast_method = (void * &)method;
    this->methods["remote_produce_test"] = cast_method;
}

std::map<std::string,void *> vz_rpcbase::get_list_methods()
{
    return this->methods;
}

void vz_rpcbase::remote_produce_test(const char *arg)
{
}