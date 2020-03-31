# indexiine-installer
Installer for Indexiine

You can find more Information about Indexiine in the [GBAtemp thread](https://gbatemp.net/threads/indexiine-load-cfw-during-boot-and-offline-without-a-vc-ds-title.553681/)!

## Usage

You don't need to run CFW. Indexiine Installer will do an IOSU Exploit when there is no CFW.  

* Extract the `.zip` from the [releases page](https://github.com/GaryOderNichts/indexiine-installer/releases) to your sd card  
* Run Indexiine Installer
* Press A to backup current index.html and install Indexiine, Press B to restore

The backup is written to `sd:/wiiu/apps/indexiine-installer/backup-index.html`

## Credits

Jonhyjp  - Indexiine  
FIX94    - IOSU Exploit and wupserver copied from wuphax  
dimok789 - libiosuhax  

## Build

Run `make` in arm_user, arm_kernel and wupserver and copy each `*bin.h` to /payload  
Run `make`  

## Disclaimer

I am not responsible for any bricks or other damage done to your device!
