# The plan!

## Objectives:

0. strip 3d stuf from code (create a branch)
1. Have a renderer object that renders points to the screen
   a. strip utility (pure) functions (not create pipeline since we will need various versions of it...)
   b. Function add point

2. Have renderer object render 3d points

## Notes

### Realtime

- when geo is modified only command buffer is needed to change, the pipeline remains.
- pipeline changes (needed to rebuild) when we change de rendering proces (shaders, passes...)
- swapchin needs to be rebuild when window/render target changes (size, aspect...)
