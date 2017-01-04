# Attractiveness_Info_I
Software for the evaluation of attractiveness data needed for Informatics I at Reutlingen University.

Runs on Windows, OS X and Linux
# How to:
If the program is run, the user is prompted with a request to enter the path to the source data. After that a matrikel number can be entered. Both of these options can also be set through commandline arguments.<br/>
-p [sourcePath] sets the path to the folder with the source data.<br/>
-o [outputPath] configures a different output location.<br/>
-g turns the gui on (pictures and graphs will be shown, not just saved)<br/>
-d switches on downloadAll. A complete set of all pictures sorted by score will be downloaded.

# Dependencies:
- gnuplot (optional but recommended)
- bitsadmin (Windows only. most systems should still have ist even though it's discontinued)

# Return values:
- 0: All went well
- -1: Unknown error
- 100: Error when allocating memory
- 101: Error when opening file
- 102: Error when opening Pipe
- 103: No annotation for matrikel number
- 104: Unknown operating system
- 105: Command not found
- 106: Unable to get PATH
