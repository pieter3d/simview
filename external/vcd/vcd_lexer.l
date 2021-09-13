%{
    #include <iostream>
    #include <sstream>
    #include <cassert>
    #include "vcd_lexer.hpp"
    #include "vcd_parser.gen.hpp"
    #include "location.hh"

    using std::cout;
    using std::stringstream;

    //Called everytime a token matches
    //This advances the end of the location to the end
    //of the token.
    #define YY_USER_ACTION loc_.columns(YYLeng());

%}

/* Generate a C++ lexer */
%option c++

/* track line numbers*/
%option yylineno 

/* No lexing accross files */
%option noyywrap

/* unistd.h doesn't exist on windows */
%option nounistd

/* isatty() doesn't exist on windows */
%option never-interactive

/* no default rule to echo unrecongaized tokens to output */
%option nodefault

/* 
 * We want the yylex() member function to be implemented
 * in our class, rather than in the FlexLexer parent class.
 *
 * This allows us to acces member vairables of the lexer class
 * in side the flex rules below.
 */
%option yyclass="Lexer"

/*
 * Use a prefix to avoid name clashes with other
 * flex lexers
 */
%option prefix="VcdParse_" 

/* Character classes */
ALPHA [a-zA-Z]
SYMBOL [-_~|*/\[\]\.\{\}^+$/#"%&'!(),:;]
DIGIT [0-9]
LOGIC_VALUE [01zZxX]
ID_VALUE [^\n\r \t]
WS [ \t]
ENDL (\n|\n\r)

/* Lexer states */
%x multiline
%x var
%x changevalues
%x varid
%x change_varid


%%

%{
    //Run everytime yylex is called
    loc_.step(); //Move begining of location to end
%}

<INITIAL,changevalues,var,varid>
{WS}+                                           { /* skip white space */ loc_.step(); }

<*>
{ENDL}                                          { /* skip end-of-line */ loc_.lines(1); loc_.step(); }


$end                                            {
                                                     /*cout << "END\n";  */
                                                    return vcdparse::Parser::make_END(loc_); 
                                                }

$date                                           {
                                                     /*cout << "DATE\n";  */
                                                    BEGIN(multiline); 
                                                    return vcdparse::Parser::make_DATE(loc_); 
                                                }
$version                                        { 
                                                    /* cout << "VERSION\n"; */ 
                                                    BEGIN(multiline); 
                                                    return vcdparse::Parser::make_VERSION(loc_); 
                                                }
$timescale                                      {
                                                     /*cout << "TIMESCALE\n";  */
                                                    BEGIN(multiline); 
                                                    return vcdparse::Parser::make_TIMESCALE(loc_); 
                                                }
<multiline>.*                                   { 
                                                    /*cout << "Multiline: " << YYText() << "\n"; */
                                                    BEGIN(INITIAL);
                                                    return vcdparse::Parser::make_Multiline(YYText(), loc_); 
                                                }
$scope                                          { 
                                                     /*cout << "SCOPE\n";  */
                                                    return vcdparse::Parser::make_SCOPE(loc_); 
                                                }
module                                          { 
                                                     /*cout << "MODULE\n";  */
                                                    return vcdparse::Parser::make_MODULE(loc_); 
                                                }
$upscope                                        { 
                                                     /*cout << "UPSCOPE\n";  */
                                                    return vcdparse::Parser::make_UPSCOPE(loc_); 
                                                }
$enddefinitions                                 { 
                                                     /*cout << "ENDDEFINITIONS\n";  */
                                                    return vcdparse::Parser::make_ENDDEFINITIONS(loc_); 
                                                }

$var                                            { 
                                                    /*cout << "VAR\n";*/
                                                    BEGIN(var); 
                                                    return vcdparse::Parser::make_VAR(loc_); 
                                                }
<var>wire                                       { 
                                                    /*cout << "WIRE\n"; */
                                                    return vcdparse::Parser::make_WIRE(loc_); 
                                                }
<var>reg                                        { 
                                                    /*cout << "REG\n"; */
                                                    return vcdparse::Parser::make_REG(loc_); 
                                                }
<var>parameter                                  { 
                                                    /*cout << "REG\n"; */
                                                    return vcdparse::Parser::make_PARAMETER(loc_); 
                                                }
<var>$end                                       { 
                                                    /*cout << "END\n"; */
                                                    BEGIN(INITIAL); 
                                                    return vcdparse::Parser::make_END(loc_); 
                                                }
<var>{DIGIT}+                                   {
                                                    size_t val;
                                                    stringstream ss(YYText());
                                                    ss >> val;

                                                    if(ss.fail() || !ss.eof()) {
                                                        stringstream msg_ss;
                                                        msg_ss << "Failed to parse var width '" << YYText() << "'";
                                                        throw vcdparse::ParseError(msg_ss.str(), loc_);
                                                    }
                                                    BEGIN(varid);
                                                    /*cout << "Time: " << val << " '" << YYText() << "'\n";*/
                                                    return vcdparse::Parser::make_Integer(val, loc_); 
                                                }
({ALPHA}|{SYMBOL})({ALPHA}{DIGIT}{SYMBOL})+     { 
                                                    /*cout << "Var String: " << YYText() << "\n"; */
                                                    return vcdparse::Parser::make_String(YYText(), loc_); 
                                                }

<varid>{ID_VALUE}+                               {
                                                    BEGIN(INITIAL);
                                                    return vcdparse::Parser::make_VarId(YYText(), loc_);
                                                }


$dumpvars                                       { 
                                                     /*cout << "DUMPVARS\n";  */
                                                    BEGIN(changevalues); 
                                                    return vcdparse::Parser::make_DUMPVARS(loc_); 
                                                }
<changevalues>$end                              { 
                                                    /*cout << "END\n"; */
                                                    /*BEGIN(INITIAL); */
                                                    return vcdparse::Parser::make_END(loc_); 
                                                }
<changevalues>
^1                                              { 
                                                     /*cout << "Logic_val: " << YYText() << "\n"; */
                                                    BEGIN(change_varid);
                                                    return vcdparse::Parser::make_LOGIC_ONE(loc_); 
                                                }
<changevalues>
^0                                              { 
                                                     /*cout << "Logic_val: " << YYText() << "\n"; */
                                                    BEGIN(change_varid);
                                                    return vcdparse::Parser::make_LOGIC_ZERO(loc_); 
                                                }
<changevalues>
^(x|X)                                          { 
                                                     /*cout << "Logic_val: " << YYText() << "\n"; */
                                                    BEGIN(change_varid);
                                                    return vcdparse::Parser::make_LOGIC_UNKOWN(loc_); 
                                                }
<changevalues>
^(z|Z)                                          { 
                                                     /*cout << "Logic_val: " << YYText() << "\n"; */
                                                    BEGIN(change_varid);
                                                    return vcdparse::Parser::make_LOGIC_HIGHZ(loc_); 
                                                }
<changevalues>
^b{DIGIT}+                                      {
                                                    const char* begin = YYText() + 1;
                                                    const char* end = YYText() + YYLeng();
                                                    std::string str(begin, end);
                                                    /*cout << "Logic_val: " << YYText() << "\n";*/
                                                    BEGIN(change_varid);
                                                    return vcdparse::Parser::make_BitString(str, loc_);
                                                }

<change_varid>
{ID_VALUE}+$                                    {
                                                    /*cout << "ID: '" << YYText() << "'\n";*/
                                                    BEGIN(changevalues);
                                                    return vcdparse::Parser::make_VarId(YYText(), loc_); 
                                                }
<*>
^#{DIGIT}+                                    {
                                                    const char* begin = YYText() + 1;
                                                    const char* end = YYText() + YYLeng();
                                                    std::string str(begin, end);
                                                    stringstream ss;
                                                    ss << str;
                                                    size_t val;
                                                    ss >> val;

                                                    if(ss.fail() || !ss.eof()) {
                                                        stringstream msg_ss;
                                                        msg_ss << "Failed to parse Time value '" << YYText() << "'";
                                                        throw vcdparse::ParseError(msg_ss.str(), loc_);
                                                    }
                                                    /*cout << "Time: " << YYText() << "\n";*/
                                                    return vcdparse::Parser::make_Time(val, loc_); 
                                                }

({ALPHA}|{SYMBOL})({ALPHA}|{DIGIT}|{SYMBOL})* { 
                                                    /*cout << "String: " << YYText() << "\n"; */
                                                    return vcdparse::Parser::make_String(YYText(), loc_); 
                                                }

<*>
.                                               { 
                                                    /*cout << "Unrecognized: " << YYText() << "\n";*/
                                                    stringstream ss;
                                                    ss << "Unexpected character '" << YYText() << "'";
                                                    throw vcdparse::ParseError(ss.str(), loc_);
                                                }

<<EOF>>                                         { 
                                                    /*cout << "EOF\n";*/
                                                    return vcdparse::Parser::make_EOF(loc_);
                                                }

%%
