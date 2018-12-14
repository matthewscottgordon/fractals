#include <SDL.h>

#include <cassert>
#include <complex>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

template <typename Struct, typename Constructor, typename Destructor,
          typename... Args>
std::unique_ptr<Struct, Destructor> make_unique_ptr_sdl(Constructor constructor,
                                                        Destructor destructor,
                                                        Args... args) {
   Struct *ptr = constructor(args...);
   if (ptr == nullptr)
      throw std::runtime_error(SDL_GetError());
   return std::unique_ptr<Struct, Destructor>(ptr, destructor);
}

class Initialise_sdl {
 public:
   Initialise_sdl(uint32_t flags) {
      if (SDL_Init(flags) != 0) {
         throw std::runtime_error(SDL_GetError());
      }
   }
   ~Initialise_sdl() { SDL_Quit(); }
};

template <typename Float_type, typename Int_type>
Float_type NormalizeInt(Int_type int_value) {
   return static_cast<Float_type>(int_value) *
          (1.0 / static_cast<Float_type>(std::numeric_limits<Int_type>::max()));
}

template <typename Int_type, typename Float_type>
Int_type DenormalizeInt(Float_type float_value) {
   return static_cast<Int_type>(
       float_value *
       static_cast<Float_type>(std::numeric_limits<Int_type>::max()));
}

struct Colour {
   uint8_t red;
   uint8_t green;
   uint8_t blue;

   Colour(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}

   Colour(double r, double g, double b)
       : red(DenormalizeInt<uint8_t>(r)), green(DenormalizeInt<uint8_t>(g)),
         blue(DenormalizeInt<uint8_t>(b)) {}
};

template <typename Func>
void generate_pixels(uint8_t *buffer, size_t width, size_t height, size_t pitch,
                     Func f) {
   static size_t constexpr channels = 4;
   auto loops = [buffer, width, height, pitch, f](size_t i_start,
                                                  size_t i_end) {
      for (size_t i = i_start; i < i_end; ++i) {
         double y = static_cast<double>(i) / static_cast<double>(height);
         for (size_t j = 0; j < width; ++j) {
            uint8_t *address = buffer + i * pitch + j * channels;
            double x = static_cast<double>(j) / static_cast<double>(width);
            Colour c = f(x, y);
            address[0] = 255;
            address[1] = c.blue;
            address[2] = c.green;
            address[3] = c.red;
         }
      }
   };
   constexpr size_t num_threads = 3;
   std::vector<std::future<void>> futures(num_threads);
   for (int thread = 0; thread < num_threads; ++thread)
      futures[thread] =
          std::async(std::launch::async, loops, thread * height / num_threads,
                     ((thread + 1) * height / num_threads));
   for (auto &elem : futures)
      elem.wait();
}

Colour gradient(double x, double y) { return Colour(x, y, 0); }

Colour mandelbrot(double x, double y) {
   auto c = std::complex<double>((x * 4.0) - 2.0, (y * 4.0) - 2.0);
   auto v = std::complex<double>(0, 0);
   int i = 0;
   for (; i < 1000 && std::abs(v) < 2.0; ++i)
      v = v * v + c;
   double i_scaled = std::min(1.0, static_cast<double>(i) / 200);
   if (std::abs(v) > 2.0)
      return {i_scaled, i_scaled, i_scaled};
   return {0.0, 0.0, 0.0};
}

Colour julia(std::complex<double> v, std::complex<double> c) {
   int i = 0;
   for (; i < 1000 && std::abs(v) < 2.0; ++i)
      v = v * v + c;
   double i_scaled = std::min(1.0, static_cast<double>(i) / 200);
   if (std::abs(v) > 2.0)
      return {i_scaled, i_scaled, i_scaled};
   return {0.0, 0.0, 0.0};
}

Colour animate_julia(double x, double y, double t) {
   return julia(std::complex<double>((x * 4.0) - 2.0, (y * 4.0) - 2.0),
                0.7885 * std::exp(t * std::complex<double>(0, 1)));
}

int main(int argc, char *argv[]) {
   static constexpr size_t image_width = 2350;
   static constexpr size_t image_height = 1920;
   try {
      Initialise_sdl(SDL_INIT_VIDEO);

      auto window = make_unique_ptr_sdl<SDL_Window>(
          &SDL_CreateWindow, &SDL_DestroyWindow, "Hello World!", 30, 30,
          image_width, image_height, SDL_WINDOW_SHOWN);

      auto renderer = make_unique_ptr_sdl<SDL_Renderer>(
          &SDL_CreateRenderer, &SDL_DestroyRenderer, window.get(), -1,
          SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

      SDL_RenderClear(renderer.get());
      SDL_RenderPresent(renderer.get());

      auto texture = make_unique_ptr_sdl<SDL_Texture>(
          &SDL_CreateTexture, &SDL_DestroyTexture, renderer.get(),
          SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, image_width,
          image_height);
      uint8_t *pixels = nullptr;
      int pitch = 0;
      SDL_LockTexture(texture.get(), nullptr,
                      reinterpret_cast<void **>(&pixels), &pitch);
      assert(pitch == image_width * sizeof(uint32_t));
      generate_pixels(
          pixels, image_width, image_height, pitch,
          [](double x, double y) { return animate_julia(x, y, 0); });
      SDL_UnlockTexture(texture.get());

      SDL_RenderClear(renderer.get());
      SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
      SDL_RenderPresent(renderer.get());

      auto start_time = std::chrono::steady_clock::now();
      bool quit = false;
      while (!quit) {
         double t = static_cast<double>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start_time)
                            .count()) *
                    0.0001;
         uint8_t *pixels = nullptr;
         int pitch = 0;
         SDL_LockTexture(texture.get(), nullptr,
                         reinterpret_cast<void **>(&pixels), &pitch);
         assert(pitch == image_width * sizeof(uint32_t));
         generate_pixels(
             pixels, image_width, image_height, pitch,
             [t](double x, double y) { return animate_julia(x, y, t); });

         SDL_UnlockTexture(texture.get());
         SDL_RenderClear(renderer.get());
         SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
         SDL_RenderPresent(renderer.get());

         SDL_Event event;
         while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
               quit = true;
      }
   } catch (const std::runtime_error &e) {
      std::cerr << e.what() << std::endl;
      SDL_Quit();
      return 1;
   }

   return 0;
}
