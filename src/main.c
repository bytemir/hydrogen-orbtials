#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SDL_Texture* texture = NULL;
uint32_t* pixels = NULL;

double compute_legendre(int l, int m, double x) {
    if (m < 0) m = -m; 

    double pmm = 1.0;
    if (m > 0) {
        double somx2 = sqrt((1.0 - x) * (1.0 + x));
        double fact = 1.0;
        for (int i = 1; i <= m; i++) {
            pmm *= -fact * somx2;
            fact += 2.0;
        }
    }
    if (l == m) return pmm;

    double pmmp1 = x * (2 * m + 1) * pmm;
    if (l == m + 1) return pmmp1;

    double pll = 0.0;
    for (int ll = m + 2; ll <= l; ll++) {
        pll = (x * (2 * ll - 1) * pmmp1 - (ll + m - 1) * pmm) / (ll - m);
        pmm = pmmp1;
        pmmp1 = pll;
    }
    return pll;
}

double compute_laguerre(int k, int alpha, double x) {
    if (k == 0) return 1.0;
    if (k == 1) return 1.0 + alpha - x;
    
    double L0 = 1.0;
    double L1 = 1.0 + alpha - x;
    double L2 = 0.0;
    
    for (int i = 2; i <= k; i++) {
        L2 = ((2 * i - 1 + alpha - x) * L1 - (i - 1 + alpha) * L0) / i;
        L0 = L1;
        L1 = L2;
    }
    return L2;
}

double factorial(int n) {
    if (n <= 1) return 1.0;
    double fact = 1.0;
    for (int i = 2; i <= n; i++) {
        fact *= i;
    }
    return fact;
}

double compute_radial_wave_function(int n, int l, double r) {

    if (l >= n || l < 0 || n < 1) return 0.0;

    double a0 = 1.0; 
    
    double rho = (2.0 * r) / (n * a0);

    double num = factorial(n - l - 1);
    double den = 2.0 * n * pow(factorial(n + l), 3.0); 
    
    double radial_constant = sqrt(pow(2.0 / (n * a0), 3.0) * (num / den));

    int k = n - l - 1;
    int alpha = 2 * l + 1;
    double laguerre_val = compute_laguerre(k, alpha, rho);

    double exponential = exp(-rho / 2.0);
    double rho_power = pow(rho, l);

    return radial_constant * exponential * rho_power * laguerre_val;
}

double compute_spherical_harmonic(int l, int m, double theta, double phi) {
    if (abs(m) > l || l < 0) return 0.0;

    int abs_m = abs(m);

    double num = factorial(l - abs_m);
    double den = factorial(l + abs_m);
    double angular_constant = sqrt(((2 * l + 1) / (4.0 * M_PI)) * (num / den));


    double cos_theta = cos(theta);
    double legendre_val = compute_legendre(l, abs_m, cos_theta);

    double phi_term = 1.0;
    if (m > 0) {
        phi_term = cos(m * phi);
    } else if (m < 0) {
        phi_term = sin(abs_m * phi);
    }

    double phase = 1.0;
    if (m > 0 && (m % 2 != 0)) {
        phase = -1.0;
    }

    return phase * angular_constant * legendre_val * phi_term;
}

double compute_orbital_probability(int n, int l, int m, double x, double z) {
    double r = sqrt(x * x + z * z);
    double theta = atan2(z, x); 
    double phi = atan2(z, x); 

    double rho = (2.0 * r) / n; 
    int k = n - l - 1;
    int alpha = 2 * l + 1;
    double laguerre_val = compute_laguerre(k, alpha, rho);
    double R = exp(-rho / 2.0) * pow(rho, l) * laguerre_val;

    double Y = compute_spherical_harmonic(l, m, theta, phi);

    double psi = R * Y;

    return psi * psi; 
}

void advance_quantum_state(int* n, int* l, int* m) {
    (*m)++;
    if (*m > *l) {
        (*l)++;
        if (*l >= *n) {
            (*n)++;
            if (*n > 4) {
                *n = 1; 
            }
            *l = 0;    
        }
        *m = -(*l);  
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    const int HEIGHT = 900;
    const int WIDTH = 900;
    const int SPEED = 550;
    // Quantum Numbers
    int n = 1;
    int l = 0;
    int m = 0;


    SDL_Window* window = SDL_CreateWindow("Hydrogen Orbitals", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) { SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH, 
        HEIGHT
    );

    if (!texture) {
        printf("Texture creation failed: %s\n", SDL_GetError());
    }


    pixels = (uint32_t*)malloc(WIDTH * HEIGHT * sizeof(uint32_t));

    if (!pixels) {
        printf("Pixel buffer allocation failed!\n");
    }


    SDL_Event event;
    bool quit = false;
    uint32_t last_switch_time = SDL_GetTicks(); 
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
               (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            }
        }
        uint32_t current_time = SDL_GetTicks();

        if (current_time - last_switch_time >= SPEED) {
            advance_quantum_state(&n, &l, &m);
                        
            char title_buffer[64];
            sprintf(title_buffer, "Hydrogen Orbitals | n: %d  l: %d  m: %d", n, l, m);
            
            SDL_SetWindowTitle(window, title_buffer);

            printf("\rOrbital | n: %d  l: %d  m: %d        ", n, l, m);
            fflush(stdout);   
            
            last_switch_time = current_time;
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int screenY = 0; screenY < HEIGHT; screenY++) {
            for (int screenX = 0; screenX < WIDTH; screenX++) {
                
                double scale = n * n * 8.0; 
                double x = ((double)screenX - WIDTH / 2.0) * (scale / (WIDTH / 2.0));
                double z = ((HEIGHT / 2.0) - (double)screenY) * (scale / (HEIGHT / 2.0));

                double intensity = compute_orbital_probability(n, l, m, x, z);
                double dynamic_boost = 0.01 / (double)(n * n); 

                double brightness = pow(intensity / dynamic_boost, 0.1);
                brightness = pow(brightness, 2.0);

                double threshold = 0.12;
                if (brightness < threshold) {
                    brightness = 0.0;
                } else {
                    brightness = (brightness - threshold) / (1.0 - threshold);
                    brightness = brightness * brightness;
                }

                if (brightness > 1.0) brightness = 1.0;

                uint8_t red   = (uint8_t)(pow(brightness, 1.2) * 255);
                uint8_t green = (uint8_t)(pow(brightness, 2.8) * 255);
                uint8_t blue  = (uint8_t)(pow(brightness, 6.0) * 255);
                uint8_t alpha = 255;

                uint32_t pixel_color = (red << 24) | (green << 16) | (blue << 8) | alpha;

                pixels[screenY * WIDTH + screenX] = pixel_color;

            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(uint32_t));

        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, texture, NULL, NULL);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (pixels) {
        free(pixels); 
        pixels = NULL;
    }

    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}