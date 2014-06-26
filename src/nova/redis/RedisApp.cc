#include "pch.hpp"
#include "RedisApp.h"

using nova::guest::backup::BackupRestoreInfo;
using boost::format;
using boost::optional;
using nova::redis::RedisAppStatusPtr;
using std::string;

namespace nova { namespace redis {


RedisApp::RedisApp(RedisAppStatusPtr app_status)
:   app_status(app_status) {
}

RedisApp::~RedisApp() {
}


void RedisApp::prepare(const optional<string> & json_root_password,
                       const string & config_contents,
                       const optional<string> & overrides)
{
    if (!json_root_password) {
        NOVA_LOG_ERROR("Missing root password!");
        throw RedisException(RedisException::MISSING_ROOT_PASSWORD);
    }
    const string root_password = json_root_password.get();

    NOVA_LOG_INFO("Removing the redis conf file.");
    if (system("sudo rm /etc/redis/redis.conf") == -1)
    {
        NOVA_LOG_ERROR("Unable to remove redis config file.");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Chmoding /etc/redis to 777.");
    if (system("sudo chmod -R 777 /etc/redis") == -1)
    {
        NOVA_LOG_ERROR("Unable to chmod /etc/redis to 777");
        throw RedisException(RedisException::PREPARE_ERROR);
    }

    NOVA_LOG_INFO("Creating new config file.");
    {
        ofstream fd;
        fd.open("/etc/redis/redis.conf");
        NOVA_LOG_INFO("Writing config contents.");
        NOVA_LOG_DEBUG(config_contents);
        if (fd.good())
        {
            fd << config_contents;
            fd.close();
        }
        else
        {
            NOVA_LOG_ERROR("Couldn't open config file.");
            throw RedisException(RedisException::PREPARE_ERROR);
        }
    }
    if (system("sudo chmod 644 /etc/redis/redis.conf") == -1)
    {
        NOVA_LOG_ERROR("Unable to revert file permissions.");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Creating /etc/redis/conf.d");
    if (system("mkdir /etc/redis/conf.d") == -1)
    {
        NOVA_LOG_ERROR("Unable to create redis conf.d");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Removing /etc/redis/conf.d/local.conf");
    if (system("rm /etc/redis/conf.d/local.conf") == -1)
    {
        NOVA_LOG_ERROR("Unable to remove local.conf");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Creating new /etc/redis/conf.d/local.conf");
    if (system("touch /etc/redis/conf.d/local.conf") == -1)
    {
        NOVA_LOG_ERROR("Unable to create local.conf");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Opening /etc/redis/conf.d/local.conf for writing");
    {
        ofstream fd;
        fd.open("/etc/redis/conf.d/local.conf");
        if (fd.is_open())
        {
            const string local_config = str(format(
                "requirepass %1%\n"
                "rename-command CONFIG %2%\n"
                "rename-command MONITOR %3%")
                % root_password % get_uuid() % get_uuid()));
            fd << local_config;
            fd.close();
            fd.clear();
        }
        else
        {
            NOVA_LOG_ERROR("Unable to open /etc/redis/conf.d/local.conf");
            throw RedisException(RedisException::PREPARE_ERROR);
        }
    }
    NOVA_LOG_INFO("Chmoding /etc/redis/conf.d/local.conf to 644");
    if (system("sudo chmod 644 /etc/redis/conf.d/local.conf") == -1)
    {
        NOVA_LOG_ERROR("Unable to chmod /etc/redis/conf.d/local.conf"
                       "to 644");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Chmoding  /etc/redis to 755");
    if (system("sudo chmod 755 /etc/redis") == -1)
    {
        NOVA_LOG_ERROR("Unable to chmod -R /etc/redis to 755");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    if (system("sudo chmod 755 /etc/redis/conf.d") == -1)
    {
        NOVA_LOG_ERROR("Unable to chmod -R /etc/redis/conf.d to 755");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Chowing /etc/redis to root:root");
    if (system("sudo chown -R root:root /etc/redis") == -1)
    {
        NOVA_LOG_ERROR("Unable to chown /etc/redis to root:root");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
    NOVA_LOG_INFO("Connecting to redis instance.");
    nova::redis::Client client = nova::redis::Client();
    NOVA_LOG_INFO("Stopping redis instance.");
    if (client.control->stop() != 0)
    {
        NOVA_LOG_INFO("Unable to stop redis instance.");
        //TODO(tim.simpson): If we can't stop, is this just ok?
    }
    NOVA_LOG_INFO("Starting redis instance.");
    if (client.control->start() != 0)
    {
        NOVA_LOG_ERROR("Unable to start redis instance!");
        throw RedisException(RedisException::PREPARE_ERROR);
    }
}

} }  // end namespace
