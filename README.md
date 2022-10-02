# VCMP Lua Plugin

As the name suggests, this plugin provides a scripting API for VCMP in Lua (yay!). Please check out the [WIKI](https://github.com/DizzasTeR/VCMP-Lua/wiki) to learn
about the structure of the plugin (its quite easy!)

## Purpose of this project

The purpose of this project is mainly that I love Lua, and I also love C++, therefore to keep my practice in C++ I decided to work on this plugin with one of my
favorite scripting languages (Lua).

# The Lua config file

In your server directory you should place a **luaconfig.ini** file which will allow you to specify some settings the plugin can use. Some of these settings are
optional while some (like specifying atleast 1 script file) is compulsory.

The file structure is as of right now very simple:

```ini
[config]
# Sets experimental mode ON (1) or OFF (0) | Intended for beta-testing and development only. Do not rely for stability
#experimental=1
# Sets the log level, See the Logger page on Wiki for more information
loglevel=0
# Sets the log file. This log file will be used to create daily logs and it will log everything logged by Logger class, regardless of level
logfile=DailyLogs.logs

[modules]
# This is the modules section, here you can opt in to use external modules that the plugin provides. They can be listed and toggled by setting them to a boolean

#moduleName=[true/false]
lanes=false

[scripts]
# This is the scripts section, here you can specify all your script files that you want to run.

# script=path/to/file.lua
script=lua/script.lua
```

# Building the plugin

## Windows

To build on Windows, just download the repository and run the win-build.bat file in the premake folder. You should be getting a Visual Studio 2019 solution file

**NOTE: You can only build x64 of Lua plugin on Windows for now. Why? Cuz I'm too lazy to setup my environment to compile mariadb for x32 :D**

## Linux

To build on Linux:

-   Download/Clone the repository
-   Download premake and build it
-   Inside the repository, call premake: `path/to/premake5 gmake`
-   Now use `make` with your desired `config`: `make config=release` OR `make config=release32`
