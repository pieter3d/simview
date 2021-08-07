#include "simple_tokenizer.h"
#include <unordered_set>
namespace sv {
namespace {

bool IsKeyword(const std::string &s) {
  const static std::unordered_set<std::string> kKeywords = {
      "accept_on",
      "alias",
      "always",
      "always_comb",
      "always_ff",
      "always_latch",
      "and",
      "assert",
      "assign",
      "assume",
      "automatic",
      "before",
      "begin",
      "bind",
      "bins",
      "binsof",
      "bit",
      "break",
      "buf",
      "bufif0",
      "bufif1",
      "byte",
      "case",
      "casex",
      "casez",
      "cell",
      "chandle",
      "checker",
      "class",
      "clocking",
      "cmos",
      "config",
      "const",
      "constraint",
      "context",
      "continue",
      "cover",
      "covergroup",
      "coverpoint",
      "cross",
      "deassign",
      "default",
      "defparam",
      "design",
      "disable",
      "dist",
      "do",
      "edge",
      "else",
      "end",
      "endcase",
      "endchecker",
      "endclass",
      "endclocking",
      "endconfig",
      "endfunction",
      "endgenerate",
      "endgroup",
      "endinterface",
      "endmodule",
      "endpackage",
      "endprimitive",
      "endprogram",
      "endproperty",
      "endspecify",
      "endsequence",
      "endtable",
      "endtask",
      "enum",
      "event",
      "eventually",
      "expect",
      "export",
      "extends",
      "extern",
      "final",
      "first_match",
      "for",
      "force",
      "foreach",
      "forever",
      "fork",
      "forkjoin",
      "function",
      "generate",
      "genvar",
      "global",
      "highz0",
      "highz1",
      "if",
      "iff",
      "ifnone",
      "ignore_bins",
      "illegal_bins",
      "implements",
      "implies",
      "import",
      "incdir",
      "include",
      "initial",
      "inout",
      "input",
      "inside",
      "instance",
      "int",
      "integer",
      "interconnect",
      "interface",
      "intersect",
      "join",
      "join_any",
      "join_none",
      "large",
      "let",
      "liblist",
      "library",
      "local",
      "localparam",
      "logic",
      "longint",
      "macromodule",
      "matches",
      "medium",
      "modport",
      "module",
      "nand",
      "negedge",
      "nettype",
      "new",
      "nexttime",
      "nmos",
      "nor",
      "noshowcancelled",
      "not",
      "notif0",
      "notif1",
      "null",
      "or",
      "output",
      "package",
      "packed",
      "parameter",
      "pmos",
      "posedge",
      "primitive",
      "priority",
      "program",
      "property",
      "protected",
      "pull0",
      "pull1",
      "pulldown",
      "pullup",
      "pulsestyle_ondetect",
      "pulsestyle_onevent",
      "pure",
      "rand",
      "randc",
      "randcase",
      "randsequence",
      "rcmos",
      "real",
      "realtime",
      "ref",
      "reg",
      "reject_on",
      "release",
      "repeat",
      "restrict",
      "return",
      "rnmos",
      "rpmos",
      "rtran",
      "rtranif0",
      "rtranif1",
      "s_always",
      "s_eventually",
      "s_nexttime",
      "s_until",
      "s_until_with",
      "scalared",
      "sequence",
      "shortint",
      "shortreal",
      "showcancelled",
      "signed",
      "small",
      "soft",
      "solve",
      "specify",
      "specparam",
      "static",
      "string",
      "strong",
      "strong0",
      "strong1",
      "struct",
      "super",
      "supply0",
      "supply1",
      "sync_accept_on",
      "sync_reject_on",
      "table",
      "tagged",
      "task",
      "this",
      "throughout",
      "time",
      "timeprecision",
      "timeunit",
      "tran",
      "tranif0",
      "tranif1",
      "tri",
      "tri0",
      "tri1",
      "triand",
      "trior",
      "trireg",
      "type",
      "typedef",
      "union",
      "unique",
      "unique0",
      "unsigned",
      "until",
      "until_with",
      "untyped",
      "use",
      "uwire",
      "var",
      "vectored",
      "virtual",
      "void",
      "wait",
      "wait_order",
      "wand",
      "weak",
      "weak0",
      "weak1",
      "while",
      "wildcard",
      "wire",
      "with",
      "within",
      "wor",
      "xnor",
      "xor ",
  };
  return kKeywords.find(s) != kKeywords.end();
}

bool IsInternalIdentifierCharacater(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || (c == '_') || (c == '$');
}

// Numeric digits and the $ symbol are not part of an identifier if they
// are the first characters.
bool IsLeadingIdentifierCharacater(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

} // namespace

void SimpleTokenizer::ProcessLine(const std::string &s) {
  int start_pos = 0;
  bool in_identifier = false;
  bool in_escaped_identifier = false;
  for (int i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (in_block_comment_) {
      if ((i > 0 && c == '/' && s[i - 1] == '*') || (i == s.size() - 1)) {
        comments_[line_num_].push_back({start_pos, i});
        in_block_comment_ = c != '/';
      }
    } else if (in_string_literal_) {
      if (c == '"' && (i == 0 || s[i - 1] != '\\')) in_string_literal_ = false;
    } else if (in_escaped_identifier) {
      if (c == ' ' || c == '\t' || i == s.size() - 1) {
        in_escaped_identifier = false;
        // Skip identifiers preceded by a period. Those are named port or
        // parameter connections that belong to a different scope than the
        // identifiers in this file.
        if (last_token_was_dot_) {
          last_token_was_dot_ = false;
          continue;
        }
        identifiers_[line_num_].push_back(
            {start_pos, s.substr(start_pos, i - start_pos)});
      }
    } else if (in_identifier) {
      if (!IsInternalIdentifierCharacater(c) || i == s.size() - 1) {
        in_identifier = false;
        int last_pos = (IsInternalIdentifierCharacater(c) && i == s.size() - 1)
                           ? i
                           : (i - 1);
        // Skip macros and compiler directives.
        if (start_pos > 0 && s[start_pos - 1] == '`') continue;
        // Skip numerical literals
        if (start_pos > 0 && s[start_pos - 1] == '\'') continue;
        // Skip identifiers preceded by a period. Those are named port or
        // parameter connections that belong to a different scope than the
        // identifiers in this file.
        if (last_token_was_dot_) {
          last_token_was_dot_ = false;
          continue;
        }
        auto text = s.substr(start_pos, last_pos - start_pos + 1);
        if (IsKeyword(text)) {
          keywords_[line_num_].push_back({start_pos, last_pos});
        } else {
          identifiers_[line_num_].push_back({start_pos, text});
        }
      }
    } else if (i > 0 && c == '/' && s[i - 1] == '/') {
      comments_[line_num_].push_back({i - 1, s.size() - 1});
      break;
    } else if (i > 0 && c == '*' && s[i - 1] == '/') {
      in_block_comment_ = true;
      start_pos = i - 1;
    } else if (c == '\\') {
      start_pos = i + 1; // The backslash is not part of the identifier.
      in_escaped_identifier = true;
    } else if (c == '"') {
      in_string_literal_ = true;
    } else if (IsLeadingIdentifierCharacater(c)) {
      start_pos = i;
      in_identifier = true;
    } else if (c == '.') {
      last_token_was_dot_ = true;
    } else if (c != ' ' && c != '\t') {
      last_token_was_dot_ = false;
    }
  }
  line_num_++;
}
} // namespace sv
