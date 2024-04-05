#include <iostream>
#include <vector>
#include <complex>
#include <cstdint>
#include <random>
#include </opt/homebrew/include/SDL2/SDL.h>

const int WIDTH = 1200;
const int HEIGHT = 1000;
const int MAX_ITERATIONS = 50;

using Complex = std::complex<double>;

int calculateResolution(double scale) {
    // Adjust the resolution based on the zoom level (scale)
    // You can customize this formula based on your requirements
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

void continuousPotential(char* pixels, int width, int height, double minX, double maxX, double minY, double maxY, double scale, const std::vector<uint32_t>& colorPalette, bool isJulia, const Complex& c) {
    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double zx = minX + (maxX - minX) * x / width;
            double zy = minY + (maxY - minY) * y / height;
            Complex z(zx, zy);
            int iterations = 0;
            double potential = 0.0;
            while (std::abs(z) < 2 && iterations < MAX_ITERATIONS) {
                if (isJulia) {
                    z = z * z + c;
                } else {
                    potential = std::log(std::log(std::abs(z))) / std::log(2.0);
                    z = z * z + Complex(zx, zy);
                }
                ++iterations;
            }
            if (iterations == MAX_ITERATIONS) {
                uint32_t color = 0; // Set to black
                memcpy(pixels + (y * width + x) * sizeof(uint32_t), &color, sizeof(uint32_t));
            } else {
                uint32_t color = getColor(static_cast<int>(potential * MAX_ITERATIONS), colorPalette);
                memcpy(pixels + (y * width + x) * sizeof(uint32_t), &color, sizeof(uint32_t));
            }
        }
    }
}

void julia(std::vector<uint32_t>& pixels, int width, int height, int startX, int startY, int endX, int endY, double centerX, double centerY, double scale, int resolution, const std::vector<uint32_t>& colorPalette) {
    Complex c(-0.8, 0.156);  // Julia set parameter

    #pragma omp parallel for
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                Complex z(((double)x - resolution / 2) * scale + centerX, ((double)y - resolution / 2) * scale + centerY);
                int iterations = 0;
                while (std::abs(z) < 2 && iterations < MAX_ITERATIONS) {
                    z = z * z + c;
                    ++iterations;
                }
                uint32_t color = getColor(iterations, colorPalette);
                pixels[y * width + x] = color;
            }
        }
    }
}

void mandelbrot(char* pixels, int width, int height, double centerX, double centerY, double scale, const std::vector<uint32_t>& colorPalette) {
    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Complex c((x - width / 2) * scale + centerX, (y - height / 2) * scale + centerY);
            Complex z(0, 0);
            int iterations = 0;
            while (std::abs(z) < 2 && iterations < MAX_ITERATIONS) {
                z = z * z + c;
                ++iterations;
            }
            uint32_t color = getColor(iterations, colorPalette);
            memcpy(pixels + (y * width + x) * sizeof(uint32_t), &color, sizeof(uint32_t));
        }
    }
}

int main() {
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

    double centerX = -0.5;
    double centerY = 0.0;
    double scale = 0.005;

    const std::vector<uint32_t> colorPalette = generateRandomColorPalette(8);

    std::vector<char> pixels(WIDTH * HEIGHT);

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
                    default:
                        break;
                }
            }
        }

        int resolution = calculateResolution(scale); // Adjust resolution based on zoom level
    SDL_Rect visibleRegion = calculateVisibleRegion(centerX, centerY, scale, WIDTH, HEIGHT);

    int chunkSize = 64; // Adjust chunk size as needed
    int numChunksX = (visibleRegion.w + chunkSize - 1) / chunkSize;
    int numChunksY = (visibleRegion.h + chunkSize - 1) / chunkSize;

    std::vector<uint32_t> pixelBuffer(WIDTH * HEIGHT);

    #pragma omp parallel for
    for (int chunkY = 0; chunkY < numChunksY; ++chunkY) {
        for (int chunkX = 0; chunkX < numChunksX; ++chunkX) {
            int startX = std::max(0, visibleRegion.x + chunkX * chunkSize);
            int startY = std::max(0, visibleRegion.y + chunkY * chunkSize);
            int endX = std::min(startX + chunkSize, visibleRegion.x + visibleRegion.w);
            int endY = std::min(startY + chunkSize, visibleRegion.y + visibleRegion.h);

            julia(pixelBuffer, WIDTH, HEIGHT, startX, startY, endX, endY, centerX, centerY, scale, resolution, colorPalette);

            SDL_Rect chunkRect = { startX, startY, endX - startX, endY - startY };
            SDL_UpdateTexture(texture, &chunkRect, &pixelBuffer[startY * WIDTH + startX], WIDTH * sizeof(uint32_t));
        }
    }

    // julia(reinterpret_cast<char*>(pixels.data()), WIDTH, HEIGHT, centerX, centerY, scale, colorPalette);
    // or
    // mandelbrot(reinterpret_cast<char*>(pixels.data()), WIDTH, HEIGHT, centerX, centerY, scale, colorPalette);

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