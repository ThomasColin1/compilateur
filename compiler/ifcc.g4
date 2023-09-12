grammar ifcc;

axiom : prog ;

prog : (function)+ ;

function : 'int' VARNAME '(' params? ')' '{' (statement)* '}' ;

params : ('int' VARNAME (',' 'int' VARNAME)*);

statement :
            definition ';'        
          | declaration ';'
          | retour ';'
          | functionCall';'
          | block
          | blockif
          | blockwhile
          ;

functionCall : VARNAME '(' (expr (',' expr)*)? ')' ;

block : '{' (statement)* '}' ;

blockif : 'if' '(' expr ')' block (blockelse)? ;

blockelse : 'else' block;

blockwhile: 'while' '(' expr ')' block;

declaration : 'int' VARNAME (',' VARNAME)* ;

expr  : ('!' | '-') expr        # unaireNegNot
      | expr ('*' | '/' ) expr  # multdiv
      | expr ('+' | '-' ) expr  # addsub
      | CONST                   # constExpr
      | VARNAME                 # varExpr
      | functionCall            # exprFunctionCall
      | '(' expr ')'            # par
      | expr ('<' | '>') expr   # boolInfSup
      | expr ('==' | '!=') expr # boolDiffEgal
      ;

definition : partg '=' expr ;

partg : 
        declaration             # declpartg
      | VARNAME                 # varpartg
      ;

retour : 'return' expr ;

CONST : [0-9]+ ;
VARNAME : [a-zA-Z_][a-zA-Z0-9_]* ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);