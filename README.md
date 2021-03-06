# Brogue Seed Scummer

http://brogue.wikia.com/wiki/Seedscummer

The Brogue Seed Scummer is a modified version of Brogue created and maintained by forum user Patashu. It allows you to "scum" (search) for a seed with some combination of desired attributes. For example, you might want to find a seed with both Armour of Absorption and a Ring of Regeneration, or with an unusual number of Scrolls of Enchantment. Or maybe you're on a quest to find the dungeon with the most captive monkeys...

==Using the Seed Scummer==
A Windows executable exists at https://github.com/Patashu/Brogue-Seed-Scummer/releases/tag/0.5.1 and the source code can also be downloaded at https://github.com/Patashu/Brogue-Seed-Scummer . The scummer is a modified version of the Brogue executable and should be placed in your Brogue directory next to the normal executable. You cannot run the Seed Scummer without having downloaded normal Brogue; although it contains the game code, the download does not include the other resources such as the fonts folder and the DLLs that are included in the normal download.

The Seed Scummer can be run in two ways: using the graphical user interface (GUI) or through the Windows command line/Linux terminal. The command line version has more functionality and will generate a list of suitable seeds, whilst the GUI version will automatically start a game once a suitable seed has been found. The GUI version will ignore cursed items, whereas the command line version will include seeds with cursed items in the list.

In the GUI version, just select "Startscum" from the main menu and enter your search string in the box. In the command line version, you must pass the search string as an argument (changing "broguescum.exe" to whatever you called your executable). For example:

broguescum.exe --scum quietus > results.txt

will scum for seeds with weapons of quietus.  "> results.txt" tells the program to put the results in "results.txt" in the same directory as the executable. This is must be done in the windows version in order to see the results, as the program cannot currently output directly to the command line.

'''Note: '''Windows users may be in the habit of giving command line arguments to programs by creating a shortcut to the .exe and adding the arguments to the end of the file path. This does not seem to work with the scummer; you must use the command line.

==Phrasing Searches==
To enter multiple requirements, just seperate them with commas (",").
*You can search for items by entering their names. The scummer will look for anything with a name that partially matches the name given. For example, if you search for "'''regeneration'''", the scummer will look for any Ring of Regeneration. If you search for "'''+3 Ring of Regeneration'''", the scummer will only look for +3 Rings of Regeneration.
*You can search for a particular captive ally by entering its name. For example, searching for "monkey" will find seeds with a captive monkey.
*To search for a staff with a particular number of charges, phrase your search like this "'''Staff of Poison [3/3]'''". For wands, use: "'''Wand of Plenty [4]'''".
*You can specify that you only want seeds with a minimum number of [[Scroll of Enchantment|Scrolls of Enchantment]]. Any search term containing "'''enchan'''" is interpreted as a request for a certain number of scrolls. For example, "'''enchan 15'''" and "'''15 Scrolls of Enchantment'''" would both find seeds with at least 15 [[Scroll of Enchantment|Scrolls of Enchantment]].
*If you enter the name of an item twice, the scummer will require seeds to have two seperate items. This allows you to scum for two Rings of Wisdom, for example.

==Viewseed command==
The command line version has one other funtion. It can print a list of the items and captive monsters found on each dungeon level of a given seed, along with the total number of Scrolls of Enchantment. The argument:

"--viewseed 1"

would output a list of the contents of seed 1.

==Notes==

*The GUI version will ignore cursed items, whereas the command line version will include seeds with cursed items in the list. (The enchantment level of items is listed, so you can tell which seeds have cursed items).
*If you scum for a single item, the scummer will look for seeds with this item in the first 2 levels. If you scum for multiple items, they will be in the first 7 levels.
*If you request a minimum number of scrolls of enchantment, the scummer must search all 26 item-spawning levels of the dungeon. This will make searches considerably slower.
*The command line version can only be interrupted when running by killing the process with the Task Manager or equivalent. The GUI version allows you to abort the search by pressing "Q".
