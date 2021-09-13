#pragma once

#include <iosfwd>
#include <memory>
#include <set>

#include "vcd_data.hpp"

namespace vcdparse {

//Forward delcarations
class Lexer;
class Parser;
class ParseError;

//Class for loading an SDF file.
//
//The vcd file can be parsed using load(), which returns true
//if successful - after which the loaded data can be accessed via 
//get_vcd_data().
//
//The virtual method on_error() can be overriding to control
//error handling. The default simply prints out an error message,
//but it could also be defined to (re-)throw an exception.
class Loader {

    public:
        Loader();
        ~Loader();

        bool load(std::string filename);
        bool load(std::istream& is, std::string filename="<inputstream>");

        const VcdData& get_vcd_data() { return vcd_data_; };

    protected:
        virtual void on_error(ParseError& error);

    private:
        Var::Id generate_var_id(std::string str_id);
    private:
        friend Parser;
        std::string filename_;
        std::unique_ptr<Lexer> lexer_;
        std::unique_ptr<Parser> parser_;

        bool pre_allocate_time_values_ = true;

        VcdData vcd_data_;
        std::vector<std::string> current_scope_;
        size_t curr_time_;
        size_t change_count_;

        VcdData::TimeValues time_values_;

        std::map<std::string,Var::Id> var_str_to_id_;
        Var::Id max_var_id_ = 0;

};

} //vcdparse
