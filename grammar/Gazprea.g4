grammar Gazprea;

// PARSER RULES
file: block EOF;

// Blocks
block: stat*;

// Expressions
expr: TUPL_ACCESS                                             #dot
    | '(' expr ')'                                            #parens
    | expr '[' expr (',' expr)? ']'                           #index
    | expr RANGE expr                                         #range
    | INT_RANGE RANGE INT                                     #rangeRealStartRealEnd // edge case for range e.g. "1....1"
    | INT_RANGE expr                                          #rangeIntStart // edge case for range e.g. "1...2"
    | <assoc=right> op=('+' | '-' | KW_NOT) expr              #unary
    | <assoc=right> expr '^' expr                             #exp
    | expr op=('*' | '/' | '%' | '**') expr                   #multDivRem
    | expr op=('+' | '-') expr                                #addSub
    | expr KW_BY expr                                         #by
    | expr op=('<' | '>' | '<=' | '>=') expr                  #ltGt
    | expr op=('==' | '!=') expr                              #eqNe
    | expr KW_AND expr                                        #and
    | expr op=(KW_OR | KW_XOR) expr                           #orXor
    | <assoc=right> expr '||' expr                            #concat
    | '[' (expr (',' expr)*)? ']'                             #vecMatLit
    | builtIn                                                 #builtInFunc
    | '[' ID KW_IN expr (',' ID KW_IN expr)? '|' expr ']'     #gen
    | '[' ID KW_IN expr '&' expr (',' expr)* ']'              #filt
    | '(' expr (',' expr)* ')'                                #tupl
    | KW_AS '<' declType '>' '(' expr ')'                     #typeCast
    | ID '(' (expr (',' expr)*)? ')'                          #funcProcCall
    | ID                                                      #id
    | INT                                                     #int
    | REAL                                                    #real
    | BOOL                                                    #bool
    | CHAR                                                    #char
    | STRING                                                  #string
    ;

// Declarations
decl: QUALIFIER? declType ID ('=' expr)? SC
    | QUALIFIER ID '=' expr SC
    ;

// representation of any type (primitive, vector, matrix, tuple)
declType: (TYPE | ID) ('[' declSize (',' declSize)? ']')?
        | KW_TUPLE '(' tupleElemType (',' tupleElemType)* ')'
        ;

declSize: (expr | STAR);

tupleElemType: declType ID?;

// Typedefs
typedef: KW_TYPEDEF declType ID SC;

// Assignments
assign: ID '=' expr SC   #idAssign // simple case, assign an expression result to an id
      | expr '=' expr SC #vecMatTupAssign // assign an expression result to an element in a vector/matrix/tuple
      | expr (',' expr)+ '=' tupleExpr SC #tuplUnpackAssign // unpack a tuple
      ;

tupleExpr: expr;

// Conditionals
cond: KW_IF '(' expr ')' stat condElse?;

condElse: KW_ELSE stat;

// Function Definitions
funcDef: KW_FUNCTION ID '(' (funcParam (',' funcParam)*)? ')' KW_RETURNS declType SC // func prototypes
       | KW_FUNCTION ID '(' (funcParam (',' funcParam)*)? ')' KW_RETURNS declType '=' expr SC // single line func
       | KW_FUNCTION ID '(' (funcParam (',' funcParam)*)? ')' KW_RETURNS declType blockStat; // multi line func

// Procedure Definitions
procDef: KW_PROCEDURE ID '(' (procParam (',' procParam)*)? ')' (KW_RETURNS declType)? SC // proc prototypes
       | KW_PROCEDURE ID '(' (procParam (',' procParam)*)? ')' (KW_RETURNS declType)? blockStat; // multi line proc

// Function/Procedure parameters
procParam: QUALIFIER? declType ID?;

funcParam: declType ID?;

// Loops
loop: KW_LOOP stat (KW_WHILE '(' expr ')' SC)? #infPostPredLoop // infinite or post predicated loop
    | KW_LOOP KW_WHILE '(' expr ')' stat  #prePredLoop // pre predicated loop
    | KW_LOOP ID KW_IN expr stat #iterLoop
    ;

// Streams
stream: expr '->' KW_STD_OUTPUT SC #outStream
      | expr '<-' KW_STD_INPUT SC  #inStream // expr can only be an l-value
      ;

// Built-In functions
builtIn: KW_LENGTH '(' expr ')'
       | KW_ROWS '(' expr ')'
       | KW_COLUMNS '(' expr ')'
       | KW_FORMAT '(' expr ')'
       | KW_STREAM_STATE '(' KW_STD_INPUT ')'
       | KW_REVERSE '(' expr ')'
       ;

// Statements
stat: blockStat
    | decl
    | typedef
    | funcDef
    | procDef
    | assign
    | stream
    | cond
    | rawProcCall
    | loop
    | return
    | break
    | continue
    ;

// Block statements
blockStat: '{' block '}';

// Return statements
return: KW_RETURN (expr)? SC;

// Break statements
break: KW_BREAK SC;

// Continue statements
continue: KW_CONTINUE SC;

// Procedure calls
rawProcCall: KW_CALL ID '(' (expr (',' expr)*)? ')' SC;


// LEXER RULES

// Types
TYPE: ( KW_BOOLEAN
      | KW_CHARACTER
      | KW_INTEGER
      | KW_REAL
      | KW_STRING
      );

// Strings
STRING: '"' ( ~["\\] | '\\' . )* '"'; // anything between double quotes including escape characters and whitespace

// Boolean values
BOOL: KW_TRUE
    | KW_FALSE
    ;

// Characters
CHAR: '\'' (~['\\] | '\\' .) '\'';

// Skip whitespace
WS : [ \t\r\n]+ -> skip ;

// Line comments
L_COMMENT: '//' .*? ('\n' | EOF) -> skip;

// Multi-line comments
ML_COMMENT: '/*' .*? '*/' -> skip;

// Type qualifiers
QUALIFIER: (KW_CONST | KW_VAR);

// Keywords
KW_AND: 'and';
KW_AS: 'as';
KW_BOOLEAN: 'boolean';
KW_BREAK: 'break';
KW_BY: 'by';
KW_CALL: 'call';
KW_CHARACTER: 'character';
KW_COLUMNS: 'columns';
KW_CONST: 'const';
KW_CONTINUE: 'continue';
KW_ELSE: 'else';
KW_FALSE: 'false';
KW_FORMAT: 'format';
KW_FUNCTION: 'function';
KW_IF: 'if';
KW_IN: 'in';
KW_INTEGER: 'integer';
KW_LENGTH: 'length';
KW_LOOP: 'loop';
KW_NOT: 'not';
KW_OR: 'or';
KW_PROCEDURE: 'procedure';
KW_REAL: 'real';
KW_RETURN: 'return';
KW_RETURNS: 'returns';
KW_REVERSE: 'reverse';
KW_ROWS: 'rows';
KW_STD_INPUT: 'std_input';
KW_STD_OUTPUT: 'std_output';
KW_STREAM_STATE: 'stream_state';
KW_STRING: 'string';
KW_TRUE: 'true';
KW_TUPLE: 'tuple';
KW_TYPEDEF: 'typedef';
KW_VAR: 'var';
KW_WHILE: 'while';
KW_XOR: 'xor';

// Identifiers
ID: ALPHA_US ALPHA_DIGIT_US*;

// Range operator
RANGE: '..';

// Integer + range operator (special case to avoid conflict with integer literals and range ops)
INT_RANGE: '-'? INT RANGE;

// Integers
INT: DIGIT+;

// Tuple access (you're not allowed to access a tuple literal)
TUPL_ACCESS: ID '.' (INT | ID);

// Reals
REAL: DIGIT+ '.' DIGIT* (('e' | 'E') ('+' | '-')? DIGIT+)?
    | '.' DIGIT+ (('e' | 'E') ('+' | '-')? DIGIT+)?
    | DIGIT+ ('e' | 'E') ('+' | '-')? DIGIT+

    ;

// Special characters
STAR: '*';
SC: ';';
US: '_';

// fragments
fragment DIGIT: [0-9];
fragment ALPHA: [a-zA-Z];
fragment ALPHA_DIGIT: (ALPHA | DIGIT);
fragment ALPHA_US: (ALPHA | US);
fragment ALPHA_DIGIT_US: (ALPHA_DIGIT | US);