%debug
%defines
%locations

%{

#include "common.h"
#include "scanner.h"
#include "symbol_tab.h"
#include "name_context.h"

FILE* outfile = NULL;
extern const char* file_name;

#define EMIT(fmt, ...) fprintf(outfile, fmt, ##__VA_ARGS__)
#define EMIT_LINE() \
    fprintf(outfile, "#line %d \"%s\"\n", get_line_no(), get_file_name())

    /*
     * TODO: Add syntax for lists and dicts. Add infrastructure for strings as usrtypes.
     */

%}

%union {
    char* str;
    double num;
};

%token BREAK CASE CONTINUE CONST DEFAULT
%token DO ELSE FOR IF RETURN SWITCH IMPORT
%token NAMESPACE CLASS STRUCT WHILE
%token NUMBER
%token NOTHING STRTYPE
%token BOOLTYPE TRUE FALSE IN YIELD EXIT
%token EQU NEQU LORE GORE OR AND
%token TRY EXCEPT RAISE CTOR DTOR
%token PUBLIC PRIVATE PROTECTED MARKER
%token<str> SYMBOL
%token<num> LNUM
%token<str> STRG

%type<str> namespace_name class_name

%define parse.error verbose
%locations

%right '='
%left OR
%left AND
%left EQU NEQU
%left LORE GORE '<' '>'
%left '+' '-'
%left '*' '/' '%'
%left NEGATE

%%

module
    : {
        // initialize the parser here
        EMIT("// before everything\n");
        initSymbolTab();
        initNameContext();
        pushNameContext("root");
    } module_list {
        // emit things after the input
        EMIT("// after everything\n");
        EMIT_LINE();
        // tear down the parser here
    }
    ;

module_list
    : module_item
    | module_list module_item
    ;

module_item
    : import_statement
    | module_definition_item
    ;

module_definition_item
    : class_definition
    | data_definition
    | func_definition
    | namespace_definition
    | preproc_marker
    ;

module_definition_list
    : module_definition_item
    | module_definition_list module_definition_item
    ;

preproc_marker
    : MARKER LNUM STRG {
            EMIT("#line %d \"%s\"\n", (int)$2, $3);
            file_name = _copy_str($3);
        }
    ;

compound_name
    : SYMBOL { printf("symbol: %s\n", $1); }
    | compound_name '.' SYMBOL { printf("symbol: %s\n", $3); }
    ;

import_statement
    : IMPORT SYMBOL {}
    ;

namespace_name
    : NAMESPACE SYMBOL {
            EMIT_LINE();
            EMIT("// namespace: %s\n", $2);
            $$ = $2;
            pushNameContext($2);
        }
    ;

namespace_definition
    : namespace_name '{' module_definition_list '}' {
            popNameContext();
        }
    | namespace_name '{' '}' {
            EMIT("// context: %s\n", getNameContext());
            popNameContext();
            EMIT("// context: %s\n", getNameContext());
        }
    ;

class_name
    : CLASS SYMBOL {
            EMIT_LINE();
            EMIT("// class: %s\n", $2);
            addSymbol(createName($2), CLASS, NULL);
            $$ = $2;
        }
    ;

class_definition
    : class_name '{' class_definition_list '}' {}
    | class_name '{' '}' {}
    ;

class_definition_list
    : class_definition_item
    | class_definition_list class_definition_item
    ;

scope_operator
    : PRIVATE {}
    | PUBLIC {}
    | PROTECTED {}
    ;

class_definition_item
    : symbol_declaration
    | func_declaration
    | struct_declaration
    | scope_operator
    | constructor_decl
    | destructor_decl
    ;

type_name
    : NUMBER {}
    | STRTYPE { /* TODO: Make this a compound type, like a class */ }
    | BOOLTYPE {}
    | NOTHING {}
    | compound_name {}
    ;

symbol_type
    : type_name SYMBOL {}
    ;

constructor_decl
    : CTOR {} '(' parameter_decl_list ')' {}
    ;

destructor_decl
    : DTOR {}
    ;

struct_declaration
    : STRUCT SYMBOL {} '{' struct_body '}' {}
    ;

struct_body
    : symbol_declaration
    | struct_body symbol_declaration
    ;

symbol_declaration
    : symbol_type {}
    | CONST symbol_type {}
    ;

parameter_decl_list
    : /* empty */
    | symbol_type {}
    | parameter_decl_list ',' symbol_type {}
    ;

func_declaration
    : symbol_type {} '(' parameter_decl_list ')' {}
    ;

data_definition
    : symbol_declaration
    | symbol_declaration '=' expression {}
    ;

func_definition_name
    : type_name compound_name { printf("function def\n"); }
    ;

method_definition_name
    : func_definition_name ':' SYMBOL {}
    ;

ctor_definition_name
    : compound_name ':' CTOR {}
    ;

dtor_definition_name
    : compound_name ':' DTOR {}
    ;

func_definition
    : func_definition_name '(' parameter_decl_list ')' '{' func_body_statement_list '}' { printf("here1\n"); }
    | func_definition_name '(' parameter_decl_list ')' '{' '}' { printf("here2\n"); }
    | method_definition_name '(' parameter_decl_list ')' '{' '}' {}
    | method_definition_name '(' parameter_decl_list ')' '{' func_body_statement_list '}' {}
    | ctor_definition_name '(' parameter_decl_list ')' '{' '}' {}
    | ctor_definition_name '(' parameter_decl_list ')' '{' func_body_statement_list '}' {}
    | dtor_definition_name '{' '}' {}
    | dtor_definition_name '{' func_body_statement_list '}' {}
    ;

func_body_statement
    : data_definition {}
    | compound_name '=' expression {}
    | if_statement {}
    | for_statement {}
    | while_statement {}
    | switch_statement {}
    | do_statement {}
    | try_statement {}
    | func_reference {}
    | RAISE '(' expression ')' {}
    | RETURN {}
    | RETURN '(' expression ')' {}
    | EXIT '(' expression ')' {}
    ;

func_body_statement_list
    : func_body_statement
    | func_body_statement_list func_body_statement
    ;

loop_body_statement
    : func_body_statement
    | BREAK {}
    | CONTINUE {}
    | YIELD {}
    ;

loop_body_statement_list
    : loop_body_statement
    | loop_body_statement_list loop_body_statement
    ;

func_reference
    : compound_name '(' expression_list ')' {}
    ;

except_clause
    : EXCEPT '(' compound_name ')' '{' func_body_statement_list '}' {}
    ;

except_clause_intermediate_list
    : except_clause
    | except_clause_intermediate_list except_clause
    ;

except_clause_final
    : EXCEPT '(' ')' '{' func_body_statement_list '}' {}
    | EXCEPT '{' func_body_statement_list '}' {}
    ;

except_clause_list
    : except_clause_intermediate_list
    | except_clause_intermediate_list except_clause_final
    | except_clause_final
    ;

try_statement
    : TRY '{' func_body_statement_list '}' except_clause_list{}
    ;

else_clause
    : ELSE '(' expression ')' '{' func_body_statement_list '}' {}
    ;

else_clause_intermediate_list
    : else_clause
    | else_clause_intermediate_list else_clause
    ;

else_clause_final
    : ELSE '(' ')' '{' func_body_statement_list '}' {}
    | ELSE '{' func_body_statement_list '}' {}
    ;

else_clause_list
    : else_clause_intermediate_list
    | else_clause_intermediate_list else_clause_final
    | else_clause_final
    ;

if_clause
    : IF '(' expression ')' '{' func_body_statement_list '}' {}
    ;

if_statement
    : if_clause
    | if_clause else_clause_list
    ;

for_statement
    : FOR '(' compound_name IN expression ')' '{' loop_body_statement_list '}'
    ;

while_statement
    : WHILE '(' expression ')' '{' loop_body_statement_list '}'
    ;

case_clause
    : CASE constant_expression '{' func_body_statement_list '}' {}
    ;

case_clause_intermediate_list
    : case_clause
    | case_clause_intermediate_list case_clause
    ;

case_clause_list
    : case_clause_intermediate_list
    | case_clause_intermediate_list DEFAULT '{' func_body_statement_list '}' {}
    ;

switch_statement
    : SWITCH '(' compound_name ')' '{' case_clause_list '}' {}
    ;

do_statement
    : DO '{' loop_body_statement_list '}' WHILE '(' expression ')'
    ;

constant_expression
    : LNUM {}
    | TRUE {}
    | FALSE {}
    | STRG { /* string literals are formatted */ };

expression_factor
    : constant_expression
    | func_reference {}
    | compound_name {}
    ;

expression
    : expression_factor {}
    | expression '+' expression {}
    | expression '-' expression {}
    | expression '*' expression {}
    | expression '/' expression {}
    | expression '%' expression {}
    | expression EQU expression {}
    | expression NEQU expression {}
    | expression LORE expression {}
    | expression GORE expression {}
    | expression OR expression {}
    | expression AND expression {}
    | expression '<' expression {}
    | expression '>' expression {}
    | '-' expression %prec NEGATE {}
    | '!' expression %prec NEGATE {}
    | '(' expression ')' {}
    ;

expression_list
    : /* empty */
    | expression {}
    | expression_list ',' expression {}
    ;

%%

void yyerror(const char* s) {

    fprintf(stderr, "%s:%d: %s\n", get_file_name(), get_line_no(), s);
}
