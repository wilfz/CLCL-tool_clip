# CLCL-tool_clip

### What is this?
tool_clip is not a standalone program but an extension to Mr. Tomoaki's great [CLCL clipboard tool](https://nakka.com/soft/clcl/index_eng.html) (sourcecode: https://github.com/nakkag/CLCL).
Currently it contains the following features for clipboard items:
- export items and template folders to json file
- import text items and folders from json file and merge them into template folders
- export to and import from android app "Clipper"
- replace with regular expressions
- replace tabstops and/or sequences of spaces by a character string of choice
- convert tab separated data into an html table snippet, ready to insert into an email, OneNote, etc.
- send menu item to clipboard
- show currently selected item in viewer
- save CLCL templates to and load from an ODBC database 

To be continued ...

### Update history
- Ver 1.0.1.0 -> Ver 1.0.2.0
    - Features:
	    - new function "Show in Viewer"
	- Bugfix:
	    - show MessageBox on error in Replace with regular expression
- Ver 1.0.0.4 -> Ver 1.0.1.0
	- Features:
		- replace with regular expressions
		- export to and import from android app "Clipper"
	- Bugfix: 
		- export only TEXT and UNICODE_TEXT items to json file
		- in json-import replace empty title with (beginning of) text content
