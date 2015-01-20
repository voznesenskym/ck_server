//
//  vz_rpcbase.h
//  ZMQServer
//
//  Created on 10/9/14.
//  Copyright (c) 2014 Verizon. All rights reserved.
//

#ifndef __ZMQServer__vz_rpcbase__
#define __ZMQServer__vz_rpcbase__


#include <string>
#include <map>

class vz_rpcbase
{
private:
    std::map<std::string,void *> methods;
    
public:
    vz_rpcbase();
    std::map<std::string,void *> get_list_methods();
    
public:
    void remote_produce_test(const char *arg);
};

#endif /* defined(__ZMQServer__vz_rpcbase__) */
