#include <SDL.h>

#include <cassert>
#include <iostream>
#include <memory>

template <typename Struct, typename Constructor, typename Destructor,
          typename... Args>
std::unique_ptr<Struct, Destructor> make_unique_ptr_sdl(Constructor constructor,
                                                        Destructor destructor,
                                                        Args... args)
{
   Struct* ptr = constructor(args...);
   if (ptr == nullptr) throw std::runtime_error(SDL_GetError());
   return std::unique_ptr<Struct, Destructor>(ptr, destructor);
}

class Initialise_sdl
{
public:
   Initialise_sdl(uint32_t flags)
   {
      if (SDL_Init(flags) != 0) {
         throw std::runtime_error(SDL_GetError());
      }
   }
   ~Initialise_sdl()
   {
      SDL_Quit();
   }
};


int main(int argc, char* argv[])
{
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
      uint32_t* pixels = nullptr;
      int pitch = 0;
      SDL_LockTexture(texture.get(), nullptr, reinterpret_cast<void**>(&pixels),
                      &pitch);
      std::cerr << pitch << '\n';
      assert(pitch == image_width * sizeof(uint32_t));
      std::fill_n(pixels, image_width * image_height, 0x00ff00ff);
      SDL_UnlockTexture(texture.get());

      SDL_RenderClear(renderer.get());
      SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
      SDL_RenderPresent(renderer.get());

      bool quit = false;
      while (!quit) {
         SDL_Event event;
         if (SDL_WaitEventTimeout(&event, 100))
            if (event.type == SDL_QUIT) quit = true;
         SDL_RenderClear(renderer.get());
         SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
         SDL_RenderPresent(renderer.get());
      }
   } catch (const std::runtime_error& e) {
      std::cerr << e.what() << std::endl;
      SDL_Quit();
      return 1;
   }

   return 0;
}
