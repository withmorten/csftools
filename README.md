# csftools
SAGE str2csf and csf2str converter

can convert a SAGE csf file that contains a language with ascii characters to str and back. really old code, and i didn't really understand unicode strings back then ...

usage:

`csf2str input.csf output.str`
 
`str2csf input.str output.csf en-us`

csf knows the following languages:

* `en-us`
* `en-uk`
* `de`
* `fr-fr`
* `es`
* `it`
* `ja`
* `jw`
* `ko`
* `cn`

however, i only ever tested it on english and german ...
