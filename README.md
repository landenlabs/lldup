lldup
### OSX / Linux / DOS  Find duplicate files and optionally delete or hardlink them.

Dec-2024
#### Replaced with program lldupdir

https://github.com/landenlabs/lldupdir

<hr>
Visit home website

[https://landenlabs.com](https://landenlabs.com)


<hr>

Help Banner:
<pre>
lldup  Dennis Lang v3.03 (landenlabs.com) Oct  6 2024

Des: Find duplicate files
Use: lldup [options] directories...   or  files

  Options (only first unique characters required, options can be repeated):

   -file             ; Find duplicate files by name (default option)
   Other options available on Windows

  Common options:
   -excludefile=<filePattern> ; File name exclusion patterns
   -includefile=<filePattern> ; File name inclusion patterns
   -invert           ; Invert test output.
   -verbose          ; Show additional info
   -quiet            ; Only show errors

  With -file
   -all                ; Match contents on all files, default same name
   -justName           ; Match duplicate name only, not contents
   -ignoreExtn         ; With -justName, also ignore extension
   -preDivider=<text>  ; Pre group divider output before groups
   -postDivider=<text> ; Post group divider output after groups
   -separator=<text>   ; Separator
   -ignoreEmpty        ; Ignore 0 length files
   -hardlink           ; Hardlink matches
   -hash               ; Compute xxhash64 on file
   -md5                ; Compute md5 on file
   -no                 ; dry run with -hardlink

  With -compareAxx or -duplicateAxx
   -key=<decrypt_key>

 Examples:
   Warning - Escape * as in foo.\* or \*.png or ab\*cd.ef?

  Find file matches by name and MD hash value, -inv shows non-matche
   lldup -file dir1 dir2/subdir dir3
   lldup -file -inv  dir1 dir2/subdir dir3
   lldup -file -div=\\n==\\n  -sep=', ' dir1 dir2/subdir dir3
   lldup -file -just -invert .
  Delete files from Dir2 which may include special characters:
   lldup -file -all Dir1 Dir2 | grep Dir2 | tr '\n' '\0' | xargs -t -0 -L1 -S512 -IX rm 'X'
  Keep first files in group and delete all others:
   lldup -file -all Dir1 Dir2 | awk '/--/ { num=0;} { if (num++ >1) print;}' | xargs -S512 -L1 -IX rm 'X'

</pre>
