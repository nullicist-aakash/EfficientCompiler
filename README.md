# Overview

This is a compile time LL(1) parser in C++. I worked on this project after learning `compile time programming` in C++ (using `constexpr`), `concepts` and `modules` (which were only supported in MSVC at the time of project).

I worked on this project to get hands-on experience in using latest language features.

`LL(1)` parser seemed like a perfect project to see the capabilities of `Compile Time Programming`. I tried to implement a JSON parser using this, and I am using it in my separate project (not complete yet).

I tried to not to get involved with `UB`, and I believe I succeed in that target. 

On second look, and after learning more about `constexpr` and `templates`, I realized that the code can be simplified a lot. If I get sufficient time to refactor this whole project, I will definitely try to do it.
