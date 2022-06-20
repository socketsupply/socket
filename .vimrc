let g:clang_format#auto_format=1

let g:ale_c_clang_options = ' -I/usr/lib/jvm/java-18-openjdk-amd64/include'
let g:ale_c_clang_options .= ' -I/usr/lib/jvm/java-18-openjdk-amd64/include/linux'
let g:ale_kotlin_ktlint_options = '--android'

let g:android_sdk_path = $HOME . '/android'

au BufNewFile,BufRead *.h set filetype=cpp
au BufNewFile,BufRead *.h set syntax=cpp

set tabstop=2
set shiftwidth=2
set softtabstop=2

set smarttab
set expandtab
set autoindent
set smartindent
set nocompatible

fun! CFormatSettings()
  setlocal equalprg=clang-format
endfun

autocmd FileType c,cpp call CFormatSettings()
autocmd BufEnter *.kt ALEDisable
autocmd BufLeave *.kt ALEEnable

fun s:c()
  " Comments
  syn keyword   Todo             TODO FIXME XXX TBD contained
  syn region    Comment          start=/\/\// end=/$/ contains=Todo,@Spell
  syn region    Comment          start=/\/\*/ end=/\*\// contains=Todo,@Spell

  hi def link   Todo             Todo
  hi def link   Comment          Comment

  syn match     NatspecTag       /libopc/ contained
  syn match     NatspecTag       /SPDX-License-Identifier/ contained
  syn match     NatspecTag       /SPDX-FileCopyrightText/ contained

  " Natspec
  syn match     NatspecTag       /@see\>/ contained
  syn match     NatspecTag       /@dev\>/ contained
  syn match     NatspecTag       /@title\>/ contained
  syn match     NatspecTag       /@author\>/ contained
  syn match     NatspecTag       /@notice\>/ contained
  syn match     NatspecTag       /@param\>/ contained
  syn match     NatspecTag       /@prop\>/ contained
  syn match     NatspecTag       /@return\>/ contained
  syn match     NatspecTag       /@throws\>/ contained
  syn match     NatspecTag       /@returns\>/ contained
  syn match     NatspecTag       /@example\>/ contained
  syn match     NatspecParam     /\(@param\s*\)\@<=\<[a-zA-Z_][0-9a-zA-Z_]*/
  syn match     NatspecParam     /\(@prop\s*\)\@<=\<[a-zA-Z_][0-9a-zA-Z_]*/
  syn region    NatspecBlock     start=/\/\/\// end=/$/ contains=Todo,NatspecTag,NatspecParam,@Spell
  syn region    NatspecBlock     start=/\/\*\{2}/ end=/\*\// contains=Todo,NatspecTag,NatspecParam,@Spell

  hi def link   NatspecTag       SpecialComment
  hi def link   NatspecBlock     Comment
  hi def link   NatspecParam     Define
endfun

augroup ft_c
  autocmd!
  autocmd Syntax c call s:c()
  autocmd Syntax cpp call s:c()
  autocmd Syntax kotlin call s:c()
augroup end
