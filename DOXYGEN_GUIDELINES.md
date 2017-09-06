# Doxygen Guide for EOS
<br/>

## Index
- [Introduction](#Introduction)
- [How to generate/update the EOS documentation](#How-to-generate/update-the-EOS-documentation)
- [How to view the EOS documentation](#How-to-view-the-EOS-documentation)
- [How to create doxygen style EOS documentation](#How-to-create-doxygen-style-EOS-documentation)
- [Doxygen Command List](#Doxygen-Command-List)
  * [brief](#@brief)
  * [class](#class)
  * [defgroup](#defgroup)
  * [file](#file)
  * [ingroup](#ingroup)
  * [note](#note)
  * [param](#param)
  * [pre](#pre)
  * [ref](#ref)
  * [return](#return)
  * [section](#section)
  * [see](#see)
  * [subsection](#subsection)
  * [throws](#throws)
  * [tparam](#tparam)

<br/><br/>
## Introduction
Doxygen is a documentation generator; A tool for writing software reference documentation. The documentation is written within code, and is thus relatively easy to keep up to date.

Doxygen can cross reference documentation and code, so that the reader of a document can easily refer to the actual code. Doxygen extracts documentation from source file comments and has built-in support to generate inheritance diagrams for C++ classes. For more advanced diagrams and graphs, doxygen can use the "dot" tool from Graphviz (http://www.graphviz.org/).

Doxygen supports most Unix-like systems, macOS, and Windows.
It can be downloaded from git, ftp or from the web as source, installer or as zipped version from below location:
http://www.stack.nl/~dimitri/doxygen/download.html
<br/>
<br/>
## How to generate/update the EOS documentation
After downloading and installing doxygen, it needs to be run in order to analyze all source code comments and build the corresponding documentation.

To be configurable, doxygen uses a configuration file to determine all of its settings. The configuration file is a free-form ASCII text file, which consists of a list of assignment statements and a structure similar to that of a Makefile.

To generate the documentation you point doxygen to the configuration file as a parameter as follows:

```
doxygen <config-file>
```
<br/>
An alphabetical index of the tags that are supported in the configuration file is situated here:
https://www.stack.nl/~dimitri/doxygen/manual/config.html


For the EOS repository the configuration file is situated in the root directory of the repository (eos.doxygen).

So to generate or update the EOS documentation from command line, navigate to the root folder of the EOS repository and trigger the document generation as follows

```
doxygen eos.doxygen
```
<br/>
<br/>

## How to view the EOS documentation

The EOS doxygen configuration file is set up, so that the documentation will be created as a series of connected HTML files.

The generated HTML documentation can be viewed via browser, whereas the document root is the index.html file in the docs subfolder of the EOS repository. A direct link can be found here:
https://eosio.github.io/eos/
<br/>
<br/>

## How to create doxygen style EOS documentation

For information to be picked up by doxygen, it needs to be contained in a special comment block.

A special comment block is a C or C++ style comment block with some additional markings, so doxygen knows it is a piece of structured text that needs to end up in the generated documentation.

Doxygen picks up information in all allowed standard comment blocks, such as in the following multiline comment examples:

```
/**
<A short one line description>

<Longer description>
<May span multiple lines or paragraphs as needed>

@param  Description of method's or function's input parameter
@param  ...
@return Description of the return value
*/
```

```
/**
 * <A short one line description>
 *
 * <Longer description>
 * <May span multiple lines or paragraphs as needed>
 *
 * @param  Description of method's or function's input parameter
 * @param  ...
 * @return Description of the return value
 */
```
<br/>
When using C++ style single line comments, doxygen  parses only comments with additional slashes, such as:

```
/// <A short one line description>
///
/// <Longer description>
/// <May span multiple lines or paragraphs as needed>
///
/// @param  Description of method's or function's input parameter
/// @param  ...
/// @return Description of the return value
```
<br/>
Ordinarily comments are being placed **BEFORE** the actual item, however in certain cases such as
documenting the members of a file, struct, union, class or enum as well as the parameters of a function, it is sometimes desired to place the documentation block **AFTER** the member.

For this purpose you have to put an additional < marker in the comment block in order for doxygen to map the information correctly such as demonstrated in below examples:

```
int var; /**< Detailed description after the member */
```

```
int var; ///< Detailed description after the member
```
<br/>
Special commands such as param and return in the above examples start either with a backslash (\) or an at-sign (@), so both of the following choices are equivalent:

```
/**
   @brief  A short description
*/
```
```
/**
   \brief  A short description
*/
```
<br/>
<br/>

## Doxygen Command List

The list of commands recognised by doxygen is quite exhaustive; (An alphabetically sorted list of all commands can be found here: http://www.stack.nl/~dimitri/doxygen/manual/commands.html).

However in the EOS documentation we are currently using a small subset of the above list. Below is a full list of commands used in the EOS documentation with explanation:

Note:<br/>
\<sharp\> braces are used for single word arguments<br/>
(round) braces are used for arguments that extends until the end of the line<br/>
{curly} braces are used for arguments that extends until the next paragraph<br/>
[square] brackets are used for optinal arguments<br/>

<br/>

#### @brief
Usage: @brief { brief description }<br/><br/>
Starts a paragraph that serves as a brief description. For classes and files the brief description will be used in lists and at the start of the documentation page. For class and file members, the brief description will be placed at the declaration of the member and prepended to the detailed description. A brief description may span several lines (although it is advised to keep it brief!). A brief description ends when a blank line or another sectioning command is encountered. If multiple @brief commands are present they will be joined.

<br/>

#### @class
Usage: @class \<name\> [\<header-file\>] [\<header-name\>]<br/><br/>
Indicates that a comment block contains documentation for a class with name \<name\>. Optionally a header file and a header name can be specified. If the header-file is specified, a link to a verbatim copy of the header will be included in the HTML documentation. The \<header-name\> argument can be used to overwrite the name of the link that is used in the class documentation to something other than \<header-file\>. With the \<header-name\> argument you can also specify how the include statement should look like, by adding either quotes or sharp brackets around the name. Sharp brackets are used if just the name is given.

Example:
```
/* A dummy class */
class Test
{
};
/**
*  @class Test class.h "inc/class.h"
*  @brief This is a test class.
*
*  Some details about the Test class.
*/
```

<br/>

#### @defgroup
Usage: @defgroup \<name\> (group title)<br/><br/>
Indicates that a comment block contains documentation for a group of classes, files or namespaces. This can be used to categorize classes, files or namespaces, and document those categories. You can also use groups as members of other groups, thus building a hierarchy of groups.
The \<name\> argument should be a single-word identifier.

<br/>

#### @file
Usage: @file [\<name\>]<br/><br/>
Indicates that a comment block contains documentation for a source or header file with name \<name\>. The file name may include (part of) the path if the file-name alone is not unique. If the file name is omitted (i.e. the line after @file is left blank) then the documentation block that contains the @file command will belong to the file it is located in.
Important:
The documentation of global functions, variables, typedefs, and enums will only be included in the output if the file they are in is documented as well.

Example:
```
/** @file file.h
* A brief file description.
* A more elaborated file description.
*/
/**
* A global integer value.
* More details about this value.
*/
extern int globalValue;
```

<br/>

#### @ingroup
Usage: @ingroup (\<groupname\> [\<groupname\> \<groupname\>])<br/><br/>
If the @ingroup command is placed in a comment block of a class, file or namespace, then it will be added to the group or groups identified by \<groupname\>.

<br/>

#### @note
Usage: @note { text }<br/><br/>
Starts a paragraph where a note can be entered. The paragraph will be indented. The text of the paragraph has no special internal structure. All visual enhancement commands may be used inside the paragraph. Multiple adjacent @note commands will be joined into a single paragraph. Each note description will start on a new line. Alternatively, one @note command may mention several notes. The @note command ends when a blank line or some other sectioning command is encountered.

<br/>

#### @param
Usage: @param [(dir)] \<parameter-name\> { parameter description }<br/><br/>
Starts a parameter description for a function parameter with name \<parameter-name\>, followed by a description of the parameter. The existence of the parameter is checked and a warning is given if the documentation of this (or any other) parameter is missing or not present in the function declaration or definition.
The @param command has an optional attribute, (dir), specifying the direction of the parameter. Possible values are "[in]", "[in,out]", and "[out]", note the [square] brackets in this description. When a parameter is both input and output, [in,out] is used as attribute. Here is an example for the function memcpy:
```
/*!
* Copies bytes from a source memory area to a destination memory area,
* where both areas may not overlap.
* @param[out] dest The memory area to copy to.
* @param[in] src The memory area to copy from.
* @param[in] n The number of bytes to copy
*/
void memcpy(void *dest, const void *src, size_t n);
```
<br/>

The parameter description is a paragraph with no special internal structure. All visual enhancement commands may be used inside the paragraph.
Multiple adjacent @param commands will be joined into a single paragraph. Each parameter description will start on a new line. The @paramdescription ends when a blank line or some other sectioning command is encountered.
Note that you can also document multiple parameters with a single @param command using a comma separated list. Here is an example:
```
/** Sets the position.
* @param x,y,z Coordinates of the position in 3D space.
*/
void setPosition(double x,double y,double z,double t)
{
}
```

<br/>

#### @pre
Usage: @pre { description of the precondition }<br/><br/>
Starts a paragraph where the precondition of an entity can be described. The paragraph will be indented. The text of the paragraph has no special internal structure. All visual enhancement commands may be used inside the paragraph. Multiple adjacent @pre commands will be joined into a single paragraph. Each precondition will start on a new line. Alternatively, one @pre command may mention several preconditions. The @pre command ends when a blank line or some other sectioning command is encountered.

<br/>

#### @ref
Usage: @ref \<name\> ["(text)"]<br/><br/>
Creates a reference to a named section, subsection, page or anchor. For HTML documentation the reference command will generate a link to the section. For a section or subsection the title of the section will be used as the text of the link. For an anchor the optional text between quotes will be used or \<name\> if no text is specified. For documentation the reference command will generate a section number for sections or the text followed by a page number if \<name\> refers to an anchor.

<br/>

#### @return
Usage: @return { description of the return value }<br/><br/>
Starts a return value description for a function. The text of the paragraph has no special internal structure. All visual enhancement commands may be used inside the paragraph. Multiple adjacent @return commands will be joined into a single paragraph. The @return description ends when a blank line or some other sectioning command is encountered.

<br/>

#### @section
Usage: @section \<section-name\> (section title)<br/><br/>
Creates a section with name \<section-name\>. The title of the section should be specified as the second argument of the @section command. This command only works inside related page documentation and not in other documentation blocks!

<br/>

#### @see
Usage: @see { references }<br/><br/>
Starts a paragraph where one or more cross-references to classes, functions, methods, variables, files or URL may be specified. Two names joined by either :: or # are understood as referring to a class and one of its members. One of several overloaded methods or constructors may be selected by including a parenthesized list of argument types after the method name.
Equivalent to @sa.

<br/>

#### @subsection
Usage: @subsection \<subsection-name\> (subsection title)<br/><br/>
Creates a subsection with name \<subsection-name\>. The title of the subsection should be specified as the second argument of the @subsection command. This command only works inside a section of a related page documentation block and not in other documentation blocks!

<br/>

#### @throws
Usage: @throws \<exception-object\> { exception description }<br/><br/>
Starts an exception description for an exception object with name \<exception-object\>. Followed by a description of the exception. The existence of the exception object is not checked. The text of the paragraph has no special internal structure. All visual enhancement commands may be used inside the paragraph. Multiple adjacent @throws commands will be joined into a single paragraph. Each exception description will start on a new line. The @throws description ends when a blank line or some other sectioning command is encountered. Equivalent to @throw.

<br/>

#### @tparam
Usage: @tparam \<template-parameter-name\> { description }<br/><br/>
Starts a template parameter for a class or function template parameter with name \<template-parameter-name\>, followed by a description of the template parameter.
Otherwise similar to @param.
