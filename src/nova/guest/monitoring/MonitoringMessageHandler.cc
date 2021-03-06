#include "nova/guest/monitoring/monitoring.h"
#include "nova/guest/monitoring/status.h"
#include "nova/guest/GuestException.h"
#include "nova/guest/guest.h"
#include "nova/guest/apt.h"
#include <boost/foreach.hpp>
#include "nova/Log.h"
#include <sstream>
#include <string>
#include <boost/tuple/tuple.hpp>

using nova::json_string;
using nova::JsonData;
using nova::JsonDataPtr;
using nova::json_obj;
using nova::JsonObjectBuilder;
using nova::Log;
using nova::JsonData;
using nova::JsonDataPtr;
using nova::JsonObject;
using nova::guest::GuestException;
using nova::guest::apt::AptGuest;
using nova::guest::apt::AptGuestPtr;
using boost::optional;
using std::string;
using namespace boost;

namespace nova { namespace guest { namespace monitoring {

namespace {

    JsonObjectBuilder monitoring_status_to_json(const std::string & version,
                                                MonitoringStatus::Status status) {
        return json_obj(
            "version", version,
            "status", MonitoringStatus::status_name(status)
        );
    }

} // end of anonymous namespace

MonitoringMessageHandler::MonitoringMessageHandler(
    AptGuestPtr apt,
    MonitoringManagerPtr monitoring)
:   apt(apt),
    monitoring(monitoring)
{
}

JsonDataPtr MonitoringMessageHandler::handle_message(const GuestInput & input) {
    if (input.method_name == "install_and_configure_monitoring_agent") {
        NOVA_LOG_DEBUG("handling the install_and_configure_monitoring_agent method");
        const auto monitoring_token = input.args->get_string("monitoring_token");
        const auto monitoring_endpoints = input.args->get_string("monitoring_endpoints");
        monitoring->install_and_configure_monitoring_agent(*apt, monitoring_token, monitoring_endpoints);
        return JsonData::from_null();
    } else if (input.method_name == "remove_monitoring_agent") {
        NOVA_LOG_DEBUG("handling the remove_monitoring_agent method");
        monitoring->remove_monitoring_agent(*apt);
        return JsonData::from_null();
    } else if (input.method_name == "update_monitoring_agent") {
        NOVA_LOG_DEBUG("handling the update_monitoring_agent method");
        monitoring->update_monitoring_agent(*apt);
        return JsonData::from_null();
    } else if (input.method_name == "start_monitoring_agent") {
        NOVA_LOG_DEBUG("handling the start_monitoring_agent method");
        monitoring->start_monitoring_agent();
        return JsonData::from_null();
    } else if (input.method_name == "stop_monitoring_agent") {
        NOVA_LOG_DEBUG("handling the stop_monitoring_agent method");
        monitoring->stop_monitoring_agent();
        return JsonData::from_null();
    } else if (input.method_name == "restart_monitoring_agent") {
        NOVA_LOG_DEBUG("handling the restart_monitoring_agent method");
        monitoring->restart_monitoring_agent();
        return JsonData::from_null();
    } else if (input.method_name == "get_monitoring_status") {
        NOVA_LOG_DEBUG("handling the get_monitoring_status method");
        MonitoringStatus mon_status;
        auto result = mon_status.get_monitoring_status(*apt);
        NOVA_LOG_DEBUG("formating data from get_monitoring_status");
        if (result) {
            JsonDataPtr rtn(new JsonObject(
                monitoring_status_to_json(result->get<0>(),
                                          result->get<1>())
            ));
            return rtn;
        } else {
            return JsonData::from_null();
        }
    } else {
        return JsonDataPtr();
    }
}



} } } // end namespace nova::guest::monitoring
