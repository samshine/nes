#ifndef IO_H
#define IO_H

#include <iostream>
#include <thread>
#include <cmath>
#include <ctime>
#include <map>
#include <iomanip>
#include <functional>
#include <cctype>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
#include <GL/glext.h>

class IO {
  
  friend class CPU;
  friend class PPU;
  
  private:
    SDL_Window *window;
    SDL_GLContext glcon;
    GLuint texture;
    
  private:
    int button_index { 0 };
    uint8_t button_state[8];
    
    uint8_t input_state(uint8_t i);
    uint8_t handle_input();
    void strobe();
    void swap();
    void clear();
    
  public:
    IO();
    ~IO();

  public:
    void swap_with(std::vector<uint32_t> const&);
    
};

#endif