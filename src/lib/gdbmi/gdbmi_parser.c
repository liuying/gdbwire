#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gdbmi_grammar.h"
#include "gdbmi_parser.h"

int gdbmi_parse(void);

/* flex */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE gdbmi__scan_string(const char *yy_str);
extern void gdbmi__delete_buffer(YY_BUFFER_STATE state);
extern FILE *gdbmi_in;
extern int gdbmi_lex(void);
extern char *gdbmi_text;
extern int gdbmi_lineno;

struct gdbmi_parser {
    char *last_error;
    gdbmi_pstate *mips;
    struct gdbmi_pdata *pdata;
};

struct gdbmi_parser *gdbmi_parser_create(void)
{
    struct gdbmi_parser *parser;

    parser = (struct gdbmi_parser *) malloc(sizeof (struct gdbmi_parser));
    parser->last_error = NULL;

    if (!parser) {
        fprintf(stderr, "%s:%d", __FILE__, __LINE__);
        return NULL;
    }

    /* Create a new parser instance */
    parser->mips = gdbmi_pstate_new();
    if (!parser) {
        fprintf(stderr, "%s:%d", __FILE__, __LINE__);
        return NULL;
    }

    /* Create the data the parser parses into */
    parser->pdata = create_gdbmi_pdata();
    if (!parser->pdata) {
        fprintf(stderr, "%s:%d", __FILE__, __LINE__);
        return NULL;
    }

    return parser;
}

int gdbmi_parser_destroy(struct gdbmi_parser *parser)
{

    if (!parser)
        return -1;

    if (parser->last_error) {
        free(parser->last_error);
        parser->last_error = NULL;
    }

    if (parser->mips) {
        /* Free the parser instance */
        gdbmi_pstate_delete(parser->mips);
        parser->mips = NULL;
    }

    if (parser->pdata) {
        destroy_gdbmi_pdata(parser->pdata);
        parser->pdata = NULL;
    }

    free(parser);
    parser = NULL;
    return 0;
}

int
gdbmi_parser_parse_string(struct gdbmi_parser *parser,
        const char *mi_command, struct gdbmi_output **pt, int *parse_failed)
{
    YY_BUFFER_STATE state;
    int pattern;
    int mi_status;

    if (!parser)
        return -1;

    if (!mi_command)
        return -1;

    if (!parse_failed)
        return -1;

    /* Initialize output parameters */
    *pt = 0;
    *parse_failed = 0;

    parser->pdata->parsed_one = 0;

    /* Create a new input buffer for flex. */
    state = gdbmi__scan_string(strdup(mi_command));

    /* Create a new input buffer for flex and
     * iterate over all the tokens. */
    do {
        pattern = gdbmi_lex();
        if (pattern == 0)
            break;
        mi_status = gdbmi_push_parse(parser->mips, pattern, NULL,
                parser->pdata);
    } while (mi_status == YYPUSH_MORE);

    /* Parser is done, this should never happen */
    if (mi_status != YYPUSH_MORE && mi_status != 0) {
        *parse_failed = 1;
    } else if (parser->pdata->parsed_one) {
        *pt = parser->pdata->tree;
        parser->pdata->tree = NULL;
    }

    /* Free the scanners buffer */
    gdbmi__delete_buffer(state);

    return 0;
}

int
gdbmi_parser_parse_file(struct gdbmi_parser *parser,
        const char *mi_command_file, struct gdbmi_output **pt,
        int *parse_failed)
{
    int pattern;
    int mi_status;

    if (!parser)
        return -1;

    if (!mi_command_file)
        return -1;

    if (!parse_failed)
        return -1;

    *pt = 0;
    *parse_failed = 0;

    /* Initialize data */
    gdbmi_in = fopen(mi_command_file, "r");

    if (!gdbmi_in) {
        fprintf(stderr, "%s:%d", __FILE__, __LINE__);
        return -1;
    }

    /* Create a new input buffer for flex and
     * iterate over all the tokens. */
    do {
        pattern = gdbmi_lex();
        if (pattern == 0)
            break;
        mi_status = gdbmi_push_parse(parser->mips, pattern, NULL,
                parser->pdata);
    } while (mi_status == YYPUSH_MORE);

    /* Parser is done, this should never happen */
    if (mi_status != YYPUSH_MORE && mi_status != 0) {
        *parse_failed = 1;
    } else {
        *pt = parser->pdata->tree;
        parser->pdata->tree = NULL;
    }

    fclose(gdbmi_in);

    return 0;
}