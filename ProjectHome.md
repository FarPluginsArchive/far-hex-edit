**HexEditor plugin for [FAR File Manager](http://www.farmanager.com)**<br>
Version 1.2<br>
Copyright (C) 1999 Alex Pakhotin<br>
<br>
<b>1. Foreword</b>

Yes! It's a long-awaited hexadecimal editor for FAR! Now you can edit data in binary mode.<br>
<br>
<b>2. Usage</b>

Execute HexEditor using standard plugin menu or by macro key.<br>
Use standard cursor movement keys to move;<br>
Tab - to toggle hexadecimal and char modes;<br>
F4 - to return to the text editor with data saving;<br>
F5 - to go to specified address;<br>
F7 - to search text or data;<br>
Shift-F7 - to continue search;<br>
F10,Esc - to close HexEditor with confirmation.<br>
<br>
<b>3. Technical details</b>

Data in the FAR text editor is stored as separate lines. So for binary editing it has to be converted into continuous block, and after editing - back into the lines. It's quite slow operation leading to delays when the lines number is big enough. If the data hasn't been changed in HexEditor the text in the editor isn't changed too. So don't worry when you use HexEditor simply to view editor data in the hexadecimal mode - it's non-destructive operation. Program is written in C++ and compiled with Microsoft Visual C++ 5.<br>
<br>
<b>4. Disclaimer of damages</b>

This program works with data on low level, so it may be cases of data damage.<br>
<br>
THE AUTHOR IS NOT LIABLE TO YOU FOR ANY SPECIAL, CONSEQUENTIAL, INDIRECT OR SIMILAR DAMAGES, INCLUDING ANY LOST PROFITS OR LOST DATA ARISING OUT OF THE USE OR INABILITY TO USE THE SOFTWARE.<br>
<br>
If you found an error please report to the author by e-mail immediately.<br>
<br>
<b>5. Thanks</b>

Thanks a lot to Eugene Roshal, the author of FAR, for his really GREAT program and also for his help in writing this plugin.<br>
<br>
<br>
Alex Pakhotin, Omsk, Russia<br>
2:5004/16.99@FidoNet<br>