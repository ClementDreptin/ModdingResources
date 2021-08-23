# Rendering HUD
Getting feedback about what is going on in the killfeed is cool but kind of limited. In this section we are going to see 2 different ways of creating HUD elements.

## What are HUD elements?

>*In video gaming, the HUD (heads-up display) or status bar is the method by which information is visually relayed to the player as part of a game's user interface. It takes its name from the head-up displays used in modern aircraft.*

Source: [Wikipedia](https://en.wikipedia.org/wiki/HUD_(video_gaming))

On games like Call of Duty, the amount of ammo left in the weapon's magazine, the experience bar, the compass or the subtitles when you play the campaign mode are all part of the HUD. It's every piece of information displayed on the screen that's useful to the player without being part of the environment.

## HUD in Call of Duty
Call of Duty games have an HUD API (which is exposed to the GSC VM but we won't get into that), this API contains functions like `HudElem_Alloc` or `HudElem_Free` and structs such as `game_hudelem_s` or `hudelem_s`. This API allows you to easily create HUD elements without having to manually render them in an update loop. Another way to create HUD elements is to use the lower level rendering API from the engine that provides us with functions like `R_AddCmdDrawStretchPic` or `R_AddCmdDrawText`.

### HUD API (high level)
TODO

### Rendering API (low-level)
TODO
