# rlbotc-example
Example project for creating a custom Rocket League bot in C using the [RLBot framework](https://github.com/RLBot/core).

This example is split into two parts, the bot application and the match runner application:
- The bot application implements the bots' logic. The RLBot server sends the dynamic state of the Rocket League match, to which the bot replies with a new set of controller inputs.
- The match runner configures the RLBot server to start a Rocket League match. This is provided as a simple example of starting a match, which could be usefull when setting up an automated test environement for a bot. For a more streamlined experience see the [RLBot GUI](https://github.com/RLBot/gui), which is a graphical user interface to easily configure and run Rocket League bot games.

Notes:
- This bot is written for version 5 of the RLBot framework. Which, at the time of writing this documentation, has not yet officially replaced version 4. Version 5 is fully functional, but the API might still undergo breaking changes.
- Windows is currently not supported by this example bot. This should in theory only require a Windows specific implementation of the platform abstraction layer (See https://github.com/kipje13/rlbotc-example/blob/main/platform_posix.c).

## Building
As this example is CMake based, it requires CMake to be installed on your system.
Run the following commands to clone and build the project:
```
git clone https://github.com/kipje13/rlbotc-example.git
cd rlbotc-example
mkdir build
cd build
cmake ..
make
```
Then run the match_runner application to start the Rocket League match with the C example bot.

## Steam or Epic Games Launcher
The match runner configures the RLBot framework to launch Rocket League from the Steam client. This will not work if Rocket League is installed from the Epic Games Launcher. 
To fix this, change the option `rlbot_flat_Launcher_Steam` to `rlbot_flat_Launcher_Epic` in https://github.com/kipje13/rlbotc-example/blob/main/match_runner.c
