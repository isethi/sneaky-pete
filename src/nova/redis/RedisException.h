#ifndef __NOVA_REDIS_REDISEXCEPTION_H
#define __NOVA_REDIS_REDISEXCEPTION_H

#include <exception>

namespace nova { namespace redis {
class RedisException : public std::exception {

    public:
        enum Code {
            MISSING_ROOT_PASSWORD,
            PREPARE_ERROR
        }

        RedisException(Code code) throw();

        virtual ~RedisException() throw();

        virtual const char * what() throw();

    private:
        Code code;
};

} }

#endif
