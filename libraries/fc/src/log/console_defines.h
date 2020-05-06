#ifndef _MACE_CMT_CONSOLE_DEFINES_H_
#define _MACE_CMT_CONSOLE_DEFINES_H_

/// @cond INTERNAL_DEV
/**
    @file console_defines.h
    This header contains definitions for console styles and colors.
    @ingroup tconsole
*/


/**
    @defgroup console_styles Console Styles
    @brief Defines styles that can be used within text printed to the Console.

    Note that styles will not show up when printing to files (in fact, you will
    get a lot of garbage characters if you do this). Also, not all consoles
    support styled text.
    @{
*/
#if COLOR_CONSOLE

#ifndef WIN32

/**
    @def CONSOLE_DEFAULT
    @brief Sets all styles and colors to the console defaults.

    (const char*)
*/
#define CONSOLE_DEFAULT "\033[0m"
/**
    @def CONSOLE_BOLD
    @brief Print bold console text (or brighten foreground color if present).

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BOLD "\033[1m"

/**
    @def CONSOLE_HALF_BRIGHT
    @brief Print half-bright console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_HALF_BRIGHT "\033[2m"

/**
    @def CONSOLE_ITALIC
    @brief Print italic console text.

    @ingroup tconsole
    (const char*) Typically not supported.

    (const char*)
*/
#define CONSOLE_ITALIC "\033[3m"

/**
    @def CONSOLE_UNDERLINE
    @brief Print underlined console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_UNDERLINE "\033[4m"

/**
    @def CONSOLE_BLINK
    @brief Print blinking console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLINK "\033[5m"

/**
    @def CONSOLE_RAPID_BLINK
    @brief Print rapidly blinking console text. Typically not supported.

    (const char*)
*/
#define CONSOLE_RAPID_BLINK "\033[6m"

/**
    @def CONSOLE_REVERSED
    @brief Print console text with foreground and background colors reversed.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_REVERSED "\033[7m"

/**
    @def CONSOLE_CONCEALED
    @brief Print concealed console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_CONCEALED "\033[8m"

/**
    @def CONSOLE_STRIKETHROUGH
    @brief Print strikethrough console text. Typically not supported.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_STRIKETHROUGH "\033[9m"

/// @}


/**
    @defgroup console_colors Console Colors
    @brief Defines colors that can be used within text printed to the Console.

    @ingroup tconsole
    Note that colors will not show up when printing to files (in fact, you will
    get a lot of garbage characters if you do this). Also, not all consoles
    support colored text.
    @{
*/

/**
    @def CONSOLE_BLACK
    @brief Print text with black foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLACK "\033[30m"

/**
    @def CONSOLE_RED
    @brief Print text with red foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_RED "\033[31m"

/**
    @def CONSOLE_GREEN
    @brief Print text with green foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_GREEN "\033[32m"

/**
    @def CONSOLE_BROWN
    @brief Print text with brown foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BROWN "\033[33m"

/**
    @def CONSOLE_BLUE
    @brief Print text with blue foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLUE "\033[34m"

/**
    @def CONSOLE_MAGENTA
    @brief Print text with magenta foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_MAGENTA "\033[35m"

/**
    @def CONSOLE_CYAN
    @brief Print text with cyan foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_CYAN "\033[36m"

/**
    @def CONSOLE_WHITE
    @brief Print text with white foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_WHITE "\033[37m"

/**
    @def CONSOLE_BLACK_BG
    @brief Print text with black background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLACK_BG "\033[40m"

/**
    @def CONSOLE_RED_BG
    @brief Print text with red background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_RED_BG "\033[41m"

/**
    @def CONSOLE_GREEN_BG
    @brief Print text with green background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_GREEN_BG "\033[42m"

/**
    @def CONSOLE_BROWN_BG
    @brief Print text with brown background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BROWN_BG "\033[43m"

/**
    @def CONSOLE_BLUE_BG
    @brief Print text with blue background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLUE_BG "\033[44m"

/**
    @def CONSOLE_MAGENTA_BG
    @brief Print text with magenta background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_MAGENTA_BG "\033[45m"

/**
    @def CONSOLE_CYAN_BG
    @brief Print text with cyan background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_CYAN_BG "\033[46m"

/**
    @def CONSOLE_WHITE_BG
    @brief Print text with white background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_WHITE_BG "\033[47m"



#else // WIN32
#include <winsock2.h>
#include <windows.h>
//#include <stdlib.h>
#include <conio.h>

/**
    @def CONSOLE_DEFAULT
    @brief Sets all styles and colors to the console defaults.

    (const char*)
*/
#define CONSOLE_DEFAULT (FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN)
/**
    @def CONSOLE_BOLD
    @brief Print bold console text (or brighten foreground color if present).

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BOLD FOREGROUND_INTENSITY

/**
    @def CONSOLE_HALF_BRIGHT
    @brief Print half-bright console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_HALF_BRIGHT 

/**
    @def CONSOLE_ITALIC
    @brief Print italic console text.

    @ingroup tconsole
    (const char*) Typically not supported.

    (const char*)
*/
#define CONSOLE_ITALIC 

/**
    @def CONSOLE_UNDERLINE
    @brief Print underlined console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_UNDERLINE COMMON_LVB_UNDERSCORE

/**
    @def CONSOLE_BLINK
    @brief Print blinking console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLINK 

/**
    @def CONSOLE_RAPID_BLINK
    @brief Print rapidly blinking console text. Typically not supported.

    (const char*)
*/
#define CONSOLE_RAPID_BLINK 

/**
    @def CONSOLE_REVERSED
    @brief Print console text with foreground and background colors reversed.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_REVERSED CONSOLE_LVB_REVERSE_VIDEO

/**
    @def CONSOLE_CONCEALED
    @brief Print concealed console text.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_CONCEALED 

/**
    @def CONSOLE_STRIKETHROUGH
    @brief Print strikethrough console text. Typically not supported.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_STRIKETHROUGH 

/// @}


/**
    @defgroup console_colors Console Colors
    @brief Defines colors that can be used within text printed to the Console.

    @ingroup tconsole
    Note that colors will not show up when printing to files (in fact, you will
    get a lot of garbage characters if you do this). Also, not all consoles
    support colored text.
    @{
*/

/**
    @def CONSOLE_BLACK
    @brief Print text with black foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLACK 0

/**
    @def CONSOLE_RED
    @brief Print text with red foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_RED FOREGROUND_RED

/**
    @def CONSOLE_GREEN
    @brief Print text with green foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_GREEN FOREGROUND_GREEN

/**
    @def CONSOLE_BROWN
    @brief Print text with brown foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BROWN (FOREGROUND_RED | FOREGROUND_GREEN)

/**
    @def CONSOLE_BLUE
    @brief Print text with blue foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLUE FOREGROUND_BLUE

/**
    @def CONSOLE_MAGENTA
    @brief Print text with magenta foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_MAGENTA (CONSOLE_RED | CONSOLE_BLUE)

/**
    @def CONSOLE_CYAN
    @brief Print text with cyan foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_CYAN (CONSOLE_BLUE | CONSOLE_GREEN)

/**
    @def CONSOLE_WHITE
    @brief Print text with white foreground.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_WHITE (CONSOLE_RED | CONSOLE_BLUE | CONSOLE_GREEN)

/**
    @def CONSOLE_BLACK_BG
    @brief Print text with black background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLACK_BG 0

/**
    @def CONSOLE_RED_BG
    @brief Print text with red background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_RED_BG (BACKGROUND_RED)

/**
    @def CONSOLE_GREEN_BG
    @brief Print text with green background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_GREEN_BG (BACKGROUND_GREEN)

/**
    @def CONSOLE_BROWN_BG
    @brief Print text with brown background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BROWN_BG (BACKGROUND_RED | BACKGROUND_GREEN)

/**
    @def CONSOLE_BLUE_BG
    @brief Print text with blue background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_BLUE_BG (BACKGROUND_BLUE)

/**
    @def CONSOLE_MAGENTA_BG
    @brief Print text with magenta background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_MAGENTA_BG "\033[45m"

/**
    @def CONSOLE_CYAN_BG
    @brief Print text with cyan background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_CYAN_BG "\033[46m"

/**
    @def CONSOLE_WHITE_BG
    @brief Print text with white background.

    @ingroup tconsole
    (const char*)
*/
#define CONSOLE_WHITE_BG "\033[47m"
#endif


/// @}
#else // On Window's no color output WIN32
#define CONSOLE_DEFAULT ""
#define CONSOLE_BOLD ""
#define CONSOLE_HALF_BRIGHT ""
#define CONSOLE_ITALIC ""
#define CONSOLE_UNDERLINE ""
#define CONSOLE_BLINK ""
#define CONSOLE_RAPID_BLINK "" 
#define CONSOLE_REVERSED ""
#define CONSOLE_CONCEALED ""
#define CONSOLE_STRIKETHROUGH ""
#define CONSOLE_BLACK ""
#define CONSOLE_RED ""
#define CONSOLE_GREEN ""
#define CONSOLE_BROWN ""
#define CONSOLE_BLUE ""
#define CONSOLE_MAGENTA ""
#define CONSOLE_CYAN ""
#define CONSOLE_WHITE ""
#define CONSOLE_BLACK_BG ""
#define CONSOLE_RED_BG ""
#define CONSOLE_GREEN_BG ""
#define CONSOLE_BROWN_BG ""
#define CONSOLE_BLUE_BG  ""
#define CONSOLE_MAGENTA_BG  ""
#define CONSOLE_CYAN_BG  ""
#define CONSOLE_WHITE_BG  ""

/// @}
/// @endcond INTERNAL_DEV
#endif // NOT DEFINED WIN32
#endif // _BOOST_CMT_CONSOLE_DEFINES_H_
