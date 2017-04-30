#include "Suite.h"

Suite::Suite( std::string filename ): JLoader( filename )
/*  Suite loads a file and deserializes the Unit JSON object to Unit types as a vector member
 *  Suite { vector<Unit> }
 */
{
    Json::Value raw_units = this->get_root()["units"];

    for ( int index = 0; index < raw_units.size(); index++ )
    {
        this->units.push_back( Unit( raw_units[ index ] ) );
    }
};

Unit Suite::get_unit(std::string provided_name)
/*
 * returns a unit from a unitholder object by name
 * this will need reworked.  maybe should return int, populate a pointer.
 * error handling is the concern here.
 */
{
    Unit * returnable;
    bool foundMatch = false;

    for ( int i = 0; i < this->units.size(); i++ )
    {
        std::string unit_name = this->units[i].get_name();
        if ( unit_name == provided_name )
        {
            returnable = & this->units[i];
            foundMatch = true;
            break;
        }
    }
    if (! foundMatch )
    {
        std::cerr << "Unit name \"" << provided_name << "\" was referenced but not defined!" << std::endl;
        std::exit(1);
    }

    return * returnable;
}
