#pragma once

#include "vcd_parser.gen.hpp"

/*
 * Flex requires that yyFlexLexer is equivalent to
 * the lexer class we are using, before we include
 * FlexLexer.h.  See 'Generating C++ Scanners' in 
 * the Flex manual.
 */
#if ! defined(yyFlexLexerOnce)

#undef yyFlexLexer
/*
 * Flex doesn't support C++ namespaces, so
 * it is faked using the '%option prefix' defined
 * in vcd_scanner.l, for some reason Flex also prefixes
 * the class name with Flex.
 *
 * So:
 * %option prefix=VcdParse_
 * %option yyclass=Lexer
 * becomes 'VcdParse_FlexLexer'
 */
#define yyFlexLexer VcdParse_FlexLexer
#include <FlexLexer.h>

#endif

/*
 * The YY_DECL is used by flex to specify the signature of the main
 * lexer function.
 *
 * We re-define it to something reasonable
 */
#undef YY_DECL
#define YY_DECL vcdparse::Parser::symbol_type vcdparse::Lexer::next_token()

/*
 * Bison generated location tracking
 */
#include "location.hh"

namespace vcdparse {

class Lexer : private yyFlexLexer {
    //We use private inheritance to hide the flex
    //implementation details from anyone using Lexer
    public:
        vcdparse::Parser::symbol_type next_token();

        using yyFlexLexer::switch_streams;

        location get_loc() { return loc_; }
        void set_loc(location& loc) { loc_ = loc; }
    private:
        location loc_; 
};

} //vcdparse
