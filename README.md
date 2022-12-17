# BMP
Bmp Redactor
Created by Lev Bunin

+**exit**
    Closing the terminal and exit from program.

+**help**
    Printing commands and flags.

+**ls [-flags ...]**
    Standard ls command with flag support.

+**cd [-flags ...]**
    Standard cd command with flag support.

+**mkdir [-flags ...]**
    Standard mkdir command with flag support.

+**rm [-flags ...]**
    Standard rm command with flag support.

+**open [/.../path_to.bmp]**
    Opening .bmp file for changing and/or writing.

+**write [/.../path_to_save.bmp]**
    Saving .bmp file.

+**change [options]**
    Changing file by using flags:

    +**"-negative" / "-n"**
        Negative filter.

    +**"-replace-color" / "-rc"**
        Replace RGB(1) color -> RGB(2).
        * Sends a request (stdin) about getting color parameters.

    +**"-clarity" / "-cl"**
        Clarity filter
        * Sends a request (stdin) about getting ~clarity force~ parameter

    +**"-gauss"**
        Gauss filter.

    +**"-grey" / "-g"**
        Grey filter.

    +**"-sobel" / "-s"**
        Border selection filter.

    +**"-median" / "-m"**
        Median filter.
        * Sends a request (stdin) about getting ~median area~ parameter.

    +**"-viniette" / "-v"**
        Viniette filter.
        * Sends a request (stdin) about getting ~radius & power~ parameters

    +**"-frame" / "-f"**
        Framing .bmp
        * Sends a request (stdin) about getting ~x0, y0, w, h~ parameters

    +**"-resize" / "-rs"**
        Resizing picture and changing width, height
        * Sends a request (stdin) about getting ~new_w, new_h~ parameters
