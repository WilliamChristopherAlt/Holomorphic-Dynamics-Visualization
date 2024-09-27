Here's the list of all the available inputs:
  - Hold and drag your left mouse to move around the plane.
  - Hold and drag your right mouse to move the currently selected point (just a complex number that can be used to calculate the next iteration, such as the c value in the Julia Set).
  - Scroll you mouse zoom in or out.
    
  - Press ESC to escape the program
  - Press W to increase the iteration count, press it with SHIFT to do the reverse.
  - Press Z to increase the gradient sensitivity, press it with SHIFT to do the reverse.
  - Press R to reset the currently selected point to 0.
  - Press X to flip the horizontal axis.
  - Press Y to flip the vertical axis.
  - Press P to switch from and to parameter space(e.g. Mandelbrot vs Julia).
  - Press S to take a screenshot.
  - Press Ctrl + a number to change the selected point.

The program holds an array of selected points, you may change the size of this array in the source code. The points in the array may be used in the shader to calculate the next iteration.
In Shaders/compute.glsl, find the main function, in there there will be some predefined holormophic functions.
You can write your own function in the format of z = (a function of z, z0, and one or many of the selected points, where z is the current iteration and z0 is the initial value).
