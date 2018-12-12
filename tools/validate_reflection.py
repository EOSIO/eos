#!/usr/bin/env python3

import argparse
from collections import OrderedDict
import re
import os
import sys
import traceback

###############################################################
# validate_reflection
#
#
#    Looks for files with FC_REFLECT macros.  Requires fields to match class definition (provided in same file),
#  unless the FC_REFLECT is proceeded by "// @ignore <field1>, <field2>, ..., <fieldN>" to indicate that field1,
#  field2, ... fieldN are not reflected and/or "// @swap <field1>, <field2>, ..., <fieldN>" to indicate that
#  field1, field2, ... fieldN are not in the same order as the class definition.
#
#  NOTE:  If swapping fields the script expects you to only indicate fields that are not in the expected order,
#  so once it runs into the swapped field, it will remove that field from the order and expect the remaining in
#  that order, so if the class has field1, field2, field3, and field4, and the reflect macro has the order
#  field1, field3, field2, then field4, it should indicate swapping field2.  This will remove field2 from the
#  expected order and the rest will now match.  Alternatively it should indicate swapping field3, since the remaining
#  fields will also match the order.  But both field2 and field3 should not be indicated.
#
#
# 
###############################################################

import atexit
import tempfile

@atexit.register
def close_debug_file():
    if debug_file != None:
        debug_file.close()


parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('-?', action='help', default=argparse.SUPPRESS,
                         help=argparse._('show this help message and exit'))
parser.add_argument('-d', '--debug', help="generate debug output into a temporary directory", action='store_true')
parser.add_argument('-r', '--recurse', help="recurse through an entire directory (if directory provided for \"file\"", action='store_true')
parser.add_argument('-x', '--extension', type=str, help="extensions array to allow for directory and recursive search.  Defaults to \".hpp\" and \".cpp\".", action='append')
parser.add_argument('-e', '--exit-on-error', help="Exit immediately when a validation error is discovered.  Default is to run validation on all files and directories provided.", action='store_true')
parser.add_argument('files', metavar='file', nargs='+', type=str, help="File containing nodes info in JSON format.")
args = parser.parse_args()

recurse = args.recurse
if args.debug:
    temp_dir = tempfile.mkdtemp()
    print("temporary files writen to %s" % (temp_dir))
    debug_file = open(os.path.join(temp_dir, "validate_reflect.debug"), "w")
else:
    debug_file = None
extensions = []
if args.extension is None or len(args.extension) == 0:
    extensions = [".hpp",".cpp"]
else:
    for extension in args.extension:
        assert len(extension) > 0, "empty --extension passed in"
        if extension[0] != ".":
            extension = "." + extension
        extensions.append(extension) 
print("extensions=%s" % (",".join(extensions)))
ignore_str = "@ignore"
swap_str = "@swap"
fc_reflect_str = "FC_REFLECT"
fc_reflect_possible_enum_ext = "(?:_ENUM)?"
fc_reflect_derived_ext = "(?:_DERIVED)"

def debug(debug_str):
    if debug_file is not None:
        debug_file.write(debug_str + "\n")

class EmptyScope:
    single_comment_pattern = re.compile(r'//.*\n+')
    single_comment_ignore_swap_pattern = re.compile(r'//\s*(?:%s|%s)\s' % (ignore_str, swap_str))
    multi_line_comment_pattern = re.compile(r'/\*(.*?)\*/', re.MULTILINE | re.DOTALL)
    ignore_swap_pattern = re.compile(r'^\s*(%s|%s)\s+(.*)$' % (ignore_str, swap_str), re.DOTALL)
    strip_extra_pattern = re.compile(r'\n\s*\*\s*')
    invalid_chars_pattern = re.compile(r'([^\w\s,])')
    multi_line_comment_ignore_swap_pattern = re.compile(r'(\w+)(?:\s*,\s*)?')
    namespace_str = "namespace"
    struct_str = "struct"
    class_str = "class"
    enum_str = "enum"
    any_scope_pattern = re.compile(r'\{', re.DOTALL)
    start_char = "{"
    end_char = "}"

    def __init__(self, name, start, content, parent_scope):
        pname = parent_scope.name if parent_scope is not None else ""
        debug("EmptyScope.__init__ %s %d - Parent %s" % (name, start, pname))
        self.name = name
        self.content = content
        self.start = start
        self.current = start + 1
        self.parent_scope = parent_scope
        self.end = len(content) - 1 if start == 0 else None
        self.children = {}
        self.children_ordered = []
        self.fields = []
        self.usings = {}
        self.inherit = None

    def read(self):
        debug("EmptyScope(%s).read - %s" % (self.__class__.__name__, self.name))
        end = len(self.content) - 1
        while self.current < end:
            next_scope = self.next_scope()
            if next_scope is None:
                break
            self.add(next_scope)

        if self.end is None:
            self.end = self.content.find(EmptyScope.end_char, self.current, len(self.content))
            pdesc = str(self.parent_scope) if self.parent_scope is not None else "<no parent scope>"
            assert self.end != -1, "Could not find \"%s\" in \"%s\" - parent scope - %s" % (EmptyScope.end_char, self.content[self.current:], pdesc)
        debug("EmptyScope(%s).read - %s - Done at %s" % (self.__class__.__name__, self.name, self.end))

    def add(self, child):
        debug("EmptyScope.add %s (%s) to %s (%s) - DROP" % (child.name, child.__class__.__name__, self.name, self.__class__.__name__))
        pass

    def find_scope_start(self, content, start, end, find_str):
        debug("EmptyScope.find_scope_start")
        loc = content.find(find_str, start, end)
        if loc == -1:
            return loc
        else:
            return loc + len(find_str) - len(EmptyScope.start_char)

    def find_possible_end(self):
        possible = self.content.find(EmptyScope.end_char, self.current)
        debug("EmptyScope.find_possible_end current=%s  possible end=%s" % (self.current, possible))
        return possible

    def next_scope(self, end = None):
        if end is None:
            end = self.find_possible_end()
        debug("EmptyScope.next_scope current=%s  end=%s" % (self.current, end))
        match = EmptyScope.any_scope_pattern.search(self.content[self.current:end])
        if match:
            start = self.find_scope_start(self.content, self.current, end, EmptyScope.start_char)
            new_scope = EmptyScope(None, start, self.content, self)
            new_scope.read()
            self.current = new_scope.end + 1
            return new_scope

        return None

    def find_class(self, scoped_name):
        scope_separator = "::"
        loc = scoped_name.find(scope_separator)
        if loc != -1:
            child_name = scoped_name[0:loc]
            loc += len(scope_separator)
            child_scoped_name = scoped_name[loc:]
            if child_name in self.children:
                debug("find_class traverse child_name: %s, child_scoped_name: %s" % (child_name, child_scoped_name))
                return self.children[child_name].find_class(child_scoped_name)
            elif self.inherit is not None and scoped_name in self.inherit.children:
                debug("find_class found scoped_name: %s in inherited: %s" % (scoped_name, self.inherit.name))
                return self.inherit.children[scoped_name].find_class(child_scoped_name)
        else:
            if scoped_name not in self.children:
                inherit_children = ",".join(self.inherit.children) if self.inherit is not None else "no inheritance"
                inherit_using = ",".join(self.inherit.usings) if self.inherit is not None else "no inheritance"
                inherit = self.inherit.name if self.inherit is not None else None
                debug("find_class %s not in children, using: %s, inherit: %s - children: %s, using: %s" % (scoped_name, ",".join(self.usings), inherit, inherit_children, inherit_using))

            if scoped_name in self.children:
                debug("find_class found scoped_name: %s" % (scoped_name))
                return self.children[scoped_name]
            elif scoped_name in self.usings:
                using = self.usings[scoped_name]
                debug("find_class found scoped_name: %s, using: %s" % (scoped_name, using))
                return self.find_class(using)
            elif self.inherit is not None and scoped_name in self.inherit.children:
                debug("find_class found scoped_name: %s in inherited: %s" % (scoped_name, self.inherit.name))
                return self.inherit.children[scoped_name]
            else:
                debug("find_class could not find scoped_name: %s, children: %s" % (scoped_name, ",".join(self.children)))
                return None

    def __str__(self):
        indent = ""
        next = self.parent_scope
        while next is not None:
            indent += "    "
            next = next.parent_scope
        desc = "%s%s scope type=\"%s\"\n%s  children={\n" % (indent, self.name, self.__class__.__name__, indent)
        for child in self.children_ordered:
            desc += str(self.children[child]) + "\n"
        desc += indent + "  }\n"
        desc += indent + "  fields={\n"
        for field in self.fields:
            desc += indent + "    " + field + "\n"
        desc += indent + "  }\n"
        desc += indent + "  usings={\n"
        for using in self.usings:
            desc += indent + "    " + using + ": " + self.usings[using] + "\n"
        desc += indent + "  }\n"
        return desc

def create_scope(type, name, inherit, start, content, parent_scope):
    debug("create_scope")
    if type == EmptyScope.namespace_str:
        return Namespace(name, inherit, start, content, parent_scope)
    elif type == EmptyScope.class_str or type == EmptyScope.struct_str:
        return ClassStruct(name, inherit, start, content, parent_scope, is_enum = False)
    elif type == EmptyScope.enum_str:
        return ClassStruct(name, inherit, start, content, parent_scope, is_enum = True)
    else:
        assert False, "Script does not account for type = \"%s\" found in \"%s\"" % (type, content[start:])

class ClassStruct(EmptyScope):
    field_pattern = re.compile(r'\n\s*?(?:mutable\s+)?(\w[\w:\d<>]*)\s+(\w+)\s*(?:=\s[^;]+;|;|=\s*{)', re.MULTILINE | re.DOTALL)
    enum_field_pattern = re.compile(r'\n\s*?(\w+)\s*(?:=\s*[^,}\s]+)?\s*(?:,|})', re.MULTILINE | re.DOTALL)
    class_pattern = re.compile(r'(%s|%s|%s)\s+(\w+)\s*(:\s*public\s+([^<\s]+)[^{]*)?\s*\{' % (EmptyScope.struct_str, EmptyScope.class_str, EmptyScope.enum_str), re.MULTILINE | re.DOTALL)
    cb_obj_pattern = re.compile(r'chainbase::object$')
    obj_pattern = re.compile(r'^object$')
    using_pattern = re.compile(r'\n\s*?using\s+(\w+)\s*=\s*([\w:]+)(?:<.*>)?;')

    def __init__(self, name, inherit, start, content, parent_scope, is_enum):
        debug("ClassStruct.__init__ %s %d" % (name, start))
        EmptyScope.__init__(self, name, start, content, parent_scope)
        self.classes = {}
        self.pattern = ClassStruct.class_pattern
        self.is_enum = is_enum
        self.inherit = None
        if inherit is None:
            self.ignore_id = False
        else:
            match = ClassStruct.cb_obj_pattern.search(inherit)
            if match is None:
                match = ClassStruct.obj_pattern.search(inherit)

            self.ignore_id = True if match else False
            next = self.parent_scope
            while self.inherit is None and next is not None:
                self.inherit = next.find_class(inherit)
                next = next.parent_scope
            debug("Checking for object, ignore_id: %s, inherit: %s, name: %s" % (self.ignore_id, inherit, name))

    def add(self, child):
        debug("ClassStruct.add %s (%s) to %s (%s)" % (child.name, child.__class__.__name__, self.name, self.__class__.__name__))
        if isinstance(child, ClassStruct):
            self.classes[child.name] = child
            self.children[child.name] = child
            self.children_ordered.append(child.name)

    def add_fields(self, start, end):
        loc = start
        while loc < end:
            debug("ClassStruct.add_fields -{\n%s\n}" % (self.content[loc:end + 1]))
            if self.is_enum:
                loc = self.add_enum_field(loc, end)
            else:
                loc = self.add_field(loc, end)
        debug("ClassStruct.add_fields done")

    def add_field(self, loc, end):
        match = ClassStruct.field_pattern.search(self.content[loc:end + 1])
        if match is None:
            return end
        field = match.group(2)
        self.fields.append(field)
        all = match.group(0)
        loc = self.content.find(all, loc) + len(all)
        debug("ClassStruct.add_field - %s (%d) - %s" % (field, len(self.fields), ClassStruct.field_pattern.pattern))
        return loc

    def add_enum_field(self, loc, end):
        match = ClassStruct.enum_field_pattern.search(self.content[loc:end + 1])
        if match is None:
            return end
        field = match.group(1)
        self.fields.append(field)
        all = match.group(0)
        loc = self.content.find(all, loc) + len(all)
        debug("ClassStruct.add_enum_field - %s (%d) - %s" % (field, len(self.fields), ClassStruct.enum_field_pattern.pattern))
        return loc

    def add_usings(self, start, end):
        loc = start
        while loc < end:
            debug("ClassStruct.add_usings -{\n%s\n}" % (self.content[loc:end + 1]))
            match = ClassStruct.using_pattern.search(self.content[loc:end])
            if match is None:
                break
            using = match.group(1)
            class_struct = match.group(2)
            self.usings[using] = class_struct
            all = match.group(0)
            loc = self.content.find(all, loc) + len(all)
            debug("ClassStruct.add_usings - %s (%d)" % (using, len(self.usings)))
        debug("ClassStruct.add_usings done")

    def next_scope(self, end = None):
        new_scope = None
        if end is None:
            end = self.find_possible_end()
        debug("ClassStruct.next_scope end=%s on %s\n\npossible scope={\n\"%s\"\n\n\npattern=%s" % (end, self.name, self.content[self.current:end], self.pattern.pattern))
        match = self.pattern.search(self.content[self.current:end])
        start = -1
        search_str = None
        type = None
        name = None
        inherit = None
        if match:
            debug("ClassStruct.next_scope match on %s" % (self.name))
            search_str = match.group(0)
            type = match.group(1)
            name = match.group(2)
            if len(match.groups()) >= 3:
                inherit = match.group(4)

            start = self.find_scope_start(self.content, self.current, end, search_str)
            debug("all: %s, type: %s, name: %s, start: %s, inherit: %s" % (search_str, type, name, start, inherit))

        generic_scope_start = self.find_scope_start(self.content, self.current, end, EmptyScope.start_char)
        if start == -1 and generic_scope_start == -1:
            debug("ClassStruct.next_scope end=%s no scopes add_fields and exit" % (end))
            self.add_fields(self.current, end)
            return None

        debug("found \"%s\" - \"%s\" - \"%s\" current=%s, start=%s, end=%s, pattern=%s " % (search_str, type, name, self.current, start, end, self.pattern.pattern))
        # determine if there is a non-namespace/non-class/non-struct scope before a namespace/class/struct scope
        if start != -1 and (generic_scope_start == -1 or start <= generic_scope_start):
            debug("found %s at %d" % (type, start))
            new_scope = create_scope(type, name, inherit, start, self.content, self)
        else:
            debug("found EmptyScope (%s) at %d, next scope at %s" % (type, generic_scope_start, start))
            new_scope = EmptyScope("", generic_scope_start, self.content, self)

        self.add_fields(self.current, new_scope.start)
        self.add_usings(self.current, new_scope.start)
        new_scope.read()
        self.current = new_scope.end + 1

        return new_scope

class Namespace(ClassStruct):
    namespace_class_pattern = re.compile(r'(%s|%s|%s|%s)\s+(\w+)\s*(:\s*public\s+([^<\s]+)[^{]*)?\s*\{' % (EmptyScope.namespace_str, EmptyScope.struct_str, EmptyScope.class_str, EmptyScope.enum_str), re.MULTILINE | re.DOTALL)

    def __init__(self, name, inherit, start, content, parent_scope):
        debug("Namespace.__init__ %s %d" % (name, start))
        assert inherit is None, "namespace %s should not inherit from %s" % (name, inherit)
        ClassStruct.__init__(self, name, None, start, content, parent_scope, is_enum = False)
        self.namespaces = {}
        self.pattern = Namespace.namespace_class_pattern

    def add(self, child):
        debug("Namespace.add %s (%s) to %s (%s)" % (child.name, child.__class__.__name__, self.name, self.__class__.__name__))
        if isinstance(child, ClassStruct):
            ClassStruct.add(self, child)
            return
        if isinstance(child, Namespace):
            self.namespaces[child.name] = child
            self.children[child.name] = child
            self.children_ordered.append(child.name)

class Reflection:
    def __init__(self, name):
        self.name = name
        self.fields = []
        self.ignored = []
        self.swapped = []
        self.absent = []

class Reflections:
    def __init__(self, content):
        self.content = content
        self.current = 0
        self.end = len(content)
        self.classes = OrderedDict()
        self.with_2_comments = re.compile(r'(//\s*(%s|%s)\s+([^/]*?)\s*\n\s*//\s*(%s|%s)\s+([^/]*?)\s*\n\s*(%s%s\s*\(\s*(\w[^\s<]*))(?:<[^>]*>)?\s*,)' % (ignore_str, swap_str, ignore_str, swap_str, fc_reflect_str, fc_reflect_possible_enum_ext), re.MULTILINE | re.DOTALL)
        self.with_comment = re.compile(r'(//\s*(%s|%s)\s+([^/]*?)\s*\n\s*(%s%s\s*\(\s*(\w[^\s<]*))(?:<[^>]*>)?\s*,)' % (ignore_str, swap_str, fc_reflect_str, fc_reflect_possible_enum_ext), re.MULTILINE | re.DOTALL)
        self.reflect_pattern = re.compile(r'(\b(%s%s\s*\(\s*(\w[^\s<]*)(?:<[^>]*>)?\s*,)\s*(\(.*?\))\s*\))[^\)]*%s%s\b' % (fc_reflect_str, fc_reflect_possible_enum_ext, fc_reflect_str, fc_reflect_possible_enum_ext), re.MULTILINE | re.DOTALL)
        self.field_pattern = re.compile(r'\(([^\)]+)\)', re.MULTILINE | re.DOTALL)
        self.ignore_swap_pattern = re.compile(r'\b([\w\d]+)\b', re.MULTILINE | re.DOTALL)

    def read(self):
        while self.current < self.end:
            match_2_comments = self.with_2_comments.search(self.content[self.current:])
            match_comment = self.with_comment.search(self.content[self.current:])
            match_reflect = self.reflect_pattern.search(self.content[self.current:])
            match_loc = None
            if match_2_comments or match_comment:
                loc1 = self.content.find(match_2_comments.group(1), self.current) if match_2_comments else self.end 
                loc2 = self.content.find(match_comment.group(1), self.current) if match_comment else self.end
                debug("loc1=%s and loc2=%s" % (loc1, loc2))
                group1 = match_2_comments.group(1) if match_2_comments else "<EMPTY>"
                group2 = match_comment.group(1) if match_comment else "<EMPTY>"
                debug("\n  *****       group1={\n%s\n}\n\n\n  *****       group2={\n%s\n}\n\n\n" % (group1, group2))
                if loc2 < loc1:
                    debug("loc2 earlier")
                    match_2_comments = None
                    match_loc = loc2
                else:
                    match_loc = loc1
            if match_reflect and match_loc is not None:
                debug("match_reflect and one of the other matches")
                loc1 = self.content.find(match_reflect.group(1), self.current)
                if loc1 < match_loc:
                    debug("choose the other matches")
                    match_comment = None
                    match_2_comments = None
                else:
                    debug("choose comment")
                    pass

            if match_2_comments:
                debug("match_2_comments")
                debug("Groups {")
                for g in match_2_comments.groups():
                    debug("  %s" % g)
                debug("}")
                assert len(match_2_comments.groups()) == 7, "match_2_comments wrong size due to regex pattern change"
                ignore_or_swap1 = match_2_comments[2]
                next_reflect_ignore_swap1 = match_2_comments[3]
                ignore_or_swap2 = match_2_comments[4]
                next_reflect_ignore_swap2 = match_2_comments[5]
                search_string_for_next_reflect_class = match_2_comments[6]
                next_reflect_class = match_2_comments[7]
                self.add_ignore_swaps(next_reflect_class, next_reflect_ignore_swap1, ignore_or_swap1)
                self.add_ignore_swaps(next_reflect_class, next_reflect_ignore_swap2, ignore_or_swap2)
            elif match_comment:
                debug("match_comment")
                debug("Groups {")
                for g in match_comment.groups():
                    debug("  %s" % g)
                debug("}")
                assert len(match_comment.groups()) == 5, "match_comment too short due to regex pattern change"
                # not using array indices here because for some reason the type of match_2_comments and match_comment are different
                ignore_or_swap = match_comment.group(2)
                next_reflect_ignore_swap = match_comment.group(3)
                search_string_for_next_reflect_class = match_comment.group(4)
                next_reflect_class = match_comment.group(5)
                self.add_ignore_swaps(next_reflect_class, next_reflect_ignore_swap, ignore_or_swap)

            if match_reflect:
                debug("match_reflect")
                debug("Groups {")
                for g in match_reflect.groups():
                    debug("  %s" % g)
                debug("}")
                assert len(match_reflect.groups()) == 4, "match_reflect too short due to regex pattern change"
                next_reflect = match_reflect.group(2)
                next_reflect_class = match_reflect.group(3)
                next_reflect_fields = match_reflect.group(4)
                self.add_fields(next_reflect, next_reflect_class, next_reflect_fields)
            else:
                debug("search for next reflect done")
                self.current = self.end
                break

    def find_or_add(self, reflect_class):
        if reflect_class not in self.classes:
            debug("find_or_add added \"%s\"" % (reflect_class))
            self.classes[reflect_class] = Reflection(reflect_class)
        return self.classes[reflect_class]

    def add_fields(self, next_reflect, next_reflect_class, next_reflect_fields):
        old = self.current
        self.current = self.content.find(next_reflect, self.current) + len(next_reflect)
        debug("all={\n\n%s\n\nclass=\n\n%s\n\nfields=\n\n%s\n\n" % (next_reflect, next_reflect_class, next_reflect_fields))
        fields = re.findall(self.field_pattern, next_reflect_fields)
        for field in fields:
            self.add_field(next_reflect_class, field)
        reflect_class = self.find_or_add(next_reflect_class)
        debug("add_fields %s done, fields count=%s, ignored count=%s, swapped count=%s" % (next_reflect_class, len(reflect_class.fields), len(reflect_class.ignored), len(reflect_class.swapped)))

    def add_ignore_swaps(self, next_reflect_class, next_reflect_ignores_swaps, ignore_or_swap):
        debug("class=\n\n%s\n\n%s=\n\n%s\n\n" % (next_reflect_class, ignore_or_swap, next_reflect_ignores_swaps))
        end = len(next_reflect_ignores_swaps)
        current = 0
        while current < end:
            ignore_swap_match = self.ignore_swap_pattern.search(next_reflect_ignores_swaps[current:])
            if ignore_swap_match:
                ignore_swap = ignore_swap_match.group(1)
                reflect_class = self.find_or_add(next_reflect_class)
                if (ignore_or_swap == ignore_str):
                    assert ignore_swap not in reflect_class.ignored, "Reflection for %s repeats %s \"%s\"" % (next_reflect_class, ignore_or_swap)
                    assert ignore_swap not in reflect_class.swapped, "Reflection for %s references field \"%s\" in %s  and %s " % (next_reflect_class, ignore_swap, ignore_str, swap_str)
                    reflect_class.ignored.append(ignore_swap)
                else:
                    assert ignore_swap not in reflect_class.swapped, "Reflection for %s repeats %s \"%s\"" % (next_reflect_class, ignore_or_swap)
                    assert ignore_swap not in reflect_class.ignored, "Reflection for %s references field \"%s\" in %s  and %s " % (next_reflect_class, ignore_swap, swap_str, ignore_str)
                    reflect_class.swapped.append(ignore_swap)
                debug("ignore or swap %s --> %s, ignored count=%s, swapped count=%s" % (next_reflect_class, ignore_swap, len(reflect_class.ignored), len(reflect_class.swapped)))
                current = next_reflect_ignores_swaps.find(ignore_swap_match.group(0), current) + len(ignore_swap_match.group(0))
            else:
                break
        

    def add_field(self, reflect_class_name, field):
        reflect_class = self.find_or_add(reflect_class_name)
        assert field not in reflect_class.fields, "Reflection for %s repeats field \"%s\"" % (reflect_class_name, field)
        reflect_class.fields.append(field)
        debug("add_field %s --> %s" % (reflect_class_name, field))

def replace_multi_line_comment(match):
    all=match.group(1)
    all=EmptyScope.strip_extra_pattern.sub("", all)
    debug("multiline found=%s" % (all))
    match=EmptyScope.ignore_swap_pattern.search(all)
    if match:
        ignore_or_swap = match.group(1) 
        all = match.group(2)
        debug("multiline %s now=%s" % (ignore_or_swap, all))
        invalid_chars=EmptyScope.invalid_chars_pattern.search(all)
        if invalid_chars:
            for ic in invalid_chars.groups():
                debug("invalid_char=%s" % (ic))
            debug("WARNING: looks like \"%s\" is intending to %s, but there are invalid characters - \"%s\"" % (all, ignore_or_swap, ",".join(invalid_chars.groups())))
            return ""
        groups=re.findall(EmptyScope.multi_line_comment_ignore_swap_pattern, all)
        if groups is None:
            return ""
        rtn_str="// %s " % (ignore_or_swap)
        rtn_str+=', '.join([group for group in groups if group is not None])
        debug("multiline rtn_str=%s" % (rtn_str))
        return rtn_str
    
    debug("multiline no match")
    return ""

def replace_line_comment(match):
    all=match.group(0)
    debug("singleline found=%s" % (all))
    if EmptyScope.single_comment_ignore_swap_pattern.match(all):
        return all
    else:
        return "\n"

def validate_file(file):
    f = open(file, "r")
    contents = "\n" + f.read()   # lazy fix for complex regex
    f.close()
    contents = EmptyScope.multi_line_comment_pattern.sub(replace_multi_line_comment, contents)
    contents = EmptyScope.single_comment_pattern.sub(replace_line_comment, contents)
    found = re.search(fc_reflect_str, contents)
    if found is None:
        return
    print("validate %s" % (file))
    debug("validate %s" % (file))
    global_namespace=Namespace("", None, 0, contents, None)
    global_namespace.read()
    if args.debug:
        _, filename = os.path.split(file)
        with open(os.path.join(temp_dir, filename + ".struct"), "w") as f:
            f.write("global_namespace=%s" % (global_namespace))
        with open(os.path.join(temp_dir, filename + ".stripped"), "w") as f:
            f.write(contents)
    reflections=Reflections(contents)
    reflections.read()
    for reflection_name in reflections.classes:
        reflection = reflections.classes[reflection_name]
        class_struct = global_namespace.find_class(reflection_name)
        if class_struct is None:
            match=re.search(r'^(.+?)::id_type$', reflection_name)
            if match:
                parent_class_name = match.group(1)
                parent_class = global_namespace.find_class(parent_class_name)
                if parent_class.ignore_id:
                    # this is a chainbase::object, don't need to worry about id_type definition
                    continue
        class_struct_num_fields = len(class_struct.fields) if class_struct is not None else None 
        debug("reflection_name=%s, class field count=%s, reflection field count=%s, ingore count=%s, swap count=%s" % (reflection_name, class_struct_num_fields, len(reflection.fields), len(reflection.ignored), len(reflection.swapped)))
        assert isinstance(class_struct, ClassStruct), "could not find a %s/%s/%s for %s" % (EmptyScope.class_str, EmptyScope.struct_str, EmptyScope.enum_str, reflection_name)
        if class_struct.ignore_id:
            id_field = "id"
            if id_field not in reflection.ignored and id_field not in reflection.fields:
                debug("Object ignore_id Adding id to ignored for %s" % (reflection_name))
                reflection.ignored.append(id_field)
        else:
            debug("Object ignore_id NOT adding id to ignored for %s" % (reflection_name))
        rf_index = 0
        rf_len = len(reflection.fields)

        processed = []
        back_swapped = []
        fwd_swapped = []
        ignored = []
        f_index = 0
        f_len = len(class_struct.fields)
        while f_index < f_len:
            field = class_struct.fields[f_index]
            reflect_field = reflection.fields[rf_index] if rf_index < rf_len else None
            processed.append(field)
            debug("\nfield=%s reflect_field=%s" % (field, reflect_field))
            if field in reflection.swapped:
                debug("field \"%s\" swapped (back)" % (field))
                reflection.swapped.remove(field)
                back_swapped.append(field)
                assert field in reflection.fields, "Reflection for %s indicates swapping %s but swapped position is not indicated in the reflection fields. Should it be ignored?" % (reflection_name, field)
                assert reflect_field != field, "Reflection for %s should not indicate swapping %s since it is in the correct order" % (reflection_name, field)
                f_index += 1
                continue
            if reflect_field in reflection.swapped:
                debug("field \"%s\" swapped (fwd)" % (field))
                reflection.swapped.remove(reflect_field)
                fwd_swapped.append(reflect_field)
                assert reflect_field in reflection.fields, "Reflection for %s indicates swapping field %s but it doesn't exist in that class/struct so it should be removed" % (reflection_name, reflect_field)
                rf_index += 1
                continue
            assert reflect_field not in ignored, "Reflection for %s should not indicate %s for %s; it should indicate %s - %s" % (reflection_name, ignore_str, reflect_field, swap_str, ",".join(ignored))
            if field in reflection.ignored:
                debug("ignoring: %s" % (field))
                reflection.ignored.remove(field)
                ignored.append(field)
                assert reflect_field != field, "Reflection for %s should not indicate ignoring %s since it is in the correct order" % (reflection_name, field)
                f_index += 1
                continue
            debug("ignored=%s, swapped=%s" % (",".join(reflection.ignored),",".join(reflection.swapped)))
            if reflect_field is not None and reflect_field in back_swapped:
                back_swapped.remove(reflect_field)
                rf_index += 1
            elif field in fwd_swapped:
                fwd_swapped.remove(field)
                f_index += 1
            else:
                assert reflect_field == field, "Reflection for %s should have field %s instead of %s or else it should indicate if the field should be ignored (%s) or swapped (%s)" %(reflection_name, field, reflect_field, ignore_str, swap_str)
                f_index += 1
                rf_index += 1
            debug("rf_index=%s, rf_len=%s, f_index=%s, f_len=%s" % (rf_index, rf_len, f_index, f_len))

        assert len(reflection.ignored) == 0, "Reflection for %s has erroneous ignores - \"%s\"" % (reflection_name, ",".join(reflection.ignored))
        unused_reflect_fields = []
        while rf_index < rf_len:
            debug("rf_index=%s, rf_len=%s fields=%s" % (rf_index, rf_len, ",".join(reflection.fields)))
            reflect_field = reflection.fields[rf_index]
            if reflect_field in back_swapped:
                back_swapped.remove(reflect_field)
            else:
                unused_reflect_fields.append(reflect_field)
            rf_index += 1
        assert len(unused_reflect_fields) == 0, "Reflection for %s has fields not in definition for class/struct - \"%s\"" % (reflection_name, ",".join(unused_reflect_fields))
        assert len(reflection.swapped) == 0, "Reflection for %s has erroneous swaps - \"%s\"" % (reflection_name, ",".join(reflection.swapped))
        assert len(back_swapped) == 0, "Reflection for %s indicated swapped fields that were never provided - \"%s\"" % (reflection_name, ",".join(back_swapped))
        assert len(fwd_swapped) == 0, "Reflection for %s indicated and provided swapped fields that are not in the class - \"%s\"" % (reflection_name, ",".join(fwd_swapped))

    print("%s passed" % (file))

success = True

def walk(current_dir):
    result = True
    print("Searching for files: %s" % (current_dir))
    for root, dirs, filenames in os.walk(current_dir):
        for filename in filenames:
            _, extension = os.path.splitext(filename)
            if extension not in extensions:
                continue
            try:
                validate_file(os.path.join(root, filename))
            except AssertionError:
                _, info, tb = sys.exc_info()
                traceback.print_tb(tb) # Fixed format
                tb_info = traceback.extract_tb(tb)
                filename, line, func, text = tb_info[-1]

                print("An error occurred in %s:%s: %s" % (filename, line, info), file=sys.stderr)
                if args.exit_on_error:
                    exit(1)
                result = False

        if not recurse:
            break
    return result

for file in args.files:
    if os.path.isdir(file):
        success &= walk(file)
    elif os.path.isfile(file):
        try:
            validate_file(file)
        except AssertionError:
            _, info, tb = sys.exc_info()
            traceback.print_tb(tb) # Fixed format
            tb_info = traceback.extract_tb(tb)
            filename, line, func, text = tb_info[-1]

            print("An error occurred in %s:%s: %s" % (filename, line, info), file=sys.stderr)
            if args.exit_on_error:
                exit(1)
            success = False
    else:
        print("ERROR \"%s\" is neither a directory nor a file" % file)
        success = False

if success:
    exit(0)
else:
    exit(1)
