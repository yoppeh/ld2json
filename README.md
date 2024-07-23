# LD2JSON

`ld2json` is a utility to convert a **Line-oriented Dataset** (LD) formatted
document to JSON/JSONL. Input is taken from `stdin` and is written to `stdout`.
Error and debug messages are written to `stderr`.

Example: `cat test.ld | ./ld2json`

The LD format is designed to make it easier to hand-create datasets. The format
can easily be translated to plain JSON or JSONL, while being relatively easy to
create. No need to worry about defining large JSON objects by hand. No need to
worry about column separators and escaping CSV files.

LD is line-oriented. Keys are defined on a line. The following lines contain the
data that should be assigned to that key. The data section ends at the next key.

Blank lines are not output. New lines in the input are removed in the
output. To include a newline in the output, use the two characters `\n`.

## Details

LD generates JSON objects with keys that have values. Here is an example:

```
~~:{
~~:$string_key
Value of the string key.
~~:#number_key
1234
~~:!null_value_key
null
~~:?boolean_key
true
~~:}
```

This produces the following JSONL output:

`{"string_key":"Value of the string key.","number_key":1234,"null_value_key":null,"boolean_key":true}`

Objects can also have child objects and arrays:

```
~~:{
~~:[test_array
~~:$
one
~~:$
two
~~:$
three
~~:]
~~:{child_object
~~:$field1
Field 1
~~:$field2
Field 2
~~:}
~~:}
```

This produces the following JSONL output:

`{"test_array":["one","two","three"],"child_object":{"field1":"Field 1",
"field2":"Field 2"}}`

You can also put comments in the LD file. These are ignored and not output:

```
~~~:* Some comment text
You can have comment text here too, if you want.
```

To output JSON instead of JSONL, you start off with a single anonymous object or
array at the top level:

Object:
```
~~:{
~~:}
```

Array:
```
~~:[
~~:]
```

LD data can be indented without indenting the output. If a key field starts with
a number of spaces, that number of spaces is removed frome the beginning of each
line of data:
```
~~:{
~~:$some_key
Some key.
~~:{child_object
    ~~:$key
    Data for this key is indented by four spaces, for readability.
~~:}
~~:$another_key
    In this case, the key specification isn't indented, so the four spaces at\n
    the start of these lines is kept.
~~:}
```

The output would be:

`{"some_key":"Some key.","child_object":{"key":"Data for this key is indented by
four spaces, for readability."},"another_key":"    In this case, the key
specification isn't indented, so the four spaces at\n    the start of these
lines is kept."}`

Except for the aforementioned indentation feature, the complete removal of
blank lines and the stripping of ending line feeds (and carriage returns), no
formatting is done on the output data or key names. They are output as-is.
