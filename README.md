# A Chart Recorder Capable of Using Braille Characters


:; cat input | awk '{if($3 > 55) tailer="R3C"; else tailer="..."; printf("^%03d %d%s %d%s $%s *300\n",NR,$1*3,":A:10",$3*3,":B:15", tailer);fflush();}' | ./chart_recorder -l

    004|                              ⠑⠒⠒⠢⠤⠤⢄⣀⣀⣀⣀                                  ⡇                                                                           |...
    008|                                         ⠉⠉⠉⠒⠒⠒⠤⠤⠤⣀⣀⣀                      ⠑⢲                                                                          |...
    012|                                                     ⠉⠑⠒⠒⠢⢤  A              ⠘⠤⡄                                                                        |...
    016|                                                          ⢸                   ⢇⣀  B                                                                    |...
    020|                                                 ⢀⣀⣀⡠⠤⠒⠒⠒⠉⠁                    ⢸                                                                       |...
    024|                                    ⣀⣀⣀⣀⡠⠤⠤⠔⠒⠒⠊⠉⠉⠁     A                        ⠉⡇                                                                     |...
    028|                      ⢀⣀⣀⡠⠤⠤⠔⠒⠒⠒⠒⠉⠉⠉                                             ⠑⢲                                                                    |...
    032|         ⣀⣀⣀⠤⠤⠤⠒⠒⠒⠉⠉⠉⠉⠁                                                           ⠘⠤⡄ B                                                                |R3C
    036|⣀⡠⠤⠤⠔⠒⠉⠉⠉     A                                                                     ⢇⣀                                                                 |R3C
    040|⡇                                                                                    ⢸                                                                 |R3C
    044|⠁⠐⠤⠤⠤⣀⡀                                                                               ⠉⡇                                                               |R3C
    048|      ⠈⠉⠉⠑⠒⠒⠢⠤⠤⢄⣀⣀⡀ A                                                                  ⠑⢲  B                                                           |R3C



# Background

I spend most of my time at work logged into remote machines over SSH. I've
found TMUX to be a useful companion as it's fairly common to see the
connections drop.  I've used XTerm for as long as I've used X and,
to be honest, I'm very happy with that.

Another feature of my job is that there are streaming data sources which
I want to look at. Sometimes we have data pouring out of a system over
UDP but it's never hard to turn that into a stream of numbers - often a
timestamp in the first column and decimal values in one or more columns
to the right, space separated. Think of those lines just streaming
from stdout.

Graphs are good when you want to understand what's going on.

Sometimes I'll snip a chunk of that columnated data into a file, launch
Octave and plot the results. More often than not, I won't have X over SSH
so I'll get a nice ASCII plot - which is certainly better than nothing.

On other occasions, I've used AWK to draw out the data as ASCII `*`
characters - offset by the data value - an allowed those to spool up
the screen as the readings come in. That format vaguely represents a
low-res old fashioned chart recorder.  It's easy and surprisingly good
for a quick look.

XTerm - and many of its modern counterparts support Braille characters. 
Every character cell on the screen can contain 8 pixels - four high and 
two wide. So I can immediately increase my chart recorder resolution and 
remain in the comfortable XTerm environment.

I've written a program to do just that. Counter to the spirit of proper 
UNIX applications, I've made it do two things badly instead of one thing 
well: the program can output the low-res `*` format, too.
