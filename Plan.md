# The plan!

## Objectives:

0. create separate descriptor sets for:

   - diferent allocations for dinamic and static (so diferent pools must be created)

   - samplers (it is static)
   - material (we need one per material )
   - camera (we need one per frame since it can change from one frame to another)

1. strip 3d stuf from code (create a branch)

2. Have a renderer object that renders points to the screen
   a. strip utility (pure) functions (not create pipeline since we will need various versions of it...)
   b. Function add point

3. Have renderer object render 3d points

## Notes

### Realtime

- when geo is modified only command buffer is needed to change, the pipeline remains.
- pipeline changes (needed to rebuild) when we change de rendering proces (shaders, passes...)
- swapchin needs to be rebuild when window/render target changes (size, aspect...)
