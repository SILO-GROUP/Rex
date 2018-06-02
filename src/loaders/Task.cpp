/*
    Examplar - An automation and testing framework.

    © SURRO INDUSTRIES and Chris Punches, 2017.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "Task.h"
#include <stdio.h>
#include <syslog.h>
#include "../sproc/Sproc.h"
#include "helpers.h"

/// Task_InvalidDataStructure - Exception thrown when a Task is defined with invalid JSON.
class Task_InvalidDataStructure: public std::runtime_error {
public:
    Task_InvalidDataStructure(): std::runtime_error("Task: Attempted to access a member of a Task that is not set.") {}
};

/// Task_InvalidDataStructure - Exception thrown when a Task is defined with invalid JSON.
class Task_NotReady: public std::runtime_error {
public:
    Task_NotReady(): std::runtime_error("Task: Attempted to access a unit of a Task that is not defined.") {}
};


/// Task_RequiredButFailedTask - Exception thrown when a Task fails but should not.
class TaskException: public std::exception
{
public:
    /** Constructor (C strings).
     *  @param message C-style string error message.
     *                 The string contents are copied upon construction.
     *                 Hence, responsibility for deleting the char* lies
     *                 with the caller.
     */
    explicit TaskException(const char* message):
            msg_(message)
    {
    }

    /** Constructor (C++ STL strings).
     *  @param message The error message.
     */
    explicit TaskException(const std::string& message):
            msg_(message)
    {}

    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~TaskException() throw (){}

    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a const char*. The underlying memory
     *          is in posession of the Exception object. Callers must
     *          not attempt to free the memory.
     */
    virtual const char* what() const throw (){
        return msg_.c_str();
    }

protected:
    /** Error message.
     */
    std::string msg_;
};


/// Task::Task() - Constructor for the Task class.  The Task is the building block of a Plan indicating of which Unit to
/// execute, and its dependencies on other units to have already been completed successfully.
Task::Task()
{
    // it hasn't executed yet.
    this->complete = false;

    // it hasn't been matched with a definition yet.
    this->defined = false;
}

/// Task::load_root() - loads json values to private members
///
/// \param loader_root - the Json::Value to populate from.
/// \param verbose - Whether to print verbose information to STDOUT.
void Task::load_root(Json::Value loader_root, bool verbose )
{
    if ( loader_root.isMember("name") ) {
        this->name = loader_root.get("name", "?").asString();
    }
    else {
        throw Task_InvalidDataStructure();
    }

    // fetch as Json::Value array obj
    Json::Value des_dep_root = loader_root.get("dependencies", 0);

    // iterate through each member of that obj
    for ( int i = 0; i < des_dep_root.size(); i++ ) {
        // add each string to dependencies
        if ( des_dep_root[i].asString() != "" ) {
            this->dependencies.push_back( des_dep_root[i].asString() );
            if ( verbose ) {
                std::ostringstream infostring;
                infostring << "Added dependency \"" << des_dep_root[i].asString() << "\" to task \""
                           << this->get_name() << "\"." << std::endl;

                syslog( LOG_INFO, infostring.str().c_str() );
                std::cout << infostring.str();
            }
        }
    }
}

/// Task::get_name - Retrieves the name of the current Task.
std::string Task::get_name()
{
    return this->name;
}

/// Task::load_definition - Loads a unit to a local member.  Used to tie Units to Tasks.
///
/// \param selected_unit - The unit to attach.
/// \param verbose - Whether to print to STDOUT.
void Task::load_definition( Unit selected_unit, bool verbose )
{
    this->definition = selected_unit;
    if ( verbose ) {
        std::ostringstream infostring;
        infostring << "Loaded definition \"" << selected_unit.get_name() << "\" for task \""
                   << this->get_name() << "\"." << std::endl;

        syslog( LOG_INFO, infostring.str().c_str() );
        std::cout << infostring.str();
    }
    this->defined = true;
}

/// Task::is_complete - Indicator if the task executed successfully.
bool Task::is_complete()
{
    return this->complete;
}


/// Task::mark_complete - Marks the task complete..
void Task::mark_complete()
{
    this->complete = true;
}


/// Task::get_dependencies - returns a pointer to the dependencies vector.
std::vector<std::string> Task::get_dependencies()
{
    return this->dependencies;
}

/// Task::has_definition - Indicator if the task has attached its definition from a Suite.
bool Task::has_definition()
{
    return this->defined;
}

/// Task::execute - execute a task's unit definition.
/// See the design document for what flow control needs to look like here.
/// \param verbose - Verbosity level - not implemented yet.
void Task::execute( Conf * configuration, bool verbose )
{
    // DUFFING - If Examplar is broken it's probably going to be in this block.
    // Somebody come clean this up, eh?

    // PREWORK
    // throw if unit not coupled to all necessary values since Task is stateful (yes, stateful is okay)
    std::ostringstream infostring;
    if ( ! this->has_definition() )
    {
        throw Task_NotReady();
    }

    // get the name
    std::string task_name = this->definition.get_name();
    // END PREWORK

    // get the target execution command
    std::string target_command = this->definition.get_target();

    // if we're in verbose mode, do some verbose things
    if ( verbose )
    {
/*
        infostring = std::ostringstream();
        infostring << "\tUsing unit \"" << task_name << "\"." << std::endl;
        syslog( LOG_INFO, infostring.str().c_str() );
        std::cout << infostring.str();
*/

        // check if context override
        if ( configuration->has_context_override() )
        {
            // if so, set the CWD.
            chdir( configuration->get_execution_context().c_str() );
            infostring = std::ostringstream();
            infostring << "\tExecution context: " << get_working_path() << std::endl;
            syslog(LOG_INFO, infostring.str().c_str() );
            std::cout << infostring.str();
        }

        infostring = std::ostringstream();
        infostring << "\tExecuting target \"" << target_command << "\"." << std::endl;
        syslog( LOG_INFO, infostring.str().c_str() );
        std::cout << infostring.str();
    }

    // a[0] execute target
    // TODO revise variable sourcing strategy
    int return_code = Sproc::execute( "source " + configuration->get_env_vars_file() + " && " + target_command );

    // **********************************************
    // d[0] Error Code Check
    // **********************************************
    if ( return_code == 0 )
    {
        // d[0].0 ZERO

        if ( verbose ) {
            infostring = std::ostringstream();
            infostring << "\tTarget " << task_name << " succeeded.  Marking as complete." << std::endl;
            syslog( LOG_INFO, infostring.str().c_str() );
            std::cout << infostring.str();
        }
        this->mark_complete();

        // a[1] NEXT
        return;
    }

    if ( return_code != 0 )
    {
        // d[0].1 NON-ZERO

        infostring = std::ostringstream();
        infostring << "\tTarget \"" << task_name << "\" failed with exit code " << return_code << "." << std::endl;

        syslog(LOG_ERR, infostring.str().c_str() );
        std::cerr << infostring.str();

        // **********************************************
        // d[1] Rectify Check
        // **********************************************
        if (! this->definition.get_rectify() )
        {
            // d[1].0 FALSE

            // **********************************************
            // d[2] Required Check
            // **********************************************
            if (! this->definition.get_required() )
            {
                // d[2].0 FALSE
                // a[2] NEXT
                infostring = std::ostringstream();
                infostring << "\tThis task is not required to continue the plan. Moving on." << std::endl;
                syslog(LOG_INFO, infostring.str().c_str() );
                std::cout << infostring.str();
                return;
            }

            if ( this->definition.get_required() )
            {
                // d[2].1 TRUE
                // a[3] EXCEPTION
                throw TaskException("Task \"" + task_name + "\" is required, and failed, and rectification is not enabled.");
            }
            // **********************************************
            // end - d[2] Required Check
            // **********************************************
        }


        if ( this->definition.get_rectify() )
        {
            // d[1].1 TRUE (Rectify Check)
            infostring = std::ostringstream();
            infostring << "\tRectification pattern is enabled for \"" << task_name << "\"." << std::endl;
            syslog( LOG_INFO, infostring.str().c_str() );
            std::cout << infostring.str();

            // a[4] Execute RECTIFIER
            std::string rectifier_command = this->definition.get_rectifier();

            infostring = std::ostringstream();
            infostring << "\tExecuting rectification: " << rectifier_command << "." << std::endl;
            syslog(LOG_INFO, infostring.str().c_str() );
            std::cout << infostring.str();

            int rectifier_error = Sproc::execute(  "source " + configuration->get_env_vars_file() + " && " + rectifier_command );

            // **********************************************
            // d[3] Error Code Check for Rectifier
            // **********************************************
            if ( rectifier_error != 0 )
            {
                // d[3].1 Non-Zero
                infostring = std::ostringstream();
                infostring << "\tRectification of \"" << task_name << "\" failed with exit code "
                           << rectifier_error << "." << std::endl;
                syslog( LOG_INFO, infostring.str().c_str() );

                std::cout << infostring.str();

                // **********************************************
                // d[4] Required Check
                // **********************************************
                if ( ! this->definition.get_required() ) {
                    // d[4].0 FALSE
                    // a[5] NEXT
                    infostring = std::ostringstream();
                    infostring << "\tThis task is not required to continue the plan. Moving on." << std::endl;
                    syslog(LOG_INFO, infostring.str().c_str() );
                    std::cout << infostring.str();
                    return;
                }

                if ( this->definition.get_required() )
                {
                    // d[4].1 TRUE
                    // a[6] EXCEPTION
                    throw TaskException("Task \"" + task_name + "\" is required, and failed, then rectified but rectification failed.");
                }
                // **********************************************
                // end - d[4] Required Check
                // **********************************************
            }

            // d[3] check exit code of rectifier
            if ( rectifier_error == 0 )
            {
                // d[3].0 Zero
                infostring = std::ostringstream();
                infostring << "\tRectification returned successfully." << std::endl;
                syslog( LOG_INFO, infostring.str().c_str() );
                std::cout << infostring.str();

                // a[7] Re-execute Target
                infostring = std::ostringstream();
                infostring << "\tRe-Executing target \"" << this->definition.get_target() << "\"." << std::endl;
                syslog( LOG_INFO, infostring.str().c_str() );
                std::cout << infostring.str();

                int retry_code = Sproc::execute( target_command );

                // **********************************************
                // d[5] Error Code Check
                // **********************************************
                if ( retry_code == 0 )
                {
                    // d[5].0 ZERO
                    // a[8] NEXT
                    infostring = std::ostringstream();
                    infostring << "\tRe-execution was successful." << std::endl;
                    syslog( LOG_INFO, infostring.str().c_str() );
                    std::cout << infostring.str();
                    return;
                }

                if ( retry_code != 0 )
                {
                    // d[5].1 NON-ZERO
                    infostring = std::ostringstream();
                    infostring << "\tRe-execution failed with exit code " << retry_code << "." << std::endl;
                    syslog(LOG_ERR, infostring.str().c_str() );
                    std::cerr << infostring.str();

                    // **********************************************
                    // d[6] Required Check
                    // **********************************************
                    if ( ! this->definition.get_required() )
                    {
                        // d[6].0 FALSE
                        // a[9] NEXT
                        infostring = std::ostringstream();
                        infostring << "\tThis task is not required to continue the plan. Moving on." << std::endl;
                        syslog(LOG_INFO, infostring.str().c_str() );
                        std::cout << infostring.str();

                        return;
                    }

                    if ( this->definition.get_required() )
                    {
                        // d[6].1 TRUE
                        // a[10] EXCEPTION
                        throw TaskException("Task \"" + task_name + "\" is required, and failed, then rectified but rectifier did not heal the condition causing the target to fail.  Cannot proceed with Plan.");
                    }
                    // **********************************************
                    // end - d[6] Required Check
                    // **********************************************
                }

            }
        }
        // **********************************************
        // end d[1] Rectify Check
        // **********************************************
    }
}
