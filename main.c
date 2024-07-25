/**
 * @file main.c
 * @author Warren Mann (warren@nonvol.io)
 * @brief Main program module.
 * @version 0.1.0
 * @date 2024-06-20
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
#define stack_start 64

static FILE *fp = NULL;
static void **stack = NULL;
static long int stack_index = 0;
static long int stack_size = 0;
static long int line_number = 0;

static void free_object(json_object *obj);
static char *get_key(char *line);
static char *get_line(void);
static void insert_key_value(json_object *obj, char *key, json_object *value);
static bool is_blank(char *line);
static void output_object(json_object *obj);
static json_object *parse_array(void);
static json_object *parse_object(void);
static void *pop(void);
static int push(void *p);
static bool valid_number(const char *s);

int main(int ac, char **av) {
    debug_enter();
    char *line;
    bool in_comment = false;
    fp = stdin;
    stack_size = stack_start;
    stack = malloc(stack_size * sizeof(void *));
    if (stack == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return 1;
    }
    while ((line = get_line()) != NULL) {
        debug("read line %li: \"%s\"\n", line_number, line);
        char *lp = line;
        while (*lp == ' ') {
            lp++;
        }
        if (strncmp(lp, key_prefix, key_type_position - 1) == 0) {
            debug("lp[key_type_position] = %c\n", lp[key_type_position]);
            if (lp[key_type_position] == key_start_obj) {
                debug("got object\n");
                in_comment = false;
                json_object *object = parse_object();
                output_object(object);
                free_object(object);
            } else if (lp[key_type_position] == key_start_array) {
                debug("got array\n");
                in_comment = false;
                json_object *array = parse_array();
                output_object(array);
                free_object(array);
            } else if (lp[key_type_position] == key_comment) {
                debug("got comment\n");
                in_comment = true;
            } else if (!in_comment) {
                fprintf(stderr, "Invalid key type: \"%s\" on line %li\n", lp, line_number);
                debug_return 1;
            }
        }
    }
    if (stack != NULL) {
        free(stack);
    }
    debug_return 0;
}

static void free_object(json_object *obj) {
/*    void *vp;
    json_object_put(obj);
    while ((vp = pop()) != NULL) {
        free(vp);
    } */
}

static char *get_key(char *line) {
    debug_enter();
    debug("parsing key from \"%s\"\n", line);
    char *k;
    char *s = line ;
    while (*s == ' ') {
        s++;
    }
    if (*s == '\0') {
        debug_return NULL;
    }
    s += key_type_position;
    char *e = s + strlen(s) - 1;
    while (e >= s && isspace(*e)) {
        *e-- = '\0';
    }
    if (*s == '\0') {
        debug_return NULL;
    }
    debug("key = \"%s\"\n", s);
    k = strdup(s);
    if (k == NULL) {
        fprintf(stderr, "Memory allocation error on line %li\n", line_number);
        debug_return NULL;
    }
    debug_return k;
}

static char *get_line(void) {
    static char line[max_line_len];
    char *r = fgets(line, max_line_len, fp);
    if (r == NULL) {
        return NULL;
    }
    char *lp = line + strlen(line) - 1;;
    while (lp >= line && (*lp == '\n' || *lp == '\r')) {
        *lp-- = '\0';;
    } 
    line_number++;
    return r;
}
            
static void insert_key_value(json_object *obj, char *key, json_object *value) {
    debug_enter();
    if (key != NULL) {
        key++;
        debug("adding key \"%s\" value %s\n", key, json_object_to_json_string(value));
        json_object_object_add(obj, key, value);
    } else {
        fprintf(stderr, "Key is NULL on line %li\n", line_number);
    }
    debug_return;
}

static bool is_blank(char *line) {
    char *s = line;
    while (*s != '\0' && isspace(*s)) {
        s++;
    }
    if (*s == '\0') {
        return true;
    }
    return false;
}

static void output_object(json_object *obj) {
    if (obj != NULL) {
        printf("%s\n", json_object_to_json_string(obj));
    }
}

static json_object *parse_array(void) {
    debug_enter();
    json_object *array = json_object_new_array();
    json_object *value = NULL;
    char *line = NULL;
    char *data = NULL;
    const char *key = NULL;
    int indent = 0;
    while ((line = get_line()) != NULL) {
        char *lp = line;
        while (*lp == ' ') {
            lp++;
        }
        debug("line %li = \"%s\"\n", line_number, lp);
        if (strncmp(lp, key_prefix, key_type_position - 1) == 0 && lp[key_type_position] != key_escape) {
            lp = line;
            while (*lp == ' ') {
                lp++;
                indent++;
            }
            debug("got key \"%s\"\n", lp);
            if (data != NULL) {
                char *e = data + strlen(data) - 1;
                while (e >= data && isspace(*e)) {
                    *e-- = '\0';
                }
                if (key[0] != key_comment) {
                    debug("data = \"%s\"\n", data);
                    switch (key[0]) {
                        case key_boolean:
                            if (strcasecmp(data, "true") != 0 && strcasecmp(data, "false") != 0) {
                                fprintf(stderr, "Invalid boolean value \"%s\" on line %li\n", data, line_number);
                                debug_return NULL;
                            }
                            value = json_object_new_boolean(strcasecmp(data, "true") == 0);
                            if (push(data) != 0) {
                                debug_return NULL;
                            }
                            break;
                        case key_null:
                            if (strcasecmp(data, "null") != 0) {
                                fprintf(stderr, "Invalid null value \"%s\" on line %li\n", data, line_number);
                                debug_return NULL;
                            }
                            free(data);
                            data = NULL;
                            value = NULL;
                            break;
                        case key_number:
                            if (!valid_number(data)) {
                                fprintf(stderr, "Invalid number value \"%s\" on line %li\n", data, line_number);
                                debug_return NULL;
                            }
                            if (push(data) != 0) {
                                debug_return NULL;
                            }
                            value = json_object_new_double(atof(data));
                            break;
                        default:
                            debug("adding data as string \"%s\"\n", data);
                            value = json_object_new_string(data);
                            if (push(data) != 0) {
                                debug_return NULL;
                            }
                            break;
                    }
                    data = NULL;
                    json_object_array_add(array, value);
                } else {
                    free(data);
                    data = NULL;
                }
            } else if (value != NULL) {
                json_object_array_add(array, value);
                value = NULL;
            }
            if (lp[key_type_position] == key_start_obj) {
                value = parse_object();
            } else if (lp[key_type_position] == key_start_array) {
                value = parse_array();
            } else if (lp[key_type_position] == key_end_array) {
                debug_return array;
            }
            key = get_key(line);
        } else {
            if (!is_blank(line)) {
                lp = line;
                int i = 0;
                while (*lp == ' ' && i < indent) {
                    lp++;
                    i++;
                }
                if (strncmp(lp, key_prefix, key_type_position) == 0 && lp[key_type_position] == key_escape) {
                    memmove(lp + key_type_position, lp + key_type_position + 1, strlen(lp + key_type_position + 1) + 1);
                }
                long int l = strlen(lp);
                if (data == NULL) {
                    data = malloc(l + 1);
                    strncpy(data, lp, l + 1);
                } else {
                    l += strlen(data);
                    char *n = realloc(data, l + 1);
                    if (n != NULL) {
                        data = n;
                    } else {
                        fprintf(stderr, "Memory allocation error on line %li\n", line_number);
                        debug_return NULL;
                    }
                    strcat(data, lp);
                }
            }
        }
    }
    debug_return array;
}

static json_object *parse_object(void) {
    debug_enter();
    json_object *object = json_object_new_object();
    json_object *value = NULL;
    char *line = NULL;
    char *key = NULL;
    char *data = NULL;
    int indent = 0;
    fp = stdin;
    while ((line = get_line()) != NULL) {
        char *lp = line;
        while (*lp == ' ') {
            lp++;
        }
        debug("line %li = \"%s\"\n", line_number, lp);
        if (strncmp(lp, key_prefix, key_type_position - 1) == 0 && lp[key_type_position] != key_escape) {
            debug("got key \"%s\"\n", lp);
            lp = line;
            indent = 0;
            while (*lp == ' ') {
                lp++;
                indent++;
            }
            if (key != NULL) {
                if (key[1] == '\0') {
                    fprintf(stderr, "Anonymous value is not allowed on line %li\n", line_number);
                    debug_return NULL;
                }
                debug("Inserting key \"%s\" with datatype %c\n", key + 1, key[0]);
                if (key[0] != key_comment) {
                    if (data != NULL) {
                        char *e = data + strlen(data) - 1;
                        while (e >= data && isspace(*e)) {
                            *e-- = '\0';
                        }
                        debug("data = \"%s\"\n", data);
                        switch (key[0]) {
                            case key_boolean:
                                if (strcasecmp(data, "true") != 0 && strcasecmp(data, "false") != 0) {
                                    fprintf(stderr, "Invalid boolean value \"%s\" on line %li\n", data, line_number);
                                    debug_return NULL;
                                }
                                value = json_object_new_boolean(strcasecmp(data, "true") == 0);
                                break;
                            case key_null:
                                if (strcasecmp(data, "null") != 0) {
                                    fprintf(stderr, "Invalid null value \"%s\" on line %li\n", data, line_number);
                                    debug_return NULL;
                                }
                                free(data);
                                data = NULL;
                                value = NULL;
                                break;
                            case key_number:
                                if (!valid_number(data)) {
                                    fprintf(stderr, "Invalid number value \"%s\" on line %li\n", data, line_number);
                                    debug_return NULL;
                                }
                                if (push(data) != 0) {
                                    debug_return NULL;
                                }
                                value = json_object_new_double(atof(data));
                                break;
                            default:
                                debug("adding data as string \"%s\"\n", data);
                                value = json_object_new_string(data);
                                break;
                        }
                        if (data != NULL) {
                            if (push(data) != 0) {
                                return NULL;
                            }
                            data = NULL;
                        }
                    }
                    insert_key_value(object, key, value);
                    if (push(key) != 0) {
                        debug_return NULL;
                    }
                } else {
                    if (data != NULL) {
                        free(data);
                        data = NULL;
                    }
                }
            }
            key = get_key(line);
            if (lp[key_type_position] == key_start_obj) {
                value = parse_object();
            } else if (lp[key_type_position] == key_start_array) {
                value = parse_array();
            } else if (lp[key_type_position] == key_end_obj) {
                debug("returning object\n");
                debug_return object;
            }
        } else {
            if (!is_blank(line)) {
                lp = line;
                int i = 0;
                while (*lp == ' ' && i < indent) {
                    lp++;
                    i++;
                }
                if (strncmp(lp, key_prefix, key_type_position) == 0 && lp[key_type_position] == key_escape) {
                    memmove(lp + key_type_position, lp + key_type_position + 1, strlen(lp + key_type_position + 1) + 1);
                }
                long int l = strlen(lp);
                if (data == NULL) {
                    data = malloc(l + 1);
                    strncpy(data, lp, l + 1);
                } else {
                    l += strlen(data);
                    char *n = realloc(data, l + 1);
                    if (n != NULL) {
                        data = n;
                    } else {
                        fprintf(stderr, "Memory allocation error on line %li\n", line_number);
                        debug_return NULL;
                    }
                    strcat(data, lp);
                }
            }
        }
    }
    fprintf(stderr, "Unexpected EOF on line %li\n", line_number);
    debug_return NULL;
}

static void *pop(void) {
    if (stack_index == 0) {
        return NULL;
    }
    return stack[--stack_index];
}

static int push(void *p) {
    stack[stack_index++] = p;
    if (stack_index >= stack_size) {
        stack_size *= 2;
        void **n = realloc(stack, stack_size * sizeof(void *));
        if (n == NULL) {
            fprintf(stderr, "Memory allocation error on line %li\n", line_number);
            return 1;
        }
        stack = n;
    }
    return 0;
}

static bool valid_number(const char *s) {
    int i = 0;
    int l = strlen(s);
    int j = l - 1;
    while (i < l && s[i] == ' ') {
        i++;
    }
    while (j >= 0 && s[j] == ' ') {
        j--;
    }
     if (i > j)
        return false;
    if (i == j && !(s[i] >= '0' && s[i] <= '9')) {
        return false;
    }
    if (s[i] != '.' && s[i] != '+' && s[i] != '-' && !(s[i] >= '0' && s[i] <= '9')) {
        return false;
    }
    bool flagDotOrE = false;
    for (; i <= j; i++) {
        if (s[i] != 'e' && s[i] != '.' && s[i] != '+' && s[i] != '-' && !(s[i] >= '0' && s[i] <= '9')) {
            return false;
        }
        if (s[i] == '.') {
            if (flagDotOrE == true) {
                return false;
            }
            if (i + 1 > l) {
                return false;
            }
            if (!(s[i + 1] >= '0' && s[i + 1] <= '9')) {
                return false;
            }
        }
        else if (s[i] == 'e') {
            flagDotOrE = true;
            if (!(s[i - 1] >= '0' && s[i - 1] <= '9')) {
                return false;
            }
            if (i + 1 > l) {
                return false;
            }
            if (s[i + 1] != '+' && s[i + 1] != '-' && (s[i + 1] >= '0' && s[i] <= '9')) {
                return false;
            }
        }
    }
    return true;
}

