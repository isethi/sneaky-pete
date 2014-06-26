#include "pch.hpp"
#include "nova/guest/RedisException.h"


namespace nova { namespace redis {

RedisException::RedisException(RedisException::Code code) throw()
: code(code)
{
}

RedisException::~RedisException() throw() {
}

const char * RedisException::what() throw() {
    switch(code) {
        case MISSING_ROOT_PASSWORD:
            return "Missing root password argument in prepare call!";
        case PREPARE_ERROR:
            return "An error occured while preparing Redis.";
        default:
            return "An error occurred.";
    }
}

} }  // end namespace
