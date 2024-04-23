#include <iostream>
#include <vector>
#include <complex>
#include <cstdint>
#include <random>
#include </opt/homebrew/include/SDL2/SDL.h>
#include <future>
#include <thread>
#include <vector>

//See README for more information

const int WIDTH = 1600;
const int HEIGHT = 1000;
const int MAX_ITERATIONS = 50; //This can be set higher or lower (at the cost of your CPU) for more iterations of the fractal
const double RENDER_SCALE = 1; //This can also be set to a higher number for more performant CPU, but a lack of resolution

using Complex = std::complex<double>;

int calculateResolution(double scale) {
    return static_cast<int>(WIDTH * scale);
}

SDL_Rect calculateVisibleRegion(double centerX, double centerY, double scale, int width, int height) {
    int left = std::max(0, static_cast<int>((centerX - scale * width / 2) / scale));
    int top = std::max(0, static_cast<int>((centerY - scale * height / 2) / scale));
    int visibleWidth = std::min(static_cast<int>(width / scale), width - left);
    int visibleHeight = std::min(static_cast<int>(height / scale), height - top);

    return { left, top, visibleWidth, visibleHeight };
}

std::vector<uint32_t> generateRandomColorPalette(int numColors) {
    std::vector<uint32_t> palette(numColors);
    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFF);

    for (int i = 0; i < numColors; ++i) {
        uint32_t color = dis(eng);
        palette[i] = (0xFF << 24) | color;
    }

    return palette;
}

uint32_t getColor(int iterations, const std::vector<uint32_t>& colorPalette) {
    if (iterations == MAX_ITERATIONS) {
        return 0;
    }
    double t = static_cast<double>(iterations) / MAX_ITERATIONS;
    int colorIndex = static_cast<int>(t * (colorPalette.size() - 1));
    uint32_t color1 = colorPalette[colorIndex];
    uint32_t color2 = colorPalette[colorIndex + 1];
    double fraction = t * (colorPalette.size() - 1) - colorIndex;
    uint8_t r = static_cast<uint8_t>((1 - fraction) * ((color1 >> 16) & 0xFF) + fraction * ((color2 >> 16) & 0xFF));
    uint8_t g = static_cast<uint8_t>((1 - fraction) * ((color1 >> 8) & 0xFF) + fraction * ((color2 >> 8) & 0xFF));
    uint8_t b = static_cast<uint8_t>((1 - fraction) * (color1 & 0xFF) + fraction * (color2 & 0xFF));
    return (255 << 24) | (r << 16) | (g << 8) | b;
}

// void julia(std::vector<uint32_t>& pixels, int width, int height, int startX, int startY, int endX, int endY, double centerX, double centerY, double scale, int resolution, const std::vector<uint32_t>& colorPalette) {
//     Complex c(-0.8, 0.156);  // Julia set parameter

//     #pragma omp parallel for
//     for (int y = startY; y < endY; y += RENDER_SCALE) {
//         for (int x = startX; x < endX; x += RENDER_SCALE) {
//             if (x >= 0 && x < width && y >= 0 && y < height) {
//                 double zx = ((double)x - resolution / 2) * scale + centerX;
//                 double zy = ((double)y - resolution / 2) * scale + centerY;
//                 Complex z(zx, zy);
//                 int iterations = 0;
//                 double potential = 0.0;

//                 while (std::abs(z) < 10 && iterations < MAX_ITERATIONS) {
//                     z = z * z + c;
//                     potential = std::log(std::log(std::abs(z))) / std::log(2.0);
//                     ++iterations;
//                 }

//                 uint32_t color;
//                 if (iterations == MAX_ITERATIONS) {
//                     color = getColor(static_cast<int>(potential * MAX_ITERATIONS), colorPalette);  // Black color for points inside the set
//                 } else {
//                     color = getColor(static_cast<int>(potential * MAX_ITERATIONS), colorPalette);
//                 }

//                 // Fill the scaled pixel block
//                 for (int i = 0; i < RENDER_SCALE; ++i) {
//                     for (int j = 0; j < RENDER_SCALE; ++j) {
//                         int pixelX = x + j;
//                         int pixelY = y + i;
//                         if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
//                             pixels[pixelY * width + pixelX] = color;
//                         }
//                     }
//                 }
//             }
//         }
//     }
// }

// void mandelbrot(std::vector<uint32_t>& pixels, int width, int height, int startX, int startY, int endX, int endY, double centerX, double centerY, double scale, const std::vector<uint32_t>& colorPalette) {
//     int resolution = calculateResolution(scale);

//     #pragma omp parallel for
//     for (int y = startY; y < endY; y += RENDER_SCALE) {
//         for (int x = startX; x < endX; x += RENDER_SCALE) {
//             if (x >= 0 && x < width && y >= 0 && y < height) {
//                 double zx = ((double)x - resolution / 2) * scale + centerX;
//                 double zy = ((double)y - resolution / 2) * scale + centerY;
//                 Complex c(zx, zy);
//                 Complex z(0, 0);
//                 int iterations = 0;
//                 while (std::abs(z) < 8 && iterations < MAX_ITERATIONS) {
//                     z = z * z + c;
//                     ++iterations;
//                 }

//                 uint32_t color = getColor(iterations, colorPalette);

//                 // Fill the scaled pixel block
//                 for (int i = 0; i < RENDER_SCALE; ++i) {
//                     for (int j = 0; j < RENDER_SCALE; ++j) {
//                         int pixelX = x + j;
//                         int pixelY = y + i;
//                         if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
//                             pixels[pixelY * width + pixelX] = color;
//                         }
//                     }
//                 }
//             }
//         }
//     }
// }

void mandelbrot(std::vector<uint32_t>& pixels, int width, int height, int startX, int startY, int endX, int endY, double centerX, double centerY, double scale, const std::vector<uint32_t>& colorPalette) {
    int resolution = calculateResolution(scale);

    std::vector<std::future<void>> futures;
    int chunkSize = 64;
    int numChunksX = (endX - startX + chunkSize - 1) / chunkSize;
    int numChunksY = (endY - startY + chunkSize - 1) / chunkSize;

    for (int chunkY = 0; chunkY < numChunksY; ++chunkY) {
        for (int chunkX = 0; chunkX < numChunksX; ++chunkX) {
            int chunkStartX = startX + chunkX * chunkSize;
            int chunkStartY = startY + chunkY * chunkSize;
            int chunkEndX = std::min(chunkStartX + chunkSize, endX);
            int chunkEndY = std::min(chunkStartY + chunkSize, endY);

            futures.emplace_back(std::async(std::launch::async, [&, chunkStartX, chunkStartY, chunkEndX, chunkEndY]() {
                for (int y = chunkStartY; y < chunkEndY; y += RENDER_SCALE) {
                    for (int x = chunkStartX; x < chunkEndX; x += RENDER_SCALE) {
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            double zx = ((double)x - resolution / 2) * scale + centerX;
                            double zy = ((double)y - resolution / 2) * scale + centerY;
                            Complex c(zx, zy);
                            Complex z(0, 0);
                            int iterations = 0;
                            while (std::abs(z) < 8 && iterations < MAX_ITERATIONS) {
                                z = z * z + c;
                                ++iterations;
                            }

                            uint32_t color = getColor(iterations, colorPalette);

                            // Fill the scaled pixel block
                            for (int i = 0; i < RENDER_SCALE; ++i) {
                                for (int j = 0; j < RENDER_SCALE; ++j) {
                                    int pixelX = x + j;
                                    int pixelY = y + i;
                                    if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                                        pixels[pixelY * width + pixelX] = color;
                                    }
                                }
                            }
                        }
                    }
                }
            }));
        }
    }

    for (auto&& future : futures) {
        future.get();
    }
}

void julia(std::vector<uint32_t>& pixels, int width, int height, int startX, int startY, int endX, int endY, double centerX, double centerY, double scale, int resolution, const std::vector<uint32_t>& colorPalette) {
    Complex c(-0.8, 0.156);  // Julia set parameter

    std::vector<std::future<void>> futures;
    int chunkSize = 64;
    int numChunksX = (endX - startX + chunkSize - 1) / chunkSize;
    int numChunksY = (endY - startY + chunkSize - 1) / chunkSize;

    for (int chunkY = 0; chunkY < numChunksY; ++chunkY) {
        for (int chunkX = 0; chunkX < numChunksX; ++chunkX) {
            int chunkStartX = startX + chunkX * chunkSize;
            int chunkStartY = startY + chunkY * chunkSize;
            int chunkEndX = std::min(chunkStartX + chunkSize, endX);
            int chunkEndY = std::min(chunkStartY + chunkSize, endY);

            futures.emplace_back(std::async(std::launch::async, [&, chunkStartX, chunkStartY, chunkEndX, chunkEndY]() {
                for (int y = chunkStartY; y < chunkEndY; y += RENDER_SCALE) {
                    for (int x = chunkStartX; x < chunkEndX; x += RENDER_SCALE) {
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            double zx = ((double)x - resolution / 2) * scale + centerX;
                            double zy = ((double)y - resolution / 2) * scale + centerY;
                            Complex z(zx, zy);
                            int iterations = 0;
                            while (std::abs(z) < 10 && iterations < MAX_ITERATIONS) {
                                z = z * z + c;
                                ++iterations;
                            }

                            uint32_t color = getColor(iterations, colorPalette);

                            // Fill the scaled pixel block
                            for (int i = 0; i < RENDER_SCALE; ++i) {
                                for (int j = 0; j < RENDER_SCALE; ++j) {
                                    int pixelX = x + j;
                                    int pixelY = y + i;
                                    if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                                        pixels[pixelY * width + pixelX] = color;
                                    }
                                }
                            }
                        }
                    }
                }
            }));
        }
    }

    for (auto&& future : futures) {
        future.get();
    }
}

void burningShip(std::vector<uint32_t>& pixels, int width, int height, int startX, int startY, int endX, int endY, double centerX, double centerY, double scale, const std::vector<uint32_t>& colorPalette) {
    int resolution = calculateResolution(scale);

    std::vector<std::future<void>> futures;
    int chunkSize = 64;
    int numChunksX = (endX - startX + chunkSize - 1) / chunkSize;
    int numChunksY = (endY - startY + chunkSize - 1) / chunkSize;

    for (int chunkY = 0; chunkY < numChunksY; ++chunkY) {
        for (int chunkX = 0; chunkX < numChunksX; ++chunkX) {
            int chunkStartX = startX + chunkX * chunkSize;
            int chunkStartY = startY + chunkY * chunkSize;
            int chunkEndX = std::min(chunkStartX + chunkSize, endX);
            int chunkEndY = std::min(chunkStartY + chunkSize, endY);

            futures.emplace_back(std::async(std::launch::async, [&, chunkStartX, chunkStartY, chunkEndX, chunkEndY]() {
                for (int y = chunkStartY; y < chunkEndY; y += RENDER_SCALE) {
                    for (int x = chunkStartX; x < chunkEndX; x += RENDER_SCALE) {
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            double zx = ((double)x - resolution / 2) * scale + centerX;
                            double zy = ((double)y - resolution / 2) * scale + centerY;
                            Complex c(zx, zy);
                            Complex z(0, 0);
                            int iterations = 0;
                            while (std::abs(z) < 8 && iterations < MAX_ITERATIONS) {
                                double real = std::abs(z.real());
                                double imag = std::abs(z.imag());
                                z = Complex(real * real - imag * imag + c.real(), 2 * real * imag + c.imag());
                                ++iterations;
                            }

                            uint32_t color = getColor(iterations, colorPalette);

                            // Fill the scaled pixel block
                            for (int i = 0; i < RENDER_SCALE; ++i) {
                                for (int j = 0; j < RENDER_SCALE; ++j) {
                                    int pixelX = x + j;
                                    int pixelY = y + i;
                                    if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                                        pixels[pixelY * width + pixelX] = color;
                                    }
                                }
                            }
                        }
                    }
                }
            }));
        }
    }

    for (auto&& future : futures) {
        future.get();
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <set_type> [num_colors]" << std::endl;
        std::cerr << "  set_type: 'julia', 'mandelbrot', or 'burningship'" << std::endl;
        std::cerr << "  num_colors: number of colors in the palette (default: 5)" << std::endl;
        return 1;
    }   

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    window = SDL_CreateWindow("Mandelbrot Explorer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);
    if (texture == nullptr) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event event;

    double centerX = 0.0;
    double centerY = 0.0;
    double scale = 0.001;

    std::string setType(argv[1]);
    bool isJulia = (setType == "julia");
    bool isBurningShip = (setType == "burningship");

    int numColors = 5;
    if (argc == 3) {
        numColors = std::stoi(argv[2]);
    }

    const std::vector<uint32_t> colorPalette = generateRandomColorPalette(numColors);

    std::vector<uint32_t> pixelBuffer(WIDTH * HEIGHT);

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        centerY -= 20 * scale;
                        break;
                    case SDLK_DOWN:
                        centerY += 20 * scale;
                        break;
                    case SDLK_LEFT:
                        centerX -= 20 * scale;
                        break;
                    case SDLK_RIGHT:
                        centerX += 20 * scale;
                        break;
                    case SDLK_EQUALS:
                        scale *= 0.9;
                        break;
                    case SDLK_MINUS:
                        scale /= 0.9;
                        break;
                    case SDLK_j:
                        isJulia = true;
                        isBurningShip = false;
                        break;
                    case SDLK_m:
                        isJulia = false;
                        isBurningShip = false;
                        break;
                    case SDLK_b:
                        isJulia = false;
                        isBurningShip = true;
                        break;
                    default:
                        break;
                }
            }
        }

        int resolution = calculateResolution(scale);
        SDL_Rect visibleRegion = calculateVisibleRegion(centerX, centerY, scale, WIDTH, HEIGHT);

        int chunkSize = 64;
        int numChunksX = (visibleRegion.w + chunkSize - 1) / chunkSize;
        int numChunksY = (visibleRegion.h + chunkSize - 1) / chunkSize;

        #pragma omp parallel for
        for (int chunkY = 0; chunkY < numChunksY; ++chunkY) {
            for (int chunkX = 0; chunkX < numChunksX; ++chunkX) {
                int startX = std::max(0, visibleRegion.x + chunkX * chunkSize);
                int startY = std::max(0, visibleRegion.y + chunkY * chunkSize);
                int endX = std::min(startX + chunkSize, visibleRegion.x + visibleRegion.w);
                int endY = std::min(startY + chunkSize, visibleRegion.y + visibleRegion.h);

                if (isJulia) {
                    julia(pixelBuffer, WIDTH, HEIGHT, startX, startY, endX, endY, centerX, centerY, scale, resolution, colorPalette);
                } else if (isBurningShip) {
                    burningShip(pixelBuffer, WIDTH, HEIGHT, startX, startY, endX, endY, centerX, centerY, scale, colorPalette);
                } else {
                    mandelbrot(pixelBuffer, WIDTH, HEIGHT, startX, startY, endX, endY, centerX, centerY, scale, colorPalette);
                }

                SDL_Rect chunkRect = { startX, startY, endX - startX, endY - startY };
                SDL_UpdateTexture(texture, &chunkRect, &pixelBuffer[startY * WIDTH + startX], WIDTH * sizeof(uint32_t));
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}