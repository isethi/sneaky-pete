#ifndef __NOVA_REDIS_REDISAPP_H
#define __NOVA_REDIS_REDISAPP_H

#include "nova/guest/redis/status.h"
#include "nova/datastores/DatastoreApp.h"
#include "nova/guest/backup/BackupRestore.h"
#include <boost/optional.hpp>
#include <vector>

namespace nova { namespace redis {

class RedisApp : public nova::datastores::DatastoreAppWithRoot {
    public:
        RedisApp()
        virtual ~RedisApp(nova::redis::RedisAppStatusPtr app_status);

    protected:
        virtual void prepare(
                const std::string & root_password,
                const std::string & config_contents,
                const boost::optional<std::string> & overrides,
                boost::optional<nova::guest::backup::BackupRestoreInfo> restore
            );
    private:
        nova::redis::RedisAppStatusPtr app_status;
};

} }
