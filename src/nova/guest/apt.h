#ifndef __NOVA_GUEST_APT_H
#define __NOVA_GUEST_APT_H

#include "guest.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>


namespace nova { namespace guest { namespace apt {


    /** Calls apt-get and other Debian package manager commands. */
    class AptGuest : boost::noncopyable {

        public:
            /* Creates a new instance.
             * "with_sudo" - If true, "sudo" is used for all commands.
             * "self_package_name" - The name of this package.
             * "self_update_time_out" - The time waited after the update call
             * before the assumption is made that the command was executed
             * incorrectly. */
            AptGuest(bool with_sudo, const char * self_package_name,
                     int self_update_time_out);

            /** Attempts to fix apt. */
            void fix(double time_out);

            /** Install the given package. If apt-get output is not received
             *  for the duration of time_out, an exception is raised. */
            void install(const char * package_name, const double time_out);

            /** Updates this very program. */
            void install_self_update();

            /** Remove the given package. If apt-get output is not received for
             *  the duration of time_out, an exception is raised. */
            void remove(const char * package_name, const double time_out);

            /** Updates the cache on this box. If output is not received for the
             *  duration of time_out, an exception is raised. */
            void update(const boost::optional<double> time_out=boost::none);

            /** Find the version of the given package. Returns the string
             *  name of the package or boost::none if the package is not
             *  installed. */
            boost::optional<std::string> version(const char * package_name,
                                                 const double time_out=30);

            /** Change the preferences file. */
            void write_repo_files(
              const boost::optional<std::string> & preferences_file,
              const boost::optional<std::string> & sources_file,
              const boost::optional<double> time_out=boost::none);

        private:

            pid_t _install_new_self();

            void resilient_remove(const char * package_name,
                                  const double time_out);

            void write_file(const char * name, const char * file_ext,
                            const boost::optional<std::string> & file_contents,
                            const boost::optional<double> time_out);

            std::string self_package_name;
            int self_update_time_out;
            bool with_sudo;
    };

    typedef boost::shared_ptr<AptGuest> AptGuestPtr;


    class AptException : public std::exception {

        public:
            enum Code {
                ADMIN_LOCK_ERROR = 0,
                COULD_NOT_FIX = 10,
                ERROR_WRITING_PREFERENCES = 100,
                GENERAL = 20,
                PACKAGE_NOT_FOUND = 30,
                PACKAGE_STATE_ERROR = 40,
                PERMISSION_ERROR = 50,
                PROCESS_CLOSE_TIME_OUT = 60,
                PROCESS_TIME_OUT = 70,
                UNEXPECTED_PROCESS_OUTPUT = 80,
                UPDATE_FAILED = 90
            };

            AptException(const Code code) throw();

            virtual ~AptException() throw();

            virtual const char * what() const throw();

            const Code code;

    };


    class AptMessageHandler : public MessageHandler {

        public:
          AptMessageHandler(AptGuestPtr apt_guest);

          virtual nova::JsonDataPtr handle_message(const GuestInput & input);

        private:
          AptMessageHandler(const AptMessageHandler &);
          AptMessageHandler & operator = (const AptMessageHandler &);

          AptGuestPtr apt_guest;
    };

} } }  // end namespace

#endif //__NOVA_GUEST_APT_H
