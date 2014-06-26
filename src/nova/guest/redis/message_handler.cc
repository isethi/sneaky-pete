#include "pch.hpp"
#include "message_handler.h"

#include "nova/guest/apt.h"
#include "nova/guest/backup/BackupRestore.h"
#include "nova/guest/guest.h"
#include "nova/guest/GuestException.h"
#include "nova/process.h"
#include "nova/Log.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <exception>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/std/list.hpp>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/variant.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "client.h"
#include "constants.h"

using namespace boost::assign;

using nova::guest::apt::AptGuest;
using nova::guest::backup::BackupRestoreInfo;
using nova::guest::GuestException;
using nova::Log;
using nova::JsonData;
using nova::JsonDataPtr;
using nova::JsonObject;
using nova::JsonObjectPtr;
using nova::json_string;
using boost::optional;
using namespace std;
using boost::tie;
using nova::redis::SOCKET_NAME;
using nova::redis::REDIS_PORT;
using nova::redis::REDIS_AGENT_NAME;
using nova::redis::DEFAULT_REDIS_CONFIG;
using nova::redis::REQUIRE_PASS;

namespace nova { namespace guest {


namespace {

std::string get_uuid()
{
    std::stringstream ss;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    ss << uuid;
    return ss.str();
}

vector<string> get_packages_argument(JsonObjectPtr obj) {
    try {
        const auto packages = obj->get_array("packages")->to_string_vector();
        return packages;
    } catch(const JsonException) {
        NOVA_LOG_DEBUG("Interpretting \"packages\" as a single string.");
        vector<string> packages;
        packages.push_back(obj->get_string("packages"));
        return packages;
    }
}


}//end anon namespace.

RedisMessageHandler::RedisMessageHandler(
    nova::guest::apt::AptGuestPtr apt,
    nova::redis::RedisAppStatusPtr app_status,
    nova::guest::monitoring::MonitoringManagerPtr monitoring) :
    apt(apt),
    app_status(app_status),
    monitoring(monitoring)
{
}

RedisMessageHandler::~RedisMessageHandler()
{
}

JsonDataPtr RedisMessageHandler::handle_message(const GuestInput & input) {
    if (input.method_name == "prepare")
    {
        prepare_handler->prepare(input);
        return JsonData::from_string("prepare ok");
    }
    else if (input.method_name == "restart")
    {
        NOVA_LOG_INFO("Entering restart call.");
        NOVA_LOG_INFO("Connecting to redis instance.");
        nova::redis::Client client();
        NOVA_LOG_INFO("Stopping redis instance.");
        if (client.control->stop() != 0)
        {
            NOVA_LOG_ERROR("Unable to stop redis instance!");
            return JsonData::from_string("Unable to stop redis instance!");

        }
        NOVA_LOG_INFO("Starting redis instance.");
        if (client.control->start() != 0)
        {
            NOVA_LOG_ERROR("Unable to start redis instance!");
            return JsonData::from_string("Unable to start redis instance!");
        }
        NOVA_LOG_INFO("Redis instance restarted");
        NOVA_LOG_INFO("End of restart call.");
        return JsonData::from_string("restart ok");

    }
    else if (input.method_name == "change_passwords")
    {
        NOVA_LOG_INFO("Starting change_passwords call");
        NOVA_LOG_INFO("changing permissions on conf.d");
        if (system("sudo chmod -R 777 /etc/redis/conf.d") == -1)
        {
            NOVA_LOG_INFO("Unable to change conf.d to 777");
            return JsonData::from_string("change_passwords fail :"
                                         "Error unable to chmod local.conf "
                                         "to 777");
        }
        std::fstream include_file;
        std::string file_contents;
        NOVA_LOG_INFO("Opening local.conf for reading/writing");

        include_file.open("/etc/redis/conf.d/local.conf");
        if (include_file.is_open())
        {
            NOVA_LOG_INFO("Getting rid of the password entry");
            while(!include_file.eof())
            {

                char buff[301];
                std::string tmp;
                include_file.getline(buff, 300);
                tmp = std::string(buff);
                if ((int)tmp.find("requirepass") == -1)
                {
                    if (tmp != "\n")
                    {
                        file_contents.append(tmp);
                        file_contents.append("\n");
                    }
                }
            }
            include_file.close();
            include_file.clear();
        }
        else
        {
            NOVA_LOG_INFO("Unable to open local.conf for reading/writing");
            return JsonData::from_string("change_passwords fail :"
                                         "Error unable open local.conf "
                                         "for reading/writing");
        }
        JsonArrayPtr array = input.args->get_array("users");
        JsonObjectPtr obj = array->get_object(0);
        file_contents.append("requirepass ");
        file_contents.append(obj->get_string("password"));
        NOVA_LOG_INFO("Removing the local.conf file");
        if (system("sudo rm /etc/redis/conf.d/local.conf") == -1)
        {
            NOVA_LOG_INFO("Unable to remove local.conf");
            return JsonData::from_string("change_passwords fail: "
                                         "unable to remove local.conf");
        }
        NOVA_LOG_INFO("Creating new local.conf");
        if (system("touch /etc/redis/conf.d/local.conf") == -1)
        {
            NOVA_LOG_INFO("Unable to create new local.conf");
            return JsonData::from_string("change_passwords fail: "
                                         "unable to make new local.conf");
        }
        include_file.open("/etc/redis/conf.d/local.conf");
        if (include_file.is_open())
        {
            include_file << file_contents;
            include_file.close();
            include_file.clear();
        }
        else
        {
            NOVA_LOG_INFO("Unable to open local.conf for reading/writing");
            return JsonData::from_string("change_passwords fail :"
                                         "Error unable open local.conf "
                                         "for reading/writing");

        }
        NOVA_LOG_INFO("chmoding /etc/redis/conf.d to 755");
        if (system("sudo chmod -R 755 /etc/redis/conf.d") == -1)
        {
            NOVA_LOG_INFO("Unable to return conf.d to previous permissions");
        }
        NOVA_LOG_INFO("chmowning /etc/redis/conf.d to 755");
        if (system("sudo chown -R root:root /etc/redis/conf.d") == -1)
        {
            NOVA_LOG_INFO("Unable to change ownership of conf.d to root:root");
        }
        NOVA_LOG_INFO("Connecting to redis");
        nova::redis::Client client();
        client.config_set("requirepass", obj->get_string("password"));
        return JsonData::from_string("change_passwords ok");
    }
    else if (input.method_name == "start_db_with_conf_changes")
    {
        NOVA_LOG_INFO("Entering start_db_with_conf_changes call.");
        NOVA_LOG_INFO("Connecting to redis instance.");
        nova::redis::Client client();
        const auto config_contents = input.args->get_string("config_contents");
        if (!client.config->write_new_config(config_contents))
        {
            NOVA_LOG_ERROR("Unable to write new config.");
            return JsonData::from_string("Unable to write new config.");
        }
        if (client.control->start() != 0)
        {
            NOVA_LOG_ERROR("Unable to start redis instance!");
            return JsonData::from_string("Unable to stop redis instance!");
        }
        NOVA_LOG_INFO("Redis instance started.");
        NOVA_LOG_INFO("End of start_db_with_conf_changes call.");
        return JsonData::from_string("start_db_with_conf_changes ok");
    }
    else if (input.method_name == "stop_db")
    {
        NOVA_LOG_INFO("Entering stop_db call.");
        NOVA_LOG_INFO("Connecting to redis instance.");
        nova::redis::Client client();
        NOVA_LOG_INFO("Stopping redis instance.");
        if (client.control->stop() != 0)
        {
            NOVA_LOG_ERROR("Unable to stop redis instance!");
            return JsonData::from_string("Unable to stop redis instance!");
        }
        NOVA_LOG_INFO("Redis instance stopped.");
        NOVA_LOG_INFO("End of stop_db call.");
        return JsonData::from_string("stop_db ok");
    }
    else
    {
        return JsonDataPtr();
    }
}


}}//end namespace nova::guest
