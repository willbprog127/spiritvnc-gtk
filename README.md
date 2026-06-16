> [!IMPORTANT]
> This project is __was__ in read-only archive mode until 2026-06-16 due to unresolved gtk-vnc and msys2 bugs mentioned below. gtk-vnc maintainers took an extraordinary amount of time to reply to my bug report, but I can finally start working on this project again!

# spiritvnc-gtk
A GTK 3-based multi-connection VNC client for Unix-like and Windows systems 

This is a work in-progress so expect many changes and breakages.

This version of SpiritVNC is built using GTK 3 and the gtk-vnc widget.  Some features that the FLTK version has are not available here (gtk-vnc limitations and apparent bugs).  Don't ask me to release this on GTK 4 - GTK 4 is **pain**.  While this version seems to be relatively stable, be aware that these are early days so anything is possible...  

### Known issues ###
~~Clipboard ops are on-hold due to this bug: https://gitlab.gnome.org/GNOME/gtk-vnc/-/issues/35~~  There is also a gtk-vnc bug on Windows that prevents _any_ connection: https://github.com/msys2/MINGW-packages/issues/23948

![spiritvnc-gtk-screenshot](https://github.com/user-attachments/assets/b990beba-dfa5-4ee1-8f8f-9ba2bd535d87)
