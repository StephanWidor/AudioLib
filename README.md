# swAudioLib

A library providing some useful things for audio processing.
Currently this is only used in [pitchtool plugin](https://github.com/StephanWidor/pitchtool).

## Usage

Add this as a subdirectory to your cmake project, then add swAudioLib to target_link_libraries

Tested on Arch Linux using gcc15 and clang19. Other compilers might or might not work
(quite a lot usage of c++20 and c++23 features like ranges and concepts)
