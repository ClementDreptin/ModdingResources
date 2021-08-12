# Optional knowledge
The following resources will help you understand concepts that are not necessarily required but good to know if you wanna dive a little deeper.

## Game engine architectures
Despite their architecture differences, most game engines share some common concepts. The first ones that come to mind are probably:

- An event system
- A logging system
- A rendering abstraction
- An audio abstraction

This is not an exhaustive list by any means, just the first concepts that came to my mind.

If you want to study how game engines work, I recommend (once again) [The Cherno](https://www.youtube.com/TheChernoProject) and his [Game Engine series](https://www.youtube.com/playlist?list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT). In this series, The Cherno builds a game engine from scratch while explainning all the key concepts that go into making a game engine.

Another good resource is the [Quake III Arena source code](https://github.com/id-Software/Quake-III-Arena/). Quake is the game engine that major AAA franchises like Call of Duty, Halo or Counter Strike were inspired by to create their engine. As far as I know, the [IW engine](https://en.wikipedia.org/wiki/IW_(game_engine)) (engine the Call of Duty franchise is built upon) is just a modified version of Quake so a good part of the code (especially the low-level and utility stuff) is identical to the Quake engine.

## Graphics programming
Since you're mostly interacting with game engine while modding games, you rarely ever need to understand what's under the hood and how to manipulate graphics APIs. Understanding how graphics are rendered onto a screen is always useful though, especially if you want to directly interact with the rendering abstraction the engine provides or need a lower-level access to do very specific tasks.
A resource that I learned from the The Cherno's [OpenGL series](https://www.youtube.com/playlist?list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2). OpenGL is typically not the graphics API games use but it's the easiest one to understand and will help you get started with key concepts such as shaders, vertices, vertex buffers, buffer layouts, etc.
If you want to mod games for the Microsoft eco-system (Xbox or Windows), you should look into [DirectX](https://en.wikipedia.org/wiki/DirectX) (graphics API used on Microsoft systems). A good resource to learn DirectX from is [Jpres'](https://www.youtube.com/user/Jpres) [C++ DirectX 11 series](https://www.youtube.com/playlist?list=PLcacUGyBsOIBlGyQQWzp6D1Xn6ZENx9Y2). The series is focused on DirectX 11 which is the most recent version of DirectX that's not to low-level (like DirectX 12 is) so it's accessible for beginners.
