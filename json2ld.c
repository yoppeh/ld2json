/**
 * @file json2ld.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Main program module.
 * @version 0.1.0
 * @date 2024-07-25
 * @copyright Copyright (c) 2024
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include <json-c/json_object.h>

#ifdef DEBUG
static int indent_level = 0;
static char *indent_string = "                                                                                ";
#define debug(...) do { fprintf(stderr, "%.*s%s  %d  ", indent_level, indent_string, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); } while(0)
#define debug_indent_inc() do { indent_level += 4; if (indent_level > 20) indent_level = 20; } while(0)
#define debug_indent_dec() do { indent_level -= 4; if (indent_level < 0) indent_level = 0; } while(0)
#define debug_enter() do { debug("%s()\n", __PRETTY_FUNCTION__); indent_level += 4; } while(0)
#define debug_return indent_level -= 4; return
#else 
#define debug(...)
#define debug_enter()
#define debug_return return
#endif

#define indent_step 4
#define max_line_len 65536
#define key_prefix "~~:"
#define key_start_obj '{'
#define key_end_obj '}'
#define key_start_array '['
#define key_end_array ']'
#define key_string '$'
#define key_number '#'
#define key_boolean '?'
#define key_null '!'
#define key_comment '*'
#define key_escape '\\'
#define key_type_position (sizeof(key_prefix) - 1)
#define wrap_len 80

static FILE *fp = NULL;
static char line[max_line_len];
static const char *empty_string = "";

static void output_ld(const char *key, json_object *obj);
static char *wrap(const char *s, int width, int indent);

int main(int ac, char **av) {
    debug_enter();
    json_tokener *tok = json_tokener_new();
    json_object *json_obj = NULL;
    enum json_tokener_error jerr;
    fp = stdin;
    if (tok == NULL) {
        fprintf(stderr, "Unable to create json_tokener\n");
        debug_return 1;
    }
    for (int i = 1; i < ac; i++) {
        if (strcmp(av[i], "-h") == 0) {
            fprintf(stderr, "Usage: %s [file]\n", av[0]);
            fprintf(stderr, "file ... Input file\n");
            debug_return 0;
        } else if (fp == stdin) {
            fp = fopen(av[i], "r");
            if (fp == NULL) {
                fprintf(stderr, "Unable to open file \"%s\"\n", av[i]);
                debug_return 1;
            }
        }
    }
    while (fgets(line, max_line_len, fp) != NULL) {
        json_obj = json_tokener_parse_ex(tok, line, strlen(line));
        jerr = json_tokener_get_error(tok);
        if (jerr == json_tokener_success) {
            if (json_obj != NULL) {
                output_ld(empty_string, json_obj);
                json_object_put(json_obj);
                json_obj = NULL;
            }
        } else if (jerr != json_tokener_continue) {
            fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
            debug_return(1);
        }
    }
    if (jerr == json_tokener_continue && json_obj != NULL) {
        if (json_obj != NULL) {
            output_ld(empty_string, json_obj);
            json_object_put(json_obj);
            json_obj = NULL;
        }
    }
    debug_return 0;
}

static void output_ld(const char *key, json_object *obj) {
    debug_enter();
    static int indent = 0;
    char *k;
    char *s;
    json_object *val;
    if (obj == NULL) {
        fprintf(stderr, "Error: NULL object\n");
        debug_return;
    }
    switch (json_object_get_type(obj)) {
        case json_type_array:
            printf("%*s~~:%c%s\n", indent, " ", key_start_array, key);
            indent += indent_step;
            for (int i = 0; i < json_object_array_length(obj); i++) {
                output_ld(empty_string, json_object_array_get_idx(obj, i));
            }
            indent -= indent_step;
            printf("%*s~~:%c\n", indent, " ", key_end_array);
            break;
        case json_type_boolean:
            printf("%*s~~:%c%s\n%*s%s\n", indent, " ", key_number, key, indent, " ", json_object_get_boolean(obj) ? "true" : "false");
            break;
        case json_type_double:
            printf("%*s~~:%c%s\n%*s%lf\n", indent, " ", key_number, key, indent, " ", json_object_get_double(obj));
            break;
        case json_type_int:
            printf("%*s~~:%c%s\n%*s%d\n", indent, " ", key_number, key, indent, " ", json_object_get_int(obj));
            break;
        case json_type_null:
            printf("%*s~~:%c%s\n%*snull\n", indent, " ", key_null, key, indent, " ");
            break;
        case json_type_object:
            printf("%*s~~:%c%s\n", indent, " ", key_start_obj, key);
            indent += indent_step;
            json_object_object_foreach(obj, k, val) {
                output_ld(k, val);
            }
            indent -= indent_step;
            printf("%*s~~:%c\n", indent, " ", key_end_obj);
            break;
        case json_type_string:
            s = wrap(json_object_get_string(obj), wrap_len, indent);
            if (s != NULL) {
                printf("%*s~~:%c%s\n%s\n", indent, " ", key_string, key, s);
                free(s);
                s = NULL;
            } else {
                printf("%*s~~:%c%s\n%s\n", indent, " ", key_string, key, empty_string);
            }
            break;
        default:
            break;
    }
    debug_return;
}

static char *wrap(const char *s, int width, int indent) {
    debug_enter();
    if (indent >= width) {
        fprintf(stderr, "Error: indent must be less than width\n");
        debug_return NULL;
    }
    int sl = strlen(s);
    int rl = sl;
    int bl = ((sl / (width / 4)) * (width + indent + 2)) + (sl % width) + indent + 1;
    char *r = (char *)malloc(bl);
    if (!r) {
        debug_return NULL;
    }
    memset(r, ' ', bl);
    debug("string len = %d", sl);
    debug("buffer size = %d", bl);
    char *p = r;
    int ll = 0;
    int l;
    while (1) {
        p += indent;
        ll = width - indent;
        if (rl <= ll) {
            strcpy(p, s);
            break;
        }
        const char *e = s + ll - 1;
        while (e > s && !isspace(*e)) {
            e--;
        }
        if (e == s) {
            e = s + ll;
        }
        l = e - s + 1;
        for (int j = 0; j < l; j++) {
            if (s[j] == '\n') {
                *p++ = '\\';
                *p++ = 'n';
                l--;
            } else {
                *p++ = s[j];
            }
        }
        rl -= l;
        *p++ = '\n';
        s = e + 1;
        if (*s == '\0') {
            break;
        }
    }
    debug("result length = %ld", strlen(r));
    debug_return r;
}
