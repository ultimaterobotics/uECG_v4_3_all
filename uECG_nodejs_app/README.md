## uECG PC monitor - browser version, based on node.js

This app at the moment basically duplicates functionality of Android app, but works with x8 higher data rate. In the near future we plan to add support of multiple devices and reduce amount of lost data, then focus on data analysis

## Usage

1. Install Node JS (with npm). The process is quite intuitive and there are plenty of tutorials on how to make it
2. Download the app (git clone, or simply download all files from this folder in a convenient place)
3. Go to the app folder via command line. For this:
  - on Linux and macOS: start the terminal, run series of `cd <folder_name>` commands to get to the folder with downloaded app (where "server.js" file is located)
 - on Windows: start cmd app for accessing command line, if the app folder is not on disk C - run `D:` if it's on D, `E:` if it's on E etc, then run series of `cd <folder_name>` commands to get to the folder with downloaded app (where "server.js" file is located)
4. Install two required packages by running commands:
```
npm install express
npm install serialport
```
5. On Windows and macOS, install drivers for cp2102 chips from here: https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers (not needed on Linux)
6. Start the server via command line interface: while in the app folder, run `node server.js`
7. In browser, enter address 127.0.0.1:8080 to access the UI. Note: if you already have some web development environment running on the port 8080, node server won't override it - in this case you can change the port number in the last line of server.js to any other value (and use it in browser correspondingly)
8. On uECG device, after it's turned on and finished blinking startup sequence, press button once to switch it into direct stream mode - after that, you should immediately see data in the browser interface. (pressing the button 2nd time would switch it in arduino-compatible streaming mode, also recognized by the base station, and pressing it for the 3rd time returns to BLE mode, the base won't receive this signal)
