" Vim syntax file
" Language: stamp
" Latest Revision: 06/12/2022

if exists("b:current_syntax")
	finish
endif

let s:cpo_save = &cpo
set cpo&vim

let s:stamp_keywords = {
	\   'stampConditional' : ["if", "else"]
	\ , 'stampRepeat' : ["while"]
	\ , 'stampExec' : ["return", "break", "continue"]
	\ , 'stampBool' : ["True", "False"]
	\ , 'stampKeyword' : ["fn"]
	\ , }

function! s:syntax_keyword(dict)
	for key in keys(a:dict)
		execute 'syntax keyword' key join(a:dict[key], ' ')
	endfor
endfunction

call s:syntax_keyword(s:stamp_keywords)

syntax match stamptDecNumber display   "\v<\d%(_?\d)*"
syntax match stampHexNumber display "\v<0x\x%(_?\x)*"
syntax match stampOctNumber display "\v<0o\o%(_?\o)*"
syntax match stampBinNumber display "\v<0b[01]%(_?[01])*"

syntax region stampBlock start="{" end="}" transparent fold
syntax region stampComment start="//" end="$"

syntax region stampString matchgroup=stampStringDelimiter start=+"+ skip=+\\\\\|\\"+ end=+"+ oneline contains=stampEscape
syntax region stampChar matchgroup=stampCharDelimiter start=+'+ skip=+\\\\\|\\'+ end=+'+ oneline contains=stampEscape
syntax match stampEscape        display contained /\\./

syntax match stampFunction /\w\+\s*(/me=e-1,he=e-1
syntax match stampType /[A-Z][A-Za-z0-9_]*/

highlight default link stampDecNumber stampNumber
highlight default link stampHexNumber stampNumber
highlight default link stampOctNumber stampNumber
highlight default link stampBinNumber stampNumber

highlight default link stampKeyword Keyword
highlight default link stampType Type
highlight default link stampComment Comment
highlight default link stampString String
highlight default link stampStringDelimeter String
highlight default link stampChar String
highlight default link stampCharDelimeter String
highlight default link stampEscape Special
highlight default link stampBool Boolean
highlight default link stampNumber Number
highlight default link stampExec Special
highlight default link stampRepeat Repeat

delfunction s:syntax_keyword

let b:current_syntax = "stamp"

let &cpo = s:cpo_save
unlet! s:cpo_save
