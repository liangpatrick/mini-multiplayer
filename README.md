# mini-multiplayer
The goal of this project was to convert a simple single player game client using SDL libraries to a multiplayer client that communicates with a custom server in real time.

Feature Additions:
- Added scoreboard for current players
- Added text to display top scorer of current session
- If a player leaves a new player may take their position
- Support for up to 8 concurrent players
- Entire server side communication

## Requirements
Before beginning the system running the single player client must be linux based due to SDL rendering dependencies.

To run the client, you need to set the path for finding dynamic libraries. Assuming you’re
using bash, you can prepend the /lib directory to your LD_LIBRARY_PATH environment
variable with this command (modifying the exact path as needed):
export LD_LIBRARY_PATH="$HOME/lib:$LD_LIBRARY_PATH

You cab also install the libraries locally using:
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
Then, build with make

The server can run on any system supporting the gnu17 standard.

### Setup
Build the project using the included make file.
Start the server using './server < port >'
- '< port >' specifies the port the clients will connect to.

Start the client using './client < server name/IP > < port >'
- '< server name/IP >' specifies the address of the server, supports direct IPs and texts addresses including local host
- '< port >' specifies the port the server is hosted on, which was decided when starting the server

Please follow both [Rutgers University's Principles of Academic Integrity](http://academicintegrity.rutgers.edu/) and the [Rutgers Department of Computer Science's Academic Integrity Policy](https://www.cs.rutgers.edu/academics/undergraduate/academic-integrity-policy)


