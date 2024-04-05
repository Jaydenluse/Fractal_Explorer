## Installing Brew and SDL:

#### Run the following command to install Homebrew:
- `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`

#### In the Terminal, run the following command to install SDL2: 
- `brew install sdl2`

## Running the Code:
- Use `make` to compile the executable
- Execute the program with the following command:

      ./fractal_explorer <set_type> [num_colors]

#### - `<set_type>` is the type of fractal set to render, and can be one of the following:
- `julia`: Renders the Julia set.
- `mandelbrot`: Renders the Mandelbrot set.

#### - `[num_colors]` (optional) is the number of colors in the color palette. If not provided, the default value of 5 is used.

#### Controls:
- Use the arrow keys to move the viewpoint.
- Press `+` or `=` to zoom in.
- Press `-` to zoom out.
- For the Julia set, press `j` to switch to the Julia set rendering.
- For the Mandelbrot set, press `m` to switch to the Mandelbrot set rendering.

#### Additional Notes:
- The fractal rendering is parallelized using OpenMP for improved performance.
- The color palette is randomly generated for each run.
- The maximum number of iterations for the fractal calculation is set to 70 by default.
- The render scale determines the block size for rendering pixels and is set to 2 by default.
