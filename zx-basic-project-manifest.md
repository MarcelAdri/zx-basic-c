Project Manifest: ZX_Basic Interpreter (C / WASM)

1. Naamgevingsconventies & Typen

Om fantoom-types en compilerwarnings te voorkomen, hanteren we strikt deze definities:

    Parser-context: ParserContext (bevat de buffer, size, en de actuele cursor).

    Numerieke Scalar: NumericVariable (bevat char name[] en een rauwe double value zonder ZxValue overhead).

    String-behuizing: ZxStringSlot (bevat bool exists, bool is_dimmed, uint8_t *data, size_t len, uint8_t num_dimensions, uint16_t *dimension_sizes).

    Numerieke Array: ZxNumericArray (bevat de multidimensionale administratie voor getallen).

    Universele datadrager: ZxValue (de algemene variant waarin de parser types zoals ZX_TYPE_NUMBER en ZX_TYPE_STRING communiceert).

2. Architectonische Spelregels

    Foutafhandeling: Fouten bubbelen consistent omhoog via ZxError. We vertalen technische fouten direct naar authentieke Sinclair-foutcodes (bijv. ERR_C_NONSENSE_IN_BASIC of ERR_B_INTEGER_OUT_OF_RANGE).

    Geheugenbeheer bij parsing: Geen onnodige heap-allocaties (malloc) tijdens het parsen van indices. We gebruiken vaste stack-arrays (maximaal 10 dimensies, bijv. uint16_t indices[10]) om memory leaks te voorkomen.

    De Poortwachter-architectuur: expressions.c is de soevereine koning van de syntactische grammatica. Functies van buitenaf (zoals LET in execute.c) gebruiken de publieke API zx_parse_variable_reference om variabelen en hun indices links en rechts van de = uniform te ontleden.

3. Sinclair BASIC Idiosyncrasieën (De Kronkels)

    Procrustean Strings: Strings en string-arrays worden bij initialisatie of dimensionering direct volledig gevuld met spaties (ASCII 32). Kortere toewijzingen worden links uitgelijnd en behouden hun spatie-padding; langere toewijzingen worden afgekat (truncation).

    Naamgeving-beperkingen: * Numerieke variabelen mogen lange namen hebben (snelheid).

        Numerieke arrays, string-variabelen én string-arrays mogen strict slechts 1 letter lang zijn (a, a$, a$()).

    Array Toewijzing: LET a$ = "XYZ" op een string-array (is_dimmed == true) overschrijft geruisloos de allereerste rij en vult deze aan met spaties.

    String Slicing: 
    * A$(4) of A$(4 TO ) zonder expliciete eindgrens gebruikt gewenste lengtevlaggen (-1 resp. -2) om aan te geven dat er tot het absolute einde van de string/rij gesliced moet worden.
    * A$(4 TO 2) (waarbij eind < begin) resulteert in een expliciet berekende gewenste lengte van 0, wat correct leidt tot een legitieme, lege string.
4. Implementatie monitor actionable tokens

| Token | Beschrijving | Constant               | Implementatie                                | Opmerkingen                           |
|-------|--------------|------------------------|----------------------------------------------|---------------------------------------|
| 165   | RND          | ZX_FUN_RND             | zx_function_rnd                              |                                       |
| 166   | INKEY$       | ZX_FUN_INKEY_S         | zx_function_inkey_s                          |                                       |
| 167   | PI           | ZX_FUN_PI              | zx_function_pi                               |                                       |
| 168   | FN           | ZX_FUN_FN              | parse_function_definition                    |                                       |
| 169   | POINT        | ZX_FUN_POINT           |                                              | TODO                                  |
| 170   | SCREEN$      | ZX_FUN_SCREEN_S        | zx_function_screen_s                         |                                       |
| 171   | ATTR         | ZX_FUN_ATTR            |                                              | TODO                                  |
| 172   | AT           | ZX_TOKEN_AT            | execute_cmd_print                            |                                       |
| 173   | TAB          | ZX_TOKEN_TAB           | execute_cmd_print                            |                                       |
| 174   | VAL$         | ZX_FUN_VAL_S           | zx_function_val_s                            |                                       |
| 175   | CODE         | ZX_FUN_CODE            | zx_function_code                             |                                       |
| 176   | VAL          | ZX_FUN_VAL             | zx_function_val                              |                                       |
| 177   | LEN          | ZX_FUN_LEN             | zx_function_len                              |                                       |
| 178   | SIN          | ZX_FUN_SIN             | zx_function_sin                              |                                       |
| 179   | COS          | ZX_FUN_COS             | zx_function_cos                              |                                       |
| 180   | TAN          | ZX_FUN_TAN             | zx_function_tan                              |                                       |
| 181   | ASN          | ZX_FUN_ASN             | zx_function_asn                              |                                       |
| 182   | ACS          | ZX_FUN_ACS             | zx_function_acs                              |                                       |
| 183   | ATN          | ZX_FUN_ATN             | zx_function_asn                              |                                       |
| 184   | LN           | ZX_FUN_LN              | zx_function_ln                               |                                       |
| 185   | EXP          | ZX_FUN_EXP             | zx_function_exp                              |                                       |
| 186   | INT          | ZX_FUN_INT             | zx_function_int                              |                                       |
| 187   | SQR          | ZX_FUN_SQR             | zx_function_sqr                              |                                       |
| 188   | SGN          | ZX_FUN_SGN             | zx_function_sgn                              |                                       |
| 189   | ABS          | ZX_FUN_ABS             | zx_function_abs                              |                                       |
| 190   | PEEK         | ZX_FUN_PEEK            |                                              | TODO                                  |
| 191   | IN           | ZX_FUN_IN              |                                              | TODO                                  |
| 192   | USR          | ZX_FUN_USR             | zx_function_usr                              |                                       |
| 193   | STR$         | ZX_FUN_STR_S           | zx_function_str_s                            |                                       |
| 194   | CHR$         | ZX_FUN_CHR_S           | zx_function_chr_string                       |                                       |
| 195   | NOT          | ZX_OP_NOT              | parse_logical_not                            |                                       |
| 196   | BIN          | ZX_TOKEN_BIN           | parse_factor                                 |                                       |
| 197   | OR           | ZX_OP_OR               | parse_expression                             |                                       |
| 198   | AND          | ZX_OP_AND              | parse_logical_and                            |                                       |
| 202   | LINE         |                        |                                              | TODO                                  |
| 203   | THEN         | ZX_TOKEN_THEN          | execute_cmd_if                               |                                       |
| 204   | TO           | ZX_TOKEN_TO            | parse_array_index_string<br/>execute_cmd_for |                                       |
| 205   | STEP         |                        |                                              | TODO                                  |
| 206   | DEF FN       | ZX_STATEMENT_DEF_FN    | parse_function_definition                    |                                       |
| 207   | CAT          |                        |                                              | TODO                                  |
| 208   | FORMAT       |                        |                                              | TODO                                  |
| 209   | MOVE         |                        |                                              | TODO                                  |
| 210   | ERASE        |                        |                                              | TODO                                  |
| 211   | OPEN#        |                        |                                              | TODO                                  |
| 212   | CLOSE#       |                        |                                              | TODO                                  |
| 213   | MERGE        |                        |                                              | TODO                                  |
| 214   | VERIFY       |                        |                                              | TODO                                  |
| 215   | BEEP         |                        |                                              | TODO                                  |
| 216   | CIRCLE       |                        |                                              | TODO                                  |
| 217   | INK          |                        |                                              | TODO                                  |
| 218   | PAPER        |                        |                                              | TODO                                  |
| 219   | FLASH        |                        |                                              | TODO                                  |
| 220   | BRIGHT       |                        |                                              | TODO                                  |
| 221   | INVERSE      |                        |                                              | TODO                                  |
| 222   | OVER         |                        |                                              | TODO                                  |
| 223   | OUT          |                        |                                              | TODO                                  |
| 224   | LPRINT       |                        |                                              | TODO                                  |
| 225   | LLIST        |                        |                                              | TODO                                  |
| 226   | STOP         | ZX_STATEMENT_STOP      |                                              | TODO?                                 |
| 227   | READ         |                        |                                              | TODO                                  |
| 228   | DATA         |                        |                                              | TODO                                  |
| 229   | RESTORE      |                        |                                              | TODO                                  |
| 230   | NEW          | ZX_STATEMENT_NEW       | execute_cmd_new                              | TODO: de OK melding geeft statement 2 |
| 231   | BORDER       |                        |                                              | TODO                                  |
| 232   | CONTINUE     |                        |                                              | TODO                                  |
| 233   | DIM          | ZX_STATEMENT_DIM       | execute_cmd_dim                              |                                       |
| 234   | REM          | ZX_STATEMENT_REM       | extract_statement                            |                                       |
| 235   | FOR          | ZX_STATEMENT_FOR       | execute_cmd_for                              |                                       |
| 236   | GO TO        | ZX_STATEMENT_GO_TO     | execute_cmd_go_to                            |                                       |
| 237   | GO SUB       | ZX_STATEMENT_GO_SUB    |                                              | TODO                                  |
| 238   | INPUT        |                        |                                              | TODO                                  |
| 239   | LOAD         | ZX_STATEMENT_LOAD      | execute_cmd_load                             |                                       |
| 240   | LIST         | ZX_STATEMENT_LIST      | execute_cmd_list                             |                                       |
| 241   | LET          | ZX_STATEMENT_LET       | execute_cmd_let                              |                                       |
| 242   | PAUSE        | ZX_STATEMENT_PAUSE     | execute_cmd_pause                            |                                       |
| 243   | NEXT         | ZX_STATEMENT_NEXT      | execute_cmd_next                             |                                       |
| 244   | POKE         |                        |                                              | TODO                                  |
| 245   | PRINT        | ZX_STATEMENT_PRINT     | execute_cmd_print                            |                                       |
| 246   | PLOT         |                        |                                              | TODO                                  |
| 247   | RUN          | ZX_STATEMENT_RUN       | execute_cmd_run                              |                                       |
| 248   | SAVE         | ZX_STATEMENT_SAVE      | execute_cmd_save                             |                                       |
| 249   | RANDOMIZE    | ZX_STATEMENT_RANDOMIZE | execute_cmd_randomize                        |                                       |
| 250   | IF           | ZX_STATEMENT_IF        | execute_cmd_if                               |                                       |
| 251   | CLS          | ZX_STATEMENT_CLS       | execute_cmd_cls                              |                                       |
| 252   | DRAW         |                        |                                              | TODO                                  |
| 253   | CLEAR        |                        |                                              | TODO                                  |
| 254   | RETURN       | ZX_STATEMENT_RETURN    |                                              | TODO                                  |
| 255   | COPY         |                        |                                              | TODO                                  |
