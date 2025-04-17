/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "grammar.tab.h"

    int yyerror(char *e);
%}

%%

"option"                    { return OPTION; }
"setval"                    { return SETVAL; }
"yes"                       { return YES; }
"no"                        { return NO;  }
\"([^\"])*\"                { yylval.str = strdup(yytext); return STRING; }
0[xX][0-9a-fA-F]+           { yylval.str = strdup(yytext); return STRING; }
[a-zA-Z_][a-zA-Z0-9_]+      { yylval.str = strdup(yytext); return IDENTIFIER; }
[0-9]+                      { yylval.num = atoi(yytext); return NUMBER; }
[ \t]                       { /* Ignore */ }
[\/\/.*]                    { /* Ignore */ }
\n                          { /* IGnore */ }
"//".*                      { /* Ignore */ }
.                           { /* Ignore */ }

%%

int
yyerror(char *e)
{
    printf("\n** kconf: %s **", e);
    return 0;
}
