# Walkman-MP3-Player
I frequently do revision to some background music that both blocks out the background noise but is also not too distracting which usually I use a Spotify playlist but lots of the time going onto this playlist or skipping a song leads to me scrolling on my phone and at that point I’ve lost focus and will probably end up on Instagram. This should solve this issue by basically being a brick that ONLY plays my revision music, it will also use wired headphones so that if my Bluetooth headphones are dead, I still have this music so again, I am less likely to procrastinate my revision. 

The created device is a reasonably nice looking rendition of the popular sony walkman but instead plays whatever MP3 files are loaded onto the SD card. <img width="1582" height="2109" alt="20260622_210455" src="https://github.com/user-attachments/assets/a14de485-1734-45b8-a0a6-76ffe5a06af8" />

To create the product, Print the files from the reposetory and source the components from the BOM. Then wire all these in acordance to the wiring diagram bellow and assemble. For easier assembly, insert the keyboard switches and potentiometer before soldering and then solder these first. <img width="1338" height="1784" alt="20260623_212732" src="https://github.com/user-attachments/assets/c00f516f-f337-4e05-a6ef-666d1bfeb975" />


To set up the product after making it. Flash the firmware to the arduino NANO and wire all components to their respective ports. Rename the music MP3 files to sequentually increasing numbers from 0001, 0002, etc and copy these onto the SD card. insert the SD card into the DFPlayerMini module and seal the case. Optionally you can illustrait the different side buttons with symbols. Then the device is ready, switch it on, plug in headphones and start listening. 



<img width="541" height="443" alt="image" src="https://github.com/user-attachments/assets/6294f8dc-30c5-457d-ba6b-e5a501b678ea" />


## BOM

#### (For full BOM with links see the folder)
Quantity,  Item Name,

1,  DFPlayerMini,

1,  Arduino NANO,

1,  "0.91"" OLED Display",

1,  Potentiometer Dial,

1,  Toggle switch,£1.72,

4,  Mechanical Keyboard switches,

1,  TP4056 module,

1,  Battery - 3.7V,
