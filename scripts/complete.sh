#!/usr/bin/env bash

_complete()
{
   COMPREPLY=()

   local cur=${COMP_WORDS[COMP_CWORD]}
   local cmd=${COMP_WORDS[0]}

   choices=$( ${cmd} -- "${COMP_WORDS[@]:1}" 2>/dev/null )
   ret=$?
   
   if [[ "$ret" -ne 0 ]]; then
      return 0
   fi

   local DISPLAY_HELP=1
   if [ "${__COMPLETE_PREV_LINE:-}" != "$COMP_LINE" ] ||
       [ "${__COMPLETE_PREV_POINT:-}" != "$COMP_POINT" ]; then
       __COMPLETE_PREV_LINE=$COMP_LINE
       __COMPLETE_PREV_POINT=$COMP_POINT
       DISPLAY_HELP=
   fi

   EXPANDED_PS1="$(bash --rcfile <(echo "PS1='$PS1'") -i <<<'' 2>&1 | head -n 1)"

   if [[ ! -z "${choices[@]}" ]]; then
      COMPREPLY=( $( compgen -W '${choices}' -- ${cur} ) )
      
      if [[ ! -z "${cur}" ]]; then
         return 0 
      fi
   fi

   if [ -n "$DISPLAY_HELP" ]; then 
      ${cmd} -- "${COMP_WORDS[@]:1}" 2>&1 > /dev/null

      if [[ -z "${choices[@]}" ]]; then
         echo -n "${EXPANDED_PS1}"
         echo -n "${COMP_WORDS[@]}"
      fi
   fi

   return 0
}

complete -F _complete $@
