#include "vcd_loader.h"
#include <fstream>

#include "location.hh"
#include "vcd_lexer.h"
#include "vcd_parser.gen.h"

namespace vcdparse {

Loader::Loader()
    : filename_("") // Initialize the filename
      ,
      lexer_(new Lexer()), parser_(new Parser(*lexer_, *this)) {}

// We need to declare the destructor in the .cpp file,
// since to destruct the unique_ptr's we also need the
// full definitions of Lexer and Parser
Loader::~Loader() {}

bool Loader::load(std::string filename) {
  std::ifstream is(filename);
  return load(is, filename);
}

bool Loader::load(std::istream &is, std::string filename) {
  assert(is.good());

  // Reset
  vcd_data_ = VcdData();
  time_values_.clear();
  curr_time_ = 0;
  change_count_ = 0;

  // As a memory optimization can we pre-allocate space for the time-values.
  // This avoids having to grow the time-values vector dynamically, avoiding the
  // worst-case doubling in memory consumption
  //
  // We pre-count the number of lines that are potentially time-values to
  // determine how much space to allocate
  if (pre_allocate_time_values_) {
    size_t line_count = 0;
    std::string line;
    while (std::getline(is, line)) {
      // Only count lines that are not timestamps (#) or header ($)
      // this will be a (slightly) pessimistic count which is OK
      if (line[0] != '#' && line[0] != '$') {
        line_count++;
      }
    }

    std::cout << "Reserving space for " << line_count << " time values"
              << std::endl;
    time_values_.reserve(line_count);

    // Rest stream to beginning
    is.clear();
    is.seekg(0);
  }

  // Update the filename for location references
  filename_ = filename;

  // Point the lexer at the new input
  lexer_->switch_streams(&is);

  // Initialize locations with filename
  auto pos = position(&filename_);
  auto loc = location(pos, pos);
  lexer_->set_loc(loc);

  int retval;
  try {

    // Do the parsing
    retval = parser_->parse();

  } catch (ParseError &error) {
    // Users can re-define on_error if they want
    // to do something else (like re-throw)
    on_error(error);
    return false;
  }

  // Bision returns 0 if successful
  return (retval == 0);
}

Var::Id Loader::generate_var_id(std::string str_id) {
  Var::Id id = -1;
  auto iter = var_str_to_id_.find(str_id);
  if (iter == var_str_to_id_.end()) {
    // Create ID
    id = max_var_id_;
    var_str_to_id_[str_id] = id;
    max_var_id_++;
  } else {
    // Found
    id = iter->second;
  }
  return id;
}

void Loader::on_error(ParseError &error) {
  // Default implementation, just print out the error
  std::cout << "VCD Error " << error.loc() << ": " << error.what() << "\n";
}

} // namespace vcdparse
